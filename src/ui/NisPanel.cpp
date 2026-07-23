#include "ui/NisPanel.h"
#include "core/ChunkReader.h"
#include "core/LZCDecompressor.h"
#include "core/Logger.h"
#include "formats/TPKBlock.h"
#include "formats/SolidList.h"
#include "formats/NisEventSequence.h"
#include "renderer/GLCompat.h"
#include <vangui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <cstring>

namespace nfsmw {

// ─── Chunk collection ────────────────────────────────────────────────────────

static void CollectNisChunks(std::span<const uint8_t> data, size_t base,
                              std::vector<std::pair<size_t, uint32_t>>& out) {
    size_t pos = 0;
    while (pos + sizeof(ChunkHeader) <= data.size()) {
        ChunkHeader h{};
        std::memcpy(&h, data.data() + pos, sizeof(h));
        if (h.id == 0 && h.size == 0) break;
        const size_t payloadOff = pos + sizeof(ChunkHeader);
        if (payloadOff + h.size > data.size()) break;

        if (h.id == ChunkID::NisAnimation) {
            out.emplace_back(base + pos, h.size); // record header offset + size
        } else if (ChunkReader::IsContainer(h.id) && h.size >= 8) {
            CollectNisChunks(data.subspan(payloadOff, h.size),
                              base + payloadOff, out);
        }
        pos = payloadOff + h.size;
    }
}

// ─── Container size fixup ────────────────────────────────────────────────────
//
// After splicing a new payload into rawBytes_ we walk the entire chunk tree and
// recompute every container's size field from its actual children.
// Leaf chunks are trusted (we wrote their size field correctly at import time).

size_t NisPanel::FixContainerSizesAt(std::vector<uint8_t>& buf,
                                     size_t pos, size_t bufEnd) {
    if (pos + 8 > bufEnd || pos + 8 > buf.size()) return 0;

    uint32_t id, size;
    std::memcpy(&id,   buf.data() + pos,     4);
    std::memcpy(&size, buf.data() + pos + 4, 4);

    if (!(id & 0x80000000u)) {
        // Leaf chunk: size field is correct (either unchanged or freshly written).
        return 8 + size;
    }

    // Container: walk children using their own size fields to advance.
    size_t childPos = pos + 8;
    size_t childTotal = 0;

    while (childPos + 8 <= bufEnd && childPos + 8 <= buf.size()) {
        uint32_t childId, childSize;
        std::memcpy(&childId,   buf.data() + childPos,     4);
        std::memcpy(&childSize, buf.data() + childPos + 4, 4);
        if (childId == 0 && childSize == 0) break;

        size_t consumed = FixContainerSizesAt(buf, childPos, bufEnd);
        if (consumed == 0) break;
        childTotal += consumed;
        childPos   += consumed;
    }

    uint32_t newSize32 = static_cast<uint32_t>(childTotal);
    std::memcpy(buf.data() + pos + 4, &newSize32, 4);
    return 8 + childTotal;
}

void NisPanel::FixAllContainerSizes(std::vector<uint8_t>& buf) {
    size_t pos = 0;
    while (pos + 8 <= buf.size()) {
        size_t consumed = FixContainerSizesAt(buf, pos, buf.size());
        if (consumed == 0) break;
        pos += consumed;
    }
}

// ─── Init / Shutdown ─────────────────────────────────────────────────────────

Result<void> NisPanel::Init() {
    static constexpr const char* kVert = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
)glsl";
    static constexpr const char* kFrag = R"glsl(
#version 330 core
out vec4 fragColor;
uniform vec4 uColor;
void main() { fragColor = uColor; }
)glsl";

    auto r = skelShader_.Build(kVert, kFrag);
    if (!r) return r;

    glGenVertexArrays(1, &lineVAO_);
    glGenBuffers(1, &lineVBO_);
    glBindVertexArray(lineVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(sizeof(glm::vec3)), nullptr);
    glBindVertexArray(0);

    nisCam_.target = { 0.f, 0.f, 0.9f }; // roughly waist height in game units
    nisCam_.radius = 3.0f;
    nisCam_.pitch  = 20.f;
    nisCam_.yaw    = 45.f;
    nisCam_.nearZ  = 0.05f;
    nisCam_.farZ   = 100.f;

    skelShaderOk_ = true;

    // ── Phong mesh shader (same as ObjectExportPanel) ─────────────────────────
    static constexpr const char* kMVert = R"GLSL(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in vec4 aColor;
uniform mat4 uModel;
uniform mat3 uNormalMat;
uniform mat4 uView;
uniform mat4 uProj;
out vec3 vNormal;
out vec3 vFragPos;
out vec2 vUV;
out vec4 vColor;
void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vFragPos  = world.xyz;
    vNormal   = uNormalMat * aNormal;
    vUV       = aUV;
    vColor    = aColor;
    gl_Position = uProj * uView * world;
}
)GLSL";
    static constexpr const char* kMFrag = R"GLSL(
#version 330 core
in vec3 vNormal;
in vec3 vFragPos;
in vec2 vUV;
in vec4 vColor;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;
uniform bool uSelected;
uniform bool uHasTex;
uniform int  uBlendMode;
uniform sampler2D uTex;
uniform float uFogDensity;
out vec4 FragColor;
void main() {
    vec3 n  = normalize(vNormal);
    float d = max(dot(n, normalize(-uLightDir)), 0.0);
    vec3 base  = vColor.rgb * 2.0;
    float alpha = 1.0;
    if (uHasTex && uBlendMode == 1) {
        float shadow = texture(uTex, vUV).a;
        FragColor = vec4(0.0, 0.0, 0.0, shadow * 0.55);
        return;
    }
    if (uHasTex && uBlendMode == 2) {
        vec3 t = texture(uTex, vUV).rgb;
        FragColor = vec4(t * vColor.rgb * 2.0, 1.0);
        return;
    }
    if (uHasTex) {
        vec4 t = texture(uTex, vUV);
        if (t.a < 0.1) discard;
        base *= t.rgb;
        alpha = t.a;
    }
    vec3 col = (uAmbient + d * uLightColor) * base;
    FragColor = vec4(col, alpha);
}
)GLSL";
    if (auto r2 = meshShader_.Build(kMVert, kMFrag); !r2)
        LOG_WARN("NisPanel: mesh shader failed: {}", r2.error);
    else
        meshShaderOk_ = true;

    return Result<void>::Ok();
}

void NisPanel::Shutdown() {
    FreeMeshAndTextures();
    if (lineVAO_) { glDeleteVertexArrays(1, &lineVAO_); lineVAO_ = 0; }
    if (lineVBO_) { glDeleteBuffers(1, &lineVBO_); lineVBO_ = 0; }
    DeleteNisFBO();
    skelShaderOk_  = false;
    lineVertCount_ = 0;
}

// ─── Mesh + texture loading ───────────────────────────────────────────────────

void NisPanel::LoadMeshAndTextures() {
    FreeMeshAndTextures();
    if (rawBytes_.empty()) return;

    std::span<const uint8_t> data(rawBytes_);
    size_t pos = 0;

    while (pos + 8 <= data.size()) {
        uint32_t id = 0, sz = 0;
        std::memcpy(&id, data.data() + pos,     4);
        std::memcpy(&sz, data.data() + pos + 4, 4);
        size_t payloadOff = pos + 8;
        if (payloadOff + sz > data.size()) break;

        if (id == ChunkID::TPKContainer && sz >= 8) {
            auto r = TPKBlockParser::Parse(data.subspan(payloadOff, sz),
                                           static_cast<uint64_t>(payloadOff));
            if (r) {
                nisTexMgr_.UploadBlock(r.value);
                LOG_INFO("NisPanel: TPK '{}' → {} textures uploaded",
                         r.value.name, r.value.textures.size());
            }
        } else if (id == ChunkID::GeometryContainer && sz >= 8) {
            auto r = SolidListParser::Parse(data.subspan(payloadOff, sz),
                                            static_cast<uint64_t>(payloadOff));
            if (r) {
                for (auto& obj : r.value.objects) {
                    uint64_t h = nisMeshRenderer_.UploadGeometry(obj);
                    if (h) meshHandles_.push_back(h);
                }
                LOG_INFO("NisPanel: SolidList '{}' → {} objects uploaded",
                         r.value.sectionName, r.value.objects.size());
            }
        } else if (id & 0x80000000u) {
            // Container — recurse by not advancing past it; instead fall through
            // and let the inner walk happen on the next iteration via payloadOff.
            // Simple flat walk: just step into containers by descending one level.
            pos = payloadOff; // descend
            continue;
        }

        pos = payloadOff + sz;
    }

    hasMesh_ = !meshHandles_.empty();
}

void NisPanel::FreeMeshAndTextures() {
    for (uint64_t h : meshHandles_) nisMeshRenderer_.DeleteGeometry(h);
    meshHandles_.clear();
    nisMeshRenderer_.Clear();
    hasMesh_ = false;
}

// ─── Skeleton viewport ────────────────────────────────────────────────────────

bool NisPanel::EnsureNisFBO(int w, int h) {
    if (nisFbo_ && nisFboW_ == w && nisFboH_ == h) return true;
    DeleteNisFBO();

    glGenFramebuffers(1, &nisFbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, nisFbo_);

    glGenTextures(1, &nisFboColor_);
    glBindTexture(GL_TEXTURE_2D, nisFboColor_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, nisFboColor_, 0);

    glGenRenderbuffers(1, &nisFboDepth_);
    glBindRenderbuffer(GL_RENDERBUFFER, nisFboDepth_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, nisFboDepth_);

    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!ok) { DeleteNisFBO(); return false; }
    nisFboW_ = w;
    nisFboH_ = h;
    return true;
}

void NisPanel::DeleteNisFBO() {
    if (nisFbo_)      { glDeleteFramebuffers(1, &nisFbo_);       nisFbo_      = 0; }
    if (nisFboColor_) { glDeleteTextures(1, &nisFboColor_);      nisFboColor_ = 0; }
    if (nisFboDepth_) { glDeleteRenderbuffers(1, &nisFboDepth_); nisFboDepth_ = 0; }
    nisFboW_ = nisFboH_ = 0;
}

void NisPanel::BuildSkeletonLines() {
    lineVertCount_ = 0;
    skelBuiltFor_  = selectedClip_;
    poseBuiltTime_ = timeSec_;
    poseBuiltMode_ = static_cast<int>(poseMode_);

    if (selectedClip_ < 0 || selectedClip_ >= static_cast<int>(clips_.size())) return;
    const auto& clip = clips_[static_cast<size_t>(selectedClip_)];
    if (!clip.anim.skeleton || clip.anim.skeleton->bones.empty()) return;

    const auto& skel = *clip.anim.skeleton;
    const uint32_t N = skel.numBones;

    // Pose the rig at the current playhead. EvaluateNisPose does the forward
    // kinematics and (when solved) the per-bone keyframe sample; today it is the
    // bind pose or a clearly-synthetic sway (see NisPoseMode).
    std::vector<glm::mat4> worldT;
    if (!EvaluateNisPose(skel, timeSec_, poseMode_, worldT)) return;

    // World-space joint positions (4th column of each world transform)
    std::vector<glm::vec3> wpos(N);
    for (uint32_t i = 0; i < N; ++i)
        wpos[i] = glm::vec3(worldT[i][3]);

    // Build line pairs: parent joint → child joint
    std::vector<glm::vec3> lines;
    lines.reserve(N * 2);
    for (uint32_t i = 0; i < N; ++i) {
        int32_t p = skel.bones[i].parent;
        if (p >= 0 && static_cast<uint32_t>(p) < N) {
            lines.push_back(wpos[p]);
            lines.push_back(wpos[i]);
        }
    }
    if (lines.empty()) return;

    glBindVertexArray(lineVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(lines.size() * sizeof(glm::vec3)),
                 lines.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
    lineVertCount_ = static_cast<int>(lines.size());
}

// ─── Timeline / playback ──────────────────────────────────────────────────────

namespace {
// Stable per-actor marker colour from a small palette.
VanU32 MarkerColor(const std::string& actor) {
    static const VanVec4 kPalette[8] = {
        VanVec4(0.98f, 0.62f, 0.20f, 0.95f), VanVec4(0.35f, 0.80f, 0.98f, 0.95f),
        VanVec4(0.55f, 0.85f, 0.45f, 0.95f), VanVec4(0.98f, 0.45f, 0.55f, 0.95f),
        VanVec4(0.75f, 0.60f, 0.98f, 0.95f), VanVec4(0.95f, 0.85f, 0.35f, 0.95f),
        VanVec4(0.45f, 0.70f, 0.98f, 0.95f), VanVec4(0.98f, 0.72f, 0.85f, 0.95f),
    };
    uint32_t h = 2166136261u;
    for (char c : actor) { h ^= static_cast<uint8_t>(c); h *= 16777619u; }
    return VanGui::GetColorU32(kPalette[h % 8u]);
}
} // namespace

void NisPanel::UpdatePlayback() {
    const float dt = VanGui::GetIO().DeltaTime;
    if (playing_ && duration_ > 0.f) {
        timeSec_ += dt * playSpeed_;
        if (timeSec_ >= duration_) {
            if (loop_) {
                timeSec_ = std::fmod(timeSec_, duration_);
            } else {
                timeSec_ = duration_;
                playing_ = false;
            }
        }
    }
    if (timeSec_ < 0.f) timeSec_ = 0.f;

    // Most-recently fired event (timeline_.events is sorted by time).
    curEventIdx_ = -1;
    for (size_t i = 0; i < timeline_.events.size(); ++i) {
        if (timeline_.events[i].time <= timeSec_) curEventIdx_ = static_cast<int>(i);
        else break;
    }
}

void NisPanel::DrawTimelineBar(float w) {
    // ── Transport row ─────────────────────────────────────────────────────────
    const char* playLbl = playing_ ? "\xe2\x8f\xb8 Pause" : "\xe2\x96\xb6 Play";
    if (VanGui::Button(playLbl, VanVec2(90, 0))) playing_ = !playing_;
    VanGui::SameLine();
    if (VanGui::Button("\xe2\x8f\xae Rewind", VanVec2(90, 0))) { timeSec_ = 0.f; curEventIdx_ = -1; }
    VanGui::SameLine();
    (void)VanGui::Checkbox("Loop", &loop_);

    VanGui::SameLine(0, 12);
    (void)VanGui::Checkbox("Auto-orbit", &autoOrbit_);
    if (VanGui::IsItemHovered())
        VanGui::SetTooltip("Spin the preview camera during playback.\n"
                           "NOT the authored cinematic camera — that path is a locked\n"
                           "keyframe track (encyclopedia C10.4).");

    VanGui::SameLine(0, 12);
    VanGui::SetNextItemWidth(110.f);
    (void)VanGui::SliderFloat("##nisSpeed", &playSpeed_, 0.1f, 3.0f, "%.2fx");

    VanGui::SameLine(0, 12);
    VanGui::Text("%.2f / %.2f s", timeSec_, duration_);

    VanGui::SameLine(0, 16);
    bool synth = (poseMode_ == NisPoseMode::SyntheticIdle);
    if (VanGui::Checkbox("Synthetic preview", &synth))
        poseMode_ = synth ? NisPoseMode::SyntheticIdle : NisPoseMode::BindPose;
    if (VanGui::IsItemHovered())
        VanGui::SetTooltip("Apply a small SYNTHETIC sway so the posing path is visible.\n"
                           "NOT the game's authored motion — the real _q/_t/_s keyframes\n"
                           "are 8-bit-quantised and not yet decodable (C10.3).");

    // ── Scrub slider (full width) ─────────────────────────────────────────────
    VanGui::SetNextItemWidth(w - 12.f);
    float t = timeSec_;
    if (VanGui::SliderFloat("##nisTime", &t, 0.f, duration_ > 0.f ? duration_ : 1.f, "")) {
        timeSec_ = std::clamp(t, 0.f, duration_);
        playing_ = false;   // scrubbing pauses
    }
    const VanVec2 rmin = VanGui::GetItemRectMin();
    const VanVec2 rmax = VanGui::GetItemRectMax();
    const bool sliderHovered = VanGui::IsItemHovered();
    VanDrawList* dl = VanGui::GetWindowDrawList();
    const float trackW = std::max(rmax.x - rmin.x, 1.f);

    // Real event markers from the decoded schedule.
    if (timeline_.valid && duration_ > 0.f) {
        const float mouseX = VanGui::GetMousePos().x;
        int   nearest   = -1;
        float nearestDx = 6.f;
        for (size_t i = 0; i < timeline_.events.size(); ++i) {
            const NisTimelineEvent& ev = timeline_.events[i];
            const float fx = rmin.x + (ev.time / duration_) * trackW;
            dl->AddLine(VanVec2(fx, rmin.y + 2.f), VanVec2(fx, rmax.y - 2.f),
                        MarkerColor(ev.actor), 1.6f);
            if (sliderHovered) {
                const float dx = std::abs(fx - mouseX);
                if (dx < nearestDx) { nearestDx = dx; nearest = static_cast<int>(i); }
            }
        }
        if (nearest >= 0) {
            const NisTimelineEvent& ev = timeline_.events[static_cast<size_t>(nearest)];
            if (VanGui::BeginTooltip()) {
                VanGui::Text("%.2fs  %s", ev.time,
                             ev.label.empty() ? "(event)" : ev.label.c_str());
                if (!ev.actor.empty()) VanGui::TextDisabled("actor: %s", ev.actor.c_str());
                VanGui::EndTooltip();
            }
        }
    }

    // Playhead.
    const float px = rmin.x + (duration_ > 0.f ? timeSec_ / duration_ : 0.f) * trackW;
    dl->AddLine(VanVec2(px, rmin.y), VanVec2(px, rmax.y),
                VanGui::GetColorU32(VanVec4(1.f, 1.f, 1.f, 0.9f)), 2.0f);

    // ── Status line ───────────────────────────────────────────────────────────
    if (!timeline_.valid) {
        VanGui::TextDisabled(
            "No event timeline in this bundle — duration is a default; bone & camera "
            "motion are the locked keyframe streams (encyclopedia C10.3).");
    } else {
        std::string cur = "\xe2\x80\x94"; // em dash
        if (curEventIdx_ >= 0 && curEventIdx_ < static_cast<int>(timeline_.events.size())) {
            const NisTimelineEvent& ev = timeline_.events[static_cast<size_t>(curEventIdx_)];
            cur = ev.label.empty() ? std::string("(event)") : ev.label;
            if (!ev.actor.empty()) cur += "  [" + ev.actor + "]";
        }
        VanGui::TextDisabled("Scene: %s   |   %d events / %d lanes   |   last fired: %s",
                             timeline_.sceneName.empty() ? "(unnamed)"
                                                         : timeline_.sceneName.c_str(),
                             static_cast<int>(timeline_.events.size()),
                             static_cast<int>(timeline_.lanes.size()), cur.c_str());
    }
}

void NisPanel::UpdateNisCamInput(float /*vpW*/, float vpH, bool hovered) {
    auto& io = VanGui::GetIO();
    camInput_.orbiting  = hovered && io.MouseDown[0] && !io.KeyShift;
    camInput_.panning   = hovered && io.MouseDown[0] &&  io.KeyShift;
    camInput_.looking   = hovered && io.MouseDown[1];
    camInput_.deltaX    = io.MouseDelta.x;
    camInput_.deltaY    = io.MouseDelta.y;
    camInput_.scroll    = hovered ? io.MouseWheel : 0.f;
    camInput_.viewportH = vpH;

    // Auto-orbit: drive a slow spin through the same input path (so the camera's
    // cached view matrix is invalidated correctly) when the user isn't steering.
    if (autoOrbit_ && playing_ &&
        !camInput_.orbiting && !camInput_.panning && !camInput_.looking) {
        camInput_.orbiting = true;
        camInput_.deltaX   = -(18.f * io.DeltaTime) / 0.35f; // ~18 deg/s
        camInput_.deltaY   = 0.f;
        camInput_.scroll   = 0.f;
    }

    nisCam_.Update(camInput_);
}

void NisPanel::DrawSkeletonViewport(float w, float h) {
    int iw = static_cast<int>(w);
    int ih = static_cast<int>(h);
    if (iw < 2 || ih < 2) return;

    // Rebuild the posed skeleton when the clip, playhead, or pose mode changes.
    if (skelBuiltFor_ != selectedClip_ ||
        poseBuiltTime_ != timeSec_ ||
        poseBuiltMode_ != static_cast<int>(poseMode_))
        BuildSkeletonLines();

    // Input capture: InvisibleButton claims the rect for hover/click
    (void)VanGui::InvisibleButton("##nis_vp", VanVec2(w, h));
    bool hovered = VanGui::IsItemHovered();
    VanVec2 rectMin = VanGui::GetItemRectMin();
    UpdateNisCamInput(w, h, hovered);

    if (!skelShaderOk_ || !EnsureNisFBO(iw, ih)) {
        VanDrawList* dl = VanGui::GetWindowDrawList();
        const char* msg = skelShaderOk_ ? "FBO creation failed" : "Shader init failed";
        dl->AddText(VanVec2(rectMin.x + 8, rectMin.y + 8), 0xFF6666AA, msg);
        return;
    }

    // ── Render into FBO ───────────────────────────────────────────────────────
    GLint prevFbo = 0;
    GLint prevVP[4]{};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
    glGetIntegerv(GL_VIEWPORT, prevVP);

    glBindFramebuffer(GL_FRAMEBUFFER, nisFbo_);
    glViewport(0, 0, iw, ih);
    glClearColor(0.09f, 0.10f, 0.12f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    const glm::mat4 view = nisCam_.View();
    const glm::mat4 proj = nisCam_.Proj(w / h);

    // Textured mesh pass (if geometry was found in the BUN)
    if (hasMesh_ && meshShaderOk_) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        meshShader_.Bind();
        meshShader_.SetMat4("uModel",      glm::mat4(1.f));
        meshShader_.SetMat3("uNormalMat",  glm::mat3(1.f));
        meshShader_.SetMat4("uView",       view);
        meshShader_.SetMat4("uProj",       proj);
        meshShader_.SetVec3("uLightDir",   glm::normalize(glm::vec3(0.4f, 0.3f, -0.8f)));
        meshShader_.SetVec3("uLightColor", glm::vec3(0.9f, 0.87f, 0.8f));
        meshShader_.SetVec3("uAmbient",    glm::vec3(0.48f, 0.48f, 0.52f));
        meshShader_.SetBool("uSelected",   false);
        meshShader_.SetFloat("uFogDensity", 0.f);
        meshShader_.SetInt("uTex",         0);
        for (uint64_t handle : meshHandles_)
            nisMeshRenderer_.RenderGeometryImmediate(handle, meshShader_, view, proj, &nisTexMgr_);
        meshShader_.Unbind();
        glDisable(GL_BLEND);
    }

    // Skeleton overlay (always drawn on top so it's visible even without mesh)
    if (lineVertCount_ > 0) {
        glDisable(GL_DEPTH_TEST); // draw on top of mesh
        skelShader_.Bind();
        skelShader_.SetMat4("uMVP", proj * view);
        skelShader_.SetVec4("uColor", glm::vec4(0.25f, 0.88f, 0.45f, hasMesh_ ? 0.5f : 1.f));
        glBindVertexArray(lineVAO_);
        glLineWidth(hasMesh_ ? 1.5f : 2.f);
        glDrawArrays(GL_LINES, 0, lineVertCount_);
        glBindVertexArray(0);
        skelShader_.Unbind();
        glEnable(GL_DEPTH_TEST);
    }

    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
    glViewport(prevVP[0], prevVP[1], prevVP[2], prevVP[3]);

    // ── Draw FBO texture via DrawList (same pattern as CarViewerPanel) ────────
    VanDrawList* dl = VanGui::GetWindowDrawList();
    dl->AddImage(
        static_cast<VanTextureID>(static_cast<uintptr_t>(nisFboColor_)),
        rectMin,
        VanVec2(rectMin.x + w, rectMin.y + h),
        VanVec2(0, 1), VanVec2(1, 0));  // flip Y: GL is y-up, VanGui is y-down

    // Overlay hint
    dl->AddText(VanVec2(rectMin.x + 6, rectMin.y + h - 20),
                0xAA888888, "LMB: orbit   Shift+LMB: pan   Scroll: zoom");

    // Honest note on what the rig is doing right now.
    const char* poseNote = (poseMode_ == NisPoseMode::SyntheticIdle)
        ? "skeleton: SYNTHETIC preview (not authored motion)"
        : "skeleton: bind pose (authored keyframes locked \xe2\x80\x94 C10.3)";
    dl->AddText(VanVec2(rectMin.x + 6, rectMin.y + 6), 0xAAAAAAAA, poseNote);

    // "No skeleton" placeholder when bind pose couldn't be decoded
    if (lineVertCount_ == 0) {
        const char* msg = "No skeleton data in this clip";
        VanVec2 msz = VanGui::CalcTextSize(msg);
        dl->AddText(
            VanVec2(rectMin.x + (w - msz.x) * 0.5f,
                    rectMin.y + (h - msz.y) * 0.5f),
            0xFF665566, msg);
    }
}

// ─── Load ─────────────────────────────────────────────────────────────────────

void NisPanel::Load(const std::filesystem::path& path, TaskQueue& tasks) {
    Close();
    state_      = State::Loading;
    sourceName_ = path.filename().string();
    filePath_   = path;

    auto result = std::make_shared<Result<std::pair<std::vector<uint8_t>, bool>>>();

    tasks.Submit("Loading NIS bundle", [path, result](ProgressState& p) {
        p.fraction = -1.f;
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) {
            *result = Result<std::pair<std::vector<uint8_t>, bool>>::Err(
                "Cannot open '" + path.string() + "'");
            return;
        }
        auto sz = static_cast<size_t>(f.tellg());
        f.seekg(0);
        std::vector<uint8_t> raw(sz);
        if (!f.read(reinterpret_cast<char*>(raw.data()),
                    static_cast<std::streamsize>(sz))) {
            *result = Result<std::pair<std::vector<uint8_t>, bool>>::Err(
                "Read error: " + path.string());
            return;
        }
        bool compressed = LZCDecompressor::IsCompressed(raw);
        if (compressed) {
            auto r = LZCDecompressor::Decompress(raw);
            if (!r) {
                *result = Result<std::pair<std::vector<uint8_t>, bool>>::Err(
                    "JDLZ decompress failed: " + r.error);
                return;
            }
            *result = Result<std::pair<std::vector<uint8_t>, bool>>::Ok(
                {std::move(r.value), true});
        } else {
            *result = Result<std::pair<std::vector<uint8_t>, bool>>::Ok(
                {std::move(raw), false});
        }
    }, [this, result] {
        if (!*result) {
            errorMsg_ = result->error;
            state_    = State::Error;
            return;
        }
        rawBytes_      = std::move(result->value.first);
        wasCompressed_ = result->value.second;
        dirty_         = false;
        Reparse();
        selectedClip_  = clips_.empty() ? -1 : 0;
        LoadMeshAndTextures();
        state_         = State::Ready;
        statusMsg_.clear();
    });
}

void NisPanel::Reparse() {
    clips_.clear();
    std::vector<std::pair<size_t, uint32_t>> found;
    CollectNisChunks(rawBytes_, 0, found);

    for (auto& [hdrOff, payloadSize] : found) {
        const size_t payloadOff = hdrOff + 8;
        if (payloadOff + payloadSize > rawBytes_.size()) continue;
        std::span<const uint8_t> payload(rawBytes_.data() + payloadOff, payloadSize);
        auto parsed = NisAnimParser::Parse(payload);
        if (!parsed) continue;
        clips_.push_back({hdrOff, payloadSize, std::move(parsed.value)});
    }

    // Real timeline: decode this scene's event schedule (EventSequenceChunk).
    // Its longest action duration is the clip length; its timed events become
    // the markers on the scrub bar. Absent/unparsable → fall back to a default.
    timeline_ = NisEventSequenceParser::ParseBundle(rawBytes_);
    duration_ = (timeline_.valid && timeline_.duration > 0.05f) ? timeline_.duration : 10.f;
    timeSec_     = 0.f;
    playing_     = false;
    curEventIdx_ = -1;
    if (timeline_.valid)
        LOG_INFO("NisPanel: timeline '{}' — {:.2f}s, {} lanes, {} events",
                 timeline_.sceneName.empty() ? "(scene)" : timeline_.sceneName,
                 timeline_.duration, timeline_.lanes.size(), timeline_.events.size());
}

void NisPanel::Close() {
    clips_.clear();
    rawBytes_.clear();
    selectedClip_  = -1;
    state_         = State::Empty;
    errorMsg_.clear();
    statusMsg_.clear();
    sourceName_.clear();
    filePath_.clear();
    wasCompressed_ = false;
    dirty_         = false;
    boneFilter_[0] = '\0';
    skelBuiltFor_  = -2;
    poseBuiltTime_ = -1.f;
    poseBuiltMode_ = -1;
    lineVertCount_ = 0;
    timeline_      = NisSceneTimeline{};
    playing_       = false;
    timeSec_       = 0.f;
    duration_      = 10.f;
    curEventIdx_   = -1;
    FreeMeshAndTextures();
}

// ─── Chunk I/O ────────────────────────────────────────────────────────────────

void NisPanel::ExportChunk(size_t clipIdx, const std::filesystem::path& outPath) {
    if (clipIdx >= clips_.size()) return;
    const Clip& clip = clips_[clipIdx];
    const size_t payloadOff = clip.absOffset + 8;
    if (payloadOff + clip.payloadSize > rawBytes_.size()) return;

    std::ofstream f(outPath, std::ios::binary);
    if (!f) { statusMsg_ = "Export failed: cannot write " + outPath.string(); return; }
    f.write(reinterpret_cast<const char*>(rawBytes_.data() + payloadOff),
            static_cast<std::streamsize>(clip.payloadSize));
    statusMsg_ = "Exported to " + outPath.filename().string();
}

Result<void> NisPanel::ImportChunk(size_t clipIdx, const std::filesystem::path& srcPath) {
    if (clipIdx >= clips_.size())
        return Result<void>::Err("Invalid clip index");

    std::ifstream f(srcPath, std::ios::binary | std::ios::ate);
    if (!f) return Result<void>::Err("Cannot open " + srcPath.string());
    auto newPayloadSize = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> newPayload(newPayloadSize);
    if (!f.read(reinterpret_cast<char*>(newPayload.data()),
                static_cast<std::streamsize>(newPayloadSize)))
        return Result<void>::Err("Read error: " + srcPath.string());

    const Clip& clip = clips_[clipIdx];
    const size_t hdrOff = clip.absOffset;

    // Sanity-check the chunk header we're about to replace.
    if (hdrOff + 8 > rawBytes_.size())
        return Result<void>::Err("Clip offset out of range");
    uint32_t id;
    std::memcpy(&id, rawBytes_.data() + hdrOff, 4);
    if (id != ChunkID::NisAnimation)
        return Result<void>::Err("Expected NisAnimation chunk at offset");

    const size_t payloadOff    = hdrOff + 8;
    const uint32_t oldPayload  = clip.payloadSize;

    // Write the new size into the chunk header.
    uint32_t newSize32 = static_cast<uint32_t>(newPayloadSize);
    std::memcpy(rawBytes_.data() + hdrOff + 4, &newSize32, 4);

    // Splice: erase old payload, insert new payload at the same position.
    rawBytes_.erase(rawBytes_.begin() + static_cast<ptrdiff_t>(payloadOff),
                    rawBytes_.begin() + static_cast<ptrdiff_t>(payloadOff + oldPayload));
    rawBytes_.insert(rawBytes_.begin() + static_cast<ptrdiff_t>(payloadOff),
                     newPayload.begin(), newPayload.end());

    // Recompute all parent container sizes from actual child sizes.
    FixAllContainerSizes(rawBytes_);

    dirty_ = true;
    Reparse();

    // Re-select the same logical clip (best effort — match by skeleton name).
    selectedClip_ = clips_.empty() ? -1 : 0;
    if (clipIdx < clips_.size()) selectedClip_ = static_cast<int>(clipIdx);

    return Result<void>::Ok();
}

// ─── Draw ────────────────────────────────────────────────────────────────────

void NisPanel::Draw(float w, float h, FileDialog& fd, TaskQueue& /*tasks*/) {
    switch (state_) {
        case State::Empty:   DrawEmptyState(w, h);   break;
        case State::Loading: DrawLoadingState();     break;
        case State::Error:   DrawErrorState();       break;
        case State::Ready:   DrawReadyState(w, h, fd); break;
    }
}

void NisPanel::DrawEmptyState(float w, float h) {
    const char* msg = "Open a NIS cutscene bundle (.BUN) to browse and edit its animation data.";
    VanVec2 sz = VanGui::CalcTextSize(msg);
    VanGui::SetCursorPos(VanVec2((w - sz.x) * 0.5f, (h - sz.y) * 0.5f));
    VanGui::TextDisabled("%s", msg);
}

void NisPanel::DrawLoadingState() {
    VanGui::TextDisabled("Loading…");
}

void NisPanel::DrawErrorState() {
    VanGui::TextColored(VanVec4(0.9f, 0.3f, 0.3f, 1.0f), "Error: %s", errorMsg_.c_str());
}

void NisPanel::DrawReadyState(float w, float h, FileDialog& fd) {
    UpdatePlayback();   // advance the playhead / preview camera before drawing

    // ── Status / info row ───────────────────────────────────────────────────
    VanGui::TextDisabled("%s", sourceName_.c_str());
    if (wasCompressed_) {
        VanGui::SameLine();
        VanGui::TextDisabled("(was JDLZ — save will be uncompressed)");
    }
    if (!statusMsg_.empty()) {
        VanGui::SameLine(0, 16);
        VanGui::TextColored(VanVec4(0.4f, 0.85f, 0.4f, 1.0f), "%s", statusMsg_.c_str());
    }
    VanGui::Spacing();

    if (clips_.empty()) {
        VanGui::TextDisabled("No NisAnimation chunks (0x00E34009) found in this bundle.");
        if (timeline_.valid) {
            VanGui::TextDisabled(
                "This bundle has an event timeline (%s, %.2fs, %d events) but no skeleton "
                "to render — likely an FMV-bridge scene.",
                timeline_.sceneName.empty() ? "scene" : timeline_.sceneName.c_str(),
                timeline_.duration, static_cast<int>(timeline_.events.size()));
        }
        return;
    }

    // ── Transport + scrub timeline (real event schedule) ──────────────────────
    DrawTimelineBar(w);
    VanGui::Separator();

    // ── 3-column resizable layout: clip list | scene viewport | clip detail ──
    const float panelH = std::max(h - 132.0f, 80.0f);

    // Clamp stored widths so panes never collapse
    const float minW = 140.f;
    listPaneW_   = std::max(listPaneW_,   minW);
    detailPaneW_ = std::max(detailPaneW_, minW);

    // Pane 0: clip list (resizable right edge)
    (void)VanGui::BeginChild("##NisClipListPane", VanVec2(listPaneW_, panelH),
                       VanGuiChildFlags_ResizeX | VanGuiChildFlags_Borders);
    listPaneW_ = VanGui::GetWindowWidth(); // track resize
    DrawClipList(listPaneW_, panelH);
    VanGui::EndChild();

    VanGui::SameLine(0, 0);

    // Pane 1: skeleton viewport (resizable right edge)
    const float vpW = w - listPaneW_ - detailPaneW_;
    (void)VanGui::BeginChild("##NisVpPane", VanVec2(vpW > minW ? vpW : minW, panelH),
                       VanGuiChildFlags_ResizeX,
                       VanGuiWindowFlags_NoScrollbar | VanGuiWindowFlags_NoScrollWithMouse);
    const float actualVpW = VanGui::GetWindowWidth();
    DrawSkeletonViewport(actualVpW, panelH);
    VanGui::EndChild();

    VanGui::SameLine(0, 0);

    // Pane 2: clip detail (fills the remainder)
    detailPaneW_ = VanGui::GetContentRegionAvail().x;
    (void)VanGui::BeginChild("##NisDetailPane", VanVec2(detailPaneW_, panelH),
                       VanGuiChildFlags_Borders);
    DrawClipDetail(detailPaneW_, panelH, fd);
    VanGui::EndChild();
}

void NisPanel::DrawClipList(float listW, float h) {
    (void)listW; (void)h;   // sizing handled by the enclosing child window
    VanGui::TextDisabled("Animation Clips (%zu)", clips_.size());
    VanGui::Separator();

    for (int i = 0; i < static_cast<int>(clips_.size()); ++i) {
        const Clip& c = clips_[i];
        const char* skelName = c.anim.skeletonName.empty()
                               ? "(unnamed)" : c.anim.skeletonName.c_str();
        char label[192];
        std::snprintf(label, sizeof(label), "%s  (%zu bones)##clip%d",
                      skelName, c.anim.boneSymbols.size(), i);
        if (VanGui::Selectable(label, selectedClip_ == i))
            selectedClip_ = i;
        if (VanGui::IsItemHovered()) {
            (void)VanGui::BeginTooltip();
            VanGui::Text("Chunk header @ +0x%zX", c.absOffset);
            VanGui::Text("Payload size: %u bytes", c.payloadSize);
            VanGui::EndTooltip();
        }
    }
}

void NisPanel::DrawClipDetail(float detailW, float /*h*/, FileDialog& fd) {
    if (selectedClip_ < 0 || selectedClip_ >= static_cast<int>(clips_.size())) {
        VanGui::TextDisabled("Select a clip from the list.");
        return;
    }

    const Clip& clip = clips_[static_cast<size_t>(selectedClip_)];
    const NisAnimClip& anim = clip.anim;
    const size_t clipIdx = static_cast<size_t>(selectedClip_);

    // ── Header info ──────────────────────────────────────────────────────────
    VanGui::Text("Skeleton:  %s",
                 anim.skeletonName.empty() ? "(unnamed)" : anim.skeletonName.c_str());
    VanGui::Text("Bones:     %zu", anim.boneSymbols.size());
    VanGui::Text("ELF sections: %zu", anim.sections.size());
    VanGui::Text("Payload:   %u bytes", clip.payloadSize);
    VanGui::Spacing();

    // ── Export / Import ──────────────────────────────────────────────────────
    if (VanGui::Button("Export Chunk…")) {
        std::string defaultName = anim.skeletonName.empty()
            ? "clip" : anim.skeletonName;
        defaultName += ".bin";
        fd.Show("Export NIS Animation Chunk", FileDialog::Mode::Save, {".bin"},
                [this, clipIdx](const std::filesystem::path& p) {
                    ExportChunk(clipIdx, p);
                });
    }
    if (VanGui::IsItemHovered())
        VanGui::SetTooltip("Save the raw EAGLAnim payload to a .bin file\n"
                           "for external editing, then import it back here.");

    if (VanGui::Button("Import Chunk…")) {
        fd.Show("Import NIS Animation Chunk", FileDialog::Mode::Open, {".bin"},
                [this, clipIdx](const std::filesystem::path& p) {
                    auto r = ImportChunk(clipIdx, p);
                    if (!r) statusMsg_ = "Import failed: " + r.error;
                    else    statusMsg_ = "Imported " + p.filename().string() + " — save to apply.";
                });
    }
    if (VanGui::IsItemHovered())
        VanGui::SetTooltip("Replace this clip's EAGLAnim payload with a .bin file\n"
                           "previously exported from this panel or an external editor.");

    // ── Save As ──────────────────────────────────────────────────────────────
    if (dirty_) {
        VanGui::PushStyleColor(VanGuiCol_Button, VanVec4(0.18f, 0.55f, 0.18f, 1.0f));
        if (VanGui::Button("Save BUN As…")) {
            fd.Show("Save Modified NIS Bundle", FileDialog::Mode::Save, {".bun"},
                    [this](const std::filesystem::path& p) {
                        std::ofstream out(p, std::ios::binary);
                        if (!out) { statusMsg_ = "Save failed: cannot write file."; return; }
                        out.write(reinterpret_cast<const char*>(rawBytes_.data()),
                                  static_cast<std::streamsize>(rawBytes_.size()));
                        dirty_    = false;
                        statusMsg_ = "Saved to " + p.filename().string();
                    });
        }
        VanGui::PopStyleColor();
        if (VanGui::IsItemHovered())
            VanGui::SetTooltip("Write the modified bundle (uncompressed) to a new file.");
    }

    VanGui::Separator();

    // ── Bone list ────────────────────────────────────────────────────────────
    VanGui::SetNextItemWidth(detailW - 24.0f);
    (void)VanGui::InputTextWithHint("##boneFilter", "Filter bones…",
                              boneFilter_, sizeof(boneFilter_));
    VanGui::Spacing();

    if (VanGui::BeginTable("##bones", 1,
            VanGuiTableFlags_RowBg | VanGuiTableFlags_ScrollY,
            VanVec2(0, 0))) {
        for (size_t bi : anim.boneSymbols) {
            const NisAnimSymbol& sym = anim.symbols[bi];
            std::string display = sym.name;
            size_t dot = display.rfind('.');
            if (dot != std::string::npos) display = display.substr(dot + 1);
            if (boneFilter_[0] != '\0' &&
                display.find(boneFilter_) == std::string::npos)
                continue;
            VanGui::TableNextRow();
            (void)VanGui::TableNextColumn();
            VanGui::TextUnformatted(display.c_str());
        }
        VanGui::EndTable();
    }
}

} // namespace nfsmw
