#include "ui/ObjectExportPanel.h"
#include "core/Logger.h"
#include "core/StringHash.h"
#include "editor/MeshImporter.h"
#include "patch/SolidWriter.h"
#include <vangui.h>
#include "renderer/GLCompat.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <array>
#include <memory>
#include <algorithm>
#include <unordered_set>
#include <utility>

namespace nfsmw {

// ─── GLSL source strings (ported from reference/nfsmapeditor_phase6/renderer/Viewport.cpp) ───

static const char* kPhongVert = R"GLSL(
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

static const char* kPhongFrag = R"GLSL(
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
uniform float uFogDensity;  // 0 = no fog (preview always 0)

out vec4 FragColor;

void main() {
    vec3 n  = normalize(vNormal);
    float d = max(dot(n, normalize(-uLightDir)), 0.0);
    vec3 base = vColor.rgb * 2.0;
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
    if (uSelected) col = mix(col, vec3(1.0, 0.6, 0.1), 0.35);
    FragColor = vec4(col, alpha);
}
)GLSL";

static const char* kWireVert = R"GLSL(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
void main() {
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
)GLSL";

static const char* kWireFrag = R"GLSL(
#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main() { FragColor = uColor; }
)GLSL";

// ─────────────────────────────────────────────────────────────────────────────

Result<void> ObjectExportPanel::Init() {
    auto r1 = shaderPhong_.Build(kPhongVert, kPhongFrag);
    if (!r1) return Result<void>::Err("ObjectExportPanel phong shader: " + r1.error);

    auto r2 = shaderWire_.Build(kWireVert, kWireFrag);
    if (!r2) return Result<void>::Err("ObjectExportPanel wireframe shader: " + r2.error);

    shadersOk_ = true;
    LOG_INFO("ObjectExportPanel: shaders compiled OK");
    return Result<void>::Ok();
}

void ObjectExportPanel::Shutdown() {
    DeleteFBO();
    meshRenderer_.Clear();
    texManager_.Clear();
    currentGeom_ = 0;
    shadersOk_   = false;
    fboReady_    = false;
}

void ObjectExportPanel::CloseFile() {
    if (currentGeom_) {
        meshRenderer_.DeleteGeometry(currentGeom_);
        currentGeom_ = 0;
    }
    texManager_.Clear();
    sections_.clear();
    pendingSections_.clear();
    pendingPath_.clear();
    pendingReady_     = false;
    loadedPath_.clear();
    selectedSection_  = -1;
    selectedList_     = -1;
    selectedObject_   = -1;
    selObj_           = nullptr;
    selSect_          = nullptr;
    previewDirty_     = false;
    importInProgress_ = false;
    reselectHash_     = 0;
    reloadRequested_  = false;
    lastExportStatus_.clear();
    pendingFlags_     = 0;
    flagsDirty_       = false;
    savingFlags_      = false;
    swayPreview_      = false;
    lastFlagStatus_.clear();
    availTextures_.clear();
    texEditSlot_      = -1;
    texNewHash_       = 0;
    retexBusy_        = false;
    lastTexStatus_.clear();
    animBanks_        = {};
    selAnim_          = nullptr;
    animatePreview_   = false;
    emitters_.Clear();
    meshVisible_.clear();
    xformActive_      = false;
    xformSaving_      = false;
    lastXformStatus_.clear();
    objFilter_.Clear();
}

void ObjectExportPanel::SetExtraTPKs(std::vector<TPKBlock> globalPacks) {
    extraTPKs_ = std::move(globalPacks);
    // Upload new packs to the TextureManager.
    for (auto& tp : extraTPKs_)
        texManager_.UploadBlock(tp);
    RebuildAvailableTextures();
}

// ─── File loading ─────────────────────────────────────────────────────────────

void ObjectExportPanel::LoadFileAsync(const std::filesystem::path& path, TaskQueue& queue) {
    pendingReady_ = false;
    const std::string pathStr = path.string();

    queue.Submit("Loading " + path.filename().string(),
        [pathStr, this](ProgressState& p) {
            p.fraction = -1.0f; // indeterminate while parsing
            auto r = StreamBundleLoader::Load(std::filesystem::path(pathStr));
            p.fraction = 1.0f;
            if (r) {
                // Stage data; drain on UI thread.
                pendingSections_.clear();
                pendingSections_.push_back(std::move(r.value));
                pendingPath_ = pathStr;
            } else {
                LOG_WARN("ObjectExportPanel: load failed: {}", r.error);
                pendingSections_.clear();
                pendingPath_.clear();
            }
        },
        [this]() {
            pendingReady_ = true;
        });
}

// ─── Pending-load drain ───────────────────────────────────────────────────────

void ObjectExportPanel::PumpPending() {
    if (!pendingReady_) return;
    pendingReady_ = false;

    // Release previous GPU resources.
    if (currentGeom_) {
        meshRenderer_.DeleteGeometry(currentGeom_);
        currentGeom_ = 0;
    }
    texManager_.Clear();

    sections_    = std::move(pendingSections_);
    loadedPath_  = pendingPath_;

    // Upload textures for all sections.
    for (auto& s : sections_)
        for (auto& tp : s.texturePacks)
            texManager_.UploadBlock(tp);
    for (auto& tp : extraTPKs_)
        texManager_.UploadBlock(tp);

    // Build the pick-list of textures available to swap onto objects.
    RebuildAvailableTextures();

    // Inventory the bundle's world-animation banks (cranes, ships, blimp,
    // airliners) so animated objects can be identified and previewed.
    animBanks_ = {};
    selAnim_   = nullptr;
    if (!loadedPath_.empty()) {
        if (auto r = AnimationBankParser::ScanFile(loadedPath_))
            animBanks_ = std::move(r.value);
    }

    // Load any emitter attachments authored for this bundle.
    emitters_.SetSidecar(loadedPath_.empty() ? std::filesystem::path()
                                             : std::filesystem::path(loadedPath_ + ".fx.json"));
    emitters_.Load();

    // Reset selection.
    selectedSection_ = -1;
    selectedList_    = -1;
    selectedObject_  = -1;
    selObj_          = nullptr;
    selSect_         = nullptr;
    previewDirty_    = false;
    importInProgress_ = false;   // load finished → spinner can stop

    // If this load followed a replace, re-select the same object by name-hash so
    // the viewer shows the new geometry immediately (no manual re-click needed).
    if (reselectHash_) {
        const uint32_t want = reselectHash_;
        reselectHash_ = 0;
        for (int si = 0; si < (int)sections_.size(); ++si)
            for (int li = 0; li < (int)sections_[si].solidLists.size(); ++li) {
                const auto& objs = sections_[si].solidLists[li].objects;
                for (int oi = 0; oi < (int)objs.size(); ++oi)
                    if (objs[oi].nameHash == want) {
                        SelectObject(si, li, oi);
                        return;
                    }
            }
    }
}

// ─── FBO ─────────────────────────────────────────────────────────────────────

bool ObjectExportPanel::EnsureFBO(int w, int h) {
    if (w <= 0 || h <= 0) return false;
    if (fboReady_ && fboW_ == w && fboH_ == h) return true;

    DeleteFBO();

    glGenFramebuffers(1, &fbo_);
    glGenTextures(1, &fboColorTex_);
    glGenRenderbuffers(1, &fboDepthRbo_);

    glBindTexture(GL_TEXTURE_2D, fboColorTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindRenderbuffer(GL_RENDERBUFFER, fboDepthRbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, fboColorTex_, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, fboDepthRbo_);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_WARN("ObjectExportPanel: FBO incomplete ({}x{})", w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        DeleteFBO();
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    fboW_     = w;
    fboH_     = h;
    fboReady_ = true;
    return true;
}

void ObjectExportPanel::DeleteFBO() {
    if (fbo_)         { glDeleteFramebuffers(1, &fbo_);         fbo_ = 0; }
    if (fboColorTex_) { glDeleteTextures(1, &fboColorTex_);     fboColorTex_ = 0; }
    if (fboDepthRbo_) { glDeleteRenderbuffers(1, &fboDepthRbo_);fboDepthRbo_ = 0; }
    fboW_ = fboH_ = 0;
    fboReady_ = false;
}

// ─── Selection ────────────────────────────────────────────────────────────────

void ObjectExportPanel::SelectObject(int sectionIdx, int listIdx, int objIdx) {
    if (sectionIdx < 0 || sectionIdx >= (int)sections_.size()) return;
    const StreamSection& sect = sections_[sectionIdx];
    if (listIdx < 0 || listIdx >= (int)sect.solidLists.size()) return;
    const SolidList& sl = sect.solidLists[listIdx];
    if (objIdx < 0 || objIdx >= (int)sl.objects.size()) return;

    selectedSection_ = sectionIdx;
    selectedList_    = listIdx;
    selectedObject_  = objIdx;
    selObj_          = &sl.objects[objIdx];
    selSect_         = &sect;
    previewDirty_    = true;

    // Reset the flag editor to this object's on-disk value.
    pendingFlags_    = selObj_->flags;
    flagsDirty_      = false;
    lastFlagStatus_.clear();

    // Reset the texture editor to a closed state for the new selection.
    texEditSlot_     = -1;
    texNewHash_      = 0;
    lastTexStatus_.clear();

    // Auto-suggest the wind-sway preview for foliage; leave others static.
    swayPreview_     = ClassifyObject(selObj_->name).swayable;

    // Bind this object to its world-animation bank, if any, and auto-play it.
    selAnim_         = animBanks_.ForObjectName(selObj_->name);
    animatePreview_  = (selAnim_ != nullptr);

    // Seed the transform editor and reset sub-mesh visibility for this object.
    SyncTransformFromSelection();
    meshVisible_.assign(selObj_->meshes.size(), 1);

    // Point the emitter editor at this object (default placement = bbox centre).
    const AABB& b = selObj_->bbox;
    emitters_.SetSubject(selObj_->name,
                         0.5f * (b.min.x + b.max.x),
                         0.5f * (b.min.y + b.max.y),
                         0.5f * (b.min.z + b.max.z));
}

void ObjectExportPanel::UploadSelectedGeometry() {
    if (currentGeom_) {
        meshRenderer_.DeleteGeometry(currentGeom_);
        currentGeom_ = 0;
    }
    if (!selObj_) return;
    currentGeom_ = meshRenderer_.UploadGeometry(*selObj_);
    if (currentGeom_) {
        // Frame the camera on the object's local-space AABB.
        AABB localBounds;
        for (const auto& mesh : selObj_->meshes)
            localBounds.Expand(mesh.ComputeAABB());
        camera_.FrameAABB(localBounds);
    }
    previewDirty_ = false;
}

// ─── Camera input ─────────────────────────────────────────────────────────────

void ObjectExportPanel::UpdateCameraInput(const VanVec2& /*contentSize*/, bool hovered) {
    auto& inp = camInput_;
    inp = {};

    if (!hovered) return;

    VanGuiIO& io = VanGui::GetIO();

    inp.orbiting = io.MouseDown[2] || (io.MouseDown[0] && io.KeyAlt);
    inp.panning  = io.MouseDown[2] && io.KeyShift;
    if (inp.panning) inp.orbiting = false;

    if (inp.orbiting || inp.panning) {
        inp.deltaX = io.MouseDelta.x;
        inp.deltaY = io.MouseDelta.y;
    }

    inp.scroll      = io.MouseWheel;
    inp.viewportH   = VanGui::GetContentRegionAvail().y;

    if (io.MouseWheel != 0.0f && VanGui::IsItemHovered()) {
        // Zoom toward the mouse cursor (screen center for simplicity here).
        inp.hasZoomTarget = false;
    }
}

// ─── Export helpers ───────────────────────────────────────────────────────────

void ObjectExportPanel::RunExportObject(const SolidObject& obj,
                                         const StreamSection& sect,
                                         TaskQueue& queue) {
    // Build a default output path next to the loaded file.
    std::filesystem::path outDir;
    if (!loadedPath_.empty())
        outDir = std::filesystem::path(loadedPath_).parent_path() / "export";

    // Default export name = the object's own name; fall back to its name-hash so
    // unnamed objects each get a unique file instead of all overwriting "object".
    std::string objName = obj.name;
    if (objName.empty()) {
        char hashName[24];
        std::snprintf(hashName, sizeof(hashName), "object_%08X", obj.nameHash);
        objName = hashName;
    }
    const std::filesystem::path outPath =
        outDir / (objName + (exportFmt_ == ExportFormat::OBJ ? ".obj" : ".glb"));

    Exporter::ExportOptions opts;
    opts.applyWorldTransform = exportApplyXform_;
    opts.exportTextures      = exportTextures_;
    opts.textureSubdir       = exportTexSubdir_;

    const SolidObject* objPtr   = &obj;
    const StreamSection* secPtr = &sect;
    const std::vector<TPKBlock>* extraPtr = &extraTPKs_;

    const ExportFormat fmt = exportFmt_;

    queue.Submit("Exporting " + objName,
        [objPtr, secPtr, extraPtr, opts, outPath, fmt, this](ProgressState& p) {
            p.fraction = -1.0f;
            Result<Exporter::ExportReport> r = Result<Exporter::ExportReport>::Err("not started");

            if (fmt == ExportFormat::OBJ) {
                r = Exporter::ExportObjectOBJ(*objPtr, *secPtr, outPath, opts,
                                              extraPtr->empty() ? nullptr : extraPtr);
            } else {
                r = Exporter::ExportObjectGLTF(*objPtr, *secPtr,
                                         std::filesystem::path(outPath).replace_extension(".glb"),
                                         opts, extraPtr->empty() ? nullptr : extraPtr);
            }
            p.fraction = 1.0f;

            if (r) {
                char buf[256];
                std::snprintf(buf, sizeof(buf),
                    "Exported '%s': %zu verts, %zu tris, %zu textures",
                    outPath.filename().string().c_str(),
                    r.value.verticesWritten, r.value.trianglesWritten,
                    r.value.texturesWritten);
                lastExportStatus_ = buf;
            } else {
                lastExportStatus_ = "Export failed: " + r.error;
                LOG_WARN("ObjectExportPanel: {}", lastExportStatus_);
            }
        });
}

void ObjectExportPanel::RunExportSection(const StreamSection& sect, TaskQueue& queue) {
    std::filesystem::path outDir;
    if (!loadedPath_.empty())
        outDir = std::filesystem::path(loadedPath_).parent_path() / "export";

    const std::string secName = sect.name.empty() ? "section" : sect.name;
    const ExportFormat fmt = exportFmt_;
    const std::filesystem::path outPath =
        outDir / (secName + (fmt == ExportFormat::OBJ ? ".obj" : ".glb"));

    Exporter::ExportOptions opts;
    opts.applyWorldTransform = exportApplyXform_;
    opts.exportTextures      = exportTextures_;
    opts.textureSubdir       = exportTexSubdir_;

    const StreamSection* secPtr  = &sect;
    const std::vector<TPKBlock>* extraPtr = &extraTPKs_;

    queue.Submit("Exporting section " + secName,
        [secPtr, extraPtr, opts, outPath, fmt, this](ProgressState& p) {
            p.fraction = -1.0f;
            auto r = (fmt == ExportFormat::OBJ)
                ? Exporter::ExportSectionOBJ(*secPtr, outPath, opts,
                                             extraPtr->empty() ? nullptr : extraPtr)
                : Exporter::ExportSectionGLTF(*secPtr, outPath, opts,
                                              extraPtr->empty() ? nullptr : extraPtr);
            p.fraction = 1.0f;
            if (r) {
                char buf[256];
                std::snprintf(buf, sizeof(buf),
                    "Section export: %zu objects, %zu verts, %zu tris",
                    r.value.objectsExported, r.value.verticesWritten,
                    r.value.trianglesWritten);
                lastExportStatus_ = buf;
            } else {
                lastExportStatus_ = "Export failed: " + r.error;
            }
        });
}

// ─── Import / replace ──────────────────────────────────────────────────────────

void ObjectExportPanel::ImportReplaceSelected(const std::filesystem::path& src,
                                              TaskQueue& queue) {
    if (!selObj_ || loadedPath_.empty()) {
        lastExportStatus_ = "Select an object first, then import to replace it.";
        return;
    }
    const uint32_t    hash    = selObj_->nameHash;
    const std::string objName = selObj_->name;
    const std::string file    = loadedPath_;
    importInProgress_ = true;
    lastExportStatus_ = "Replacing '" + objName + "'\xe2\x80\xa6";

    // The worker thread must not touch panel members (they are read every frame by
    // the UI thread). It writes its result into a shared struct; the UI-thread
    // onDone callback applies it. Previously the worker wrote lastExportStatus_,
    // importInProgress_, reloadRequested_, reselectHash_ and reloadPath_ directly
    // — a data race that could leave the panel stuck "Updating object…" or reload
    // with a half-written path, so the replaced object never appeared.
    struct ReplaceOutcome {
        bool        ok = false;
        std::string status;
        std::string reloadPath;
        uint32_t    reselectHash = 0;
    };
    auto out = std::make_shared<ReplaceOutcome>();

    queue.Submit("Importing " + src.filename().string(),
        [out, src, file, hash, objName, this](ProgressState& p) {
            p.fraction = -1.0f;
            p.SetDetail("Reading " + src.filename().string());
            auto model = MeshImporter::Import(src);
            if (!model) {
                out->status = "Import failed: " + model.error;
                p.fraction = 1.0f; return;
            }
            p.SetDetail("Rebuilding object geometry\xe2\x80\xa6");
            auto r = SolidWriter::ReplaceObject(file, hash, model.value, backup_);
            p.fraction = 1.0f;
            if (!r) { out->status = "Replace failed: " + r.error; return; }

            char buf[320];
            std::snprintf(buf, sizeof(buf),
                "Replaced '%s': %zu meshes, %zu verts, %zu tris (backup: %s.bak)",
                objName.c_str(), model.value.meshes.size(),
                model.value.TotalVertices(), model.value.TotalTriangles(),
                file.c_str());
            out->ok           = true;
            out->status       = buf;
            out->reloadPath   = file;
            out->reselectHash = hash;
        },
        [out, this]() {                       // runs on the UI thread
            lastExportStatus_ = out->status;
            if (out->ok) {
                reloadPath_      = out->reloadPath;
                reselectHash_    = out->reselectHash;  // re-select after reload
                reloadRequested_ = true;               // Draw() reloads on UI thread
            } else {
                importInProgress_ = false;             // failed: stop the spinner
            }
        });
}

// ─── Draw sub-panels ─────────────────────────────────────────────────────────

void ObjectExportPanel::DrawObjectTree() {
    // Search box (mirrors the Textures panel filter).
    objFilter_.Draw("##objfilter", "Search objects\xe2\x80\xa6");
    VanGui::Separator();

    (void)VanGui::BeginChild("##OEPTree", VanVec2(0.0f, 0.0f), true,
                      VanGuiWindowFlags_HorizontalScrollbar);

    if (sections_.empty()) {
        VanGui::TextDisabled("No file loaded.\nUse File > Open BUN/BIN...");
        VanGui::EndChild();
        return;
    }

    const bool filtering = !objFilter_.IsEmpty();

    for (int si = 0; si < (int)sections_.size(); ++si) {
        const StreamSection& sec = sections_[si];
        VanGui::PushID((const void*)&sec);   // pointer id: unique & collision-proof

        const std::string secLabel = sec.name.empty()
            ? ("Section " + std::to_string(si))
            : sec.name;

        VanGuiTreeNodeFlags secFlags = VanGuiTreeNodeFlags_DefaultOpen |
                                      VanGuiTreeNodeFlags_OpenOnArrow;
        bool secOpen = VanGui::TreeNodeEx("##sec", secFlags, "%s", secLabel.c_str());
        VanGui::PopID();
        if (!secOpen) continue;

        for (int li = 0; li < (int)sec.solidLists.size(); ++li) {
            const SolidList& sl = sec.solidLists[li];
            VanGui::PushID((const void*)&sl);

            const std::string slLabel = sl.sectionName.empty()
                ? ("SolidList " + std::to_string(li))
                : sl.sectionName;

            VanGuiTreeNodeFlags slFlags = VanGuiTreeNodeFlags_OpenOnArrow;
            if (filtering) slFlags |= VanGuiTreeNodeFlags_DefaultOpen;
            bool slOpen = VanGui::TreeNodeEx("##sl", slFlags, "%s  (%zu obj)",
                                            slLabel.c_str(), sl.objects.size());
            VanGui::PopID();
            if (!slOpen) continue;

            for (int oi = 0; oi < (int)sl.objects.size(); ++oi) {
                const SolidObject& obj = sl.objects[oi];

                // Apply the search filter (match on name or hex hash).
                if (filtering) {
                    char hashStr[12];
                    std::snprintf(hashStr, sizeof(hashStr), "%08X", obj.nameHash);
                    if (!objFilter_.Matches(obj.name) && !objFilter_.Matches(hashStr))
                        continue;
                }

                VanGui::PushID((const void*)&obj);
                const bool isSelected  = (si == selectedSection_ &&
                                          li == selectedList_ &&
                                          oi == selectedObject_);

                VanGuiTreeNodeFlags objFlags =
                    VanGuiTreeNodeFlags_Leaf |
                    VanGuiTreeNodeFlags_NoTreePushOnOpen |
                    (isSelected ? VanGuiTreeNodeFlags_Selected : 0);

                char label[128];
                std::snprintf(label, sizeof(label), "%s  (%zu tri)",
                              obj.name.empty() ? "<unnamed>" : obj.name.c_str(),
                              obj.TriCount());
                (void)VanGui::TreeNodeEx("##obj", objFlags, "%s", label);

                if (VanGui::IsItemClicked())
                    SelectObject(si, li, oi);

                if (VanGui::IsItemHovered()) {
                    (void)VanGui::BeginTooltip();
                    VanGui::Text("Vertices: %zu", obj.VertexCount());
                    VanGui::Text("Triangles: %zu", obj.TriCount());
                    VanGui::Text("Mesh groups: %zu", obj.meshes.size());
                    VanGui::Text("Textures: %zu", obj.textureHashes.size());
                    VanGui::Text("Hash: 0x%08X", obj.nameHash);
                    VanGui::EndTooltip();
                }
                VanGui::PopID();
            }
            VanGui::TreePop();
        }
        VanGui::TreePop();
    }
    VanGui::EndChild();
}

void ObjectExportPanel::DrawPreviewViewport() {
    // Determine viewport size (cap at 512 for performance).
    VanVec2 avail = VanGui::GetContentRegionAvail();
    const int vW = std::min((int)avail.x, 512);
    const int vH = std::min((int)avail.y, 512);

    const bool hovered = VanGui::IsWindowHovered();
    UpdateCameraInput(avail, hovered);
    camera_.Update(camInput_);

    if (previewDirty_) UploadSelectedGeometry();

    // While a replace + reload is in flight, show a spinner so it's clear the
    // viewer is updating itself (it re-selects the object automatically after).
    if (importInProgress_) {
        const double t   = VanGui::GetTime();
        const int    dot = (int)(t * 4.0) % 4;
        char spin[40];
        std::snprintf(spin, sizeof(spin), "Updating object%.*s", dot, "...");
        VanGui::Spacing();
        VanGui::TextColored(VanVec4(0.98f, 0.55f, 0.23f, 1.0f), "%s", spin);
        VanGui::TextDisabled("Rebuilding geometry and reloading the file\xe2\x80\xa6");
        return;
    }

    if (!shadersOk_) {
        VanGui::TextColored(VanVec4(1,0.4f,0.4f,1), "Preview unavailable (shader error)");
        return;
    }
    if (!selObj_) {
        VanGui::TextDisabled("Select an object in the tree to preview.");
        return;
    }
    if (!currentGeom_) {
        VanGui::TextDisabled("Object has no uploadable mesh data.");
        return;
    }
    if (!EnsureFBO(vW, vH)) {
        VanGui::TextColored(VanVec4(1,0.4f,0.4f,1), "Preview unavailable (FBO error)");
        return;
    }

    // ── Render into the FBO ──────────────────────────────────────────────────
    GLint prevFbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
    GLint prevVP[4] = {};
    glGetIntegerv(GL_VIEWPORT, prevVP);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, vW, vH);
    glClearColor(0.13f, 0.14f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    const glm::mat4 view = camera_.View();
    const glm::mat4 proj = camera_.Proj(static_cast<float>(vW) /
                                        static_cast<float>(std::max(vH, 1)));

    shaderPhong_.Bind();
    shaderPhong_.SetVec3("uLightDir",    glm::normalize(glm::vec3(0.4f, 0.3f, -0.8f)));
    shaderPhong_.SetVec3("uLightColor",  glm::vec3(0.9f, 0.87f, 0.8f));
    shaderPhong_.SetVec3("uAmbient",     glm::vec3(0.48f, 0.48f, 0.52f));
    shaderPhong_.SetBool("uSelected",    false);
    shaderPhong_.SetFloat("uFogDensity", 0.0f);
    shaderPhong_.SetInt("uTex",          0);

    const glm::mat4 previewModel = PreviewModelMatrix();
    meshRenderer_.RenderGeometryImmediate(currentGeom_, shaderPhong_, view, proj,
                                          &texManager_, previewModel, &meshVisible_);

    if (showWireframe_) {
        shaderWire_.Bind();
        meshRenderer_.RenderGeometryWireframe(currentGeom_, shaderWire_, view, proj,
                                              previewModel);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
    glViewport(prevVP[0], prevVP[1], prevVP[2], prevVP[3]);

    // ── Display in VanGui ─────────────────────────────────────────────────────
    // VanGui texture IDs: cast from GLuint.
    VanTextureID texId = static_cast<VanTextureID>(static_cast<uintptr_t>(fboColorTex_));
    VanGui::Image(texId, VanVec2((float)vW, (float)vH),
                 VanVec2(0, 1), VanVec2(1, 0));  // flip Y (GL vs VanGui convention)

    // Overlay: stats
    const VanVec2 wPos = VanGui::GetItemRectMin();
    VanDrawList* dl = VanGui::GetWindowDrawList();
    char stats[128];
    std::snprintf(stats, sizeof(stats), "%s\n%zu verts  %zu tris  %zu groups",
                  selObj_->name.c_str(), selObj_->VertexCount(),
                  selObj_->TriCount(), selObj_->meshes.size());
    dl->AddText(VanVec2(wPos.x + 6.0f, wPos.y + 6.0f),
                VAN_COL32(255, 255, 255, 200), stats);

    // Emitter markers + live looping particles (shared component).
    emitters_.DrawOverlay(dl, proj * view * previewModel,
                          wPos.x, wPos.y, (float)vW, (float)vH);
}

// ─── Object properties (smackable / sway / wind / flags) ───────────────────────

glm::mat4 ObjectExportPanel::PreviewModelMatrix() const {
    if (!selObj_) return glm::mat4(1.0f);
    const float t = static_cast<float>(VanGui::GetTime());

    // ── Transform edit preview: rotation/scale delta about the object centre ──
    // Position is world placement (not shown in the framed local preview), so we
    // preview the delta of rotation & scale relative to the object's authored
    // transform, letting the user see the edit live.
    if (xformActive_) {
        const AABB& bb = selObj_->bbox;
        const glm::vec3 c(0.5f * (bb.min.x + bb.max.x),
                          0.5f * (bb.min.y + bb.max.y),
                          0.5f * (bb.min.z + bb.max.z));
        const glm::vec3 dEuler = glm::radians(glm::vec3(
            editRot_[0] - origRot_[0], editRot_[1] - origRot_[1], editRot_[2] - origRot_[2]));
        const glm::vec3 dScale(
            origScale_[0] != 0.f ? editScale_[0] / origScale_[0] : 1.f,
            origScale_[1] != 0.f ? editScale_[1] / origScale_[1] : 1.f,
            origScale_[2] != 0.f ? editScale_[2] / origScale_[2] : 1.f);
        glm::mat4 m(1.0f);
        m = glm::translate(m, c);
        m = m * glm::rotate(glm::mat4(1.f), dEuler.z, glm::vec3(0, 0, 1))
              * glm::rotate(glm::mat4(1.f), dEuler.y, glm::vec3(0, 1, 0))
              * glm::rotate(glm::mat4(1.f), dEuler.x, glm::vec3(1, 0, 0));
        m = glm::scale(m, dScale);
        m = glm::translate(m, -c);
        return m;
    }

    // ── World-animation bank motion (cranes / ships / blimp / airliners) ──────
    // Play the object's bank on the channels it actually declares: _t drifts it
    // along, _q yaws it, _s pulses its scale. Amplitudes are sized to the object
    // so the motion reads at any prop scale.
    if (animatePreview_ && selAnim_) {
        const AABB& b = selObj_->bbox;
        const glm::vec3 c(0.5f * (b.min.x + b.max.x),
                          0.5f * (b.min.y + b.max.y),
                          0.5f * (b.min.z + b.max.z));
        const glm::vec3 raw = b.max - b.min;
        const glm::vec3 ext(std::max(raw.x, 1.0f),
                            std::max(raw.y, 1.0f),
                            std::max(raw.z, 1.0f));
        glm::mat4 m(1.0f);
        if (selAnim_->hasTranslation) {
            const glm::vec3 d(0.10f * ext.x * std::sin(t * 0.5f),
                              0.06f * ext.y * std::sin(t * 0.37f + 1.3f),
                              0.02f * ext.z * std::sin(t * 0.9f));
            m = glm::translate(m, d);
        }
        if (selAnim_->hasRotation) {
            m = glm::translate(m, c);
            m = glm::rotate(m, 0.12f * std::sin(t * 0.6f), glm::vec3(0, 0, 1));
            m = glm::translate(m, -c);
        }
        if (selAnim_->hasScale) {
            const float s = 1.0f + 0.03f * std::sin(t * 1.4f);
            m = glm::translate(m, c);
            m = glm::scale(m, glm::vec3(s));
            m = glm::translate(m, -c);
        }
        return m;
    }

    // ── Foliage wind sway ─────────────────────────────────────────────────────
    if (!swayPreview_) return glm::mat4(1.0f);
    // Bend the object about its base (local-bbox centre in X/Y, min Z) — the same
    // per-object wind sway the map editor's PLAY mode applies to foliage. World
    // is Z-up, so the trunk runs along +Z and the bend tips it sideways.
    const AABB& b = selObj_->bbox;
    const glm::vec3 base(0.5f * (b.min.x + b.max.x),
                         0.5f * (b.min.y + b.max.y),
                         b.min.z);
    const float phase = static_cast<float>(selObj_->nameHash & 0xFFFFu) * 0.001f;
    const float ang   = 0.045f * std::sin(t * 1.6f + phase) +
                        0.015f * std::sin(t * 3.1f + phase * 1.7f);

    glm::mat4 m(1.0f);
    m = glm::translate(m, base);
    m = glm::rotate(m, ang, glm::vec3(0.8f, 0.2f, 0.0f));
    m = glm::translate(m, -base);
    return m;
}

void ObjectExportPanel::DrawObjectProperties(TaskQueue& queue) {
    VanGui::SeparatorText("Object Properties");

    // Bundle-level animation-bank inventory (visible regardless of selection).
    if (!animBanks_.empty()) {
        if (VanGui::TreeNode("##animbanks", "Animation banks in this bundle: %zu",
                             animBanks_.objects.size())) {
            for (const auto& ao : animBanks_.objects) {
                VanGui::BulletText("%s  (%s%s%s)", ao.object.c_str(),
                                   ao.hasTranslation ? "t" : "",
                                   ao.hasRotation ? "q" : "",
                                   ao.hasScale ? "s" : "");
            }
            VanGui::TreePop();
        }
    }

    if (!selObj_) {
        VanGui::TextDisabled("Select an object to inspect its smackable / sway /\n"
                             "wind classification and edit its properties.");
        return;
    }

    const ObjectClassification cls = ClassifyObject(selObj_->name);

    // ── Classification (name-derived world semantics) ─────────────────────────
    VanGui::Text("Class:");
    auto tag = [](bool on, VanVec4 col, const char* txt) {
        if (on) VanGui::TextColored(col, "    - %s", txt);
    };
    if (cls.IsPlain()) {
        VanGui::TextDisabled("    - plain geometry (no special world behaviour)");
    } else {
        tag(cls.swayable,       VanVec4(0.55f, 0.85f, 0.55f, 1.0f), "sway / wind (foliage)");
        tag(cls.smackable,      VanVec4(0.95f, 0.65f, 0.35f, 1.0f), "smackable (destructible)");
        tag(cls.pursuitBreaker, VanVec4(0.95f, 0.45f, 0.45f, 1.0f), "pursuit-breaker set-piece");
        tag(cls.panoramaLOD,    VanVec4(0.60f, 0.70f, 0.90f, 1.0f), "panorama / LOD stand-in");
        tag(cls.proxyGeometry,  VanVec4(0.70f, 0.70f, 0.70f, 1.0f), "reflection / shadow proxy");
        tag(cls.eventOnly,      VanVec4(0.85f, 0.75f, 0.50f, 1.0f), "race / cutscene-only prop");
    }
    if (cls.smackable) {
        VanGui::BulletText("Break role: %s", SmackableRoleName(cls.role));
        if (cls.setPiece)
            VanGui::BulletText("Set-piece: %s", cls.setPiece);
    }

    // ── Attributes (add smackable / sway / wind / animated, then save) ────────
    // Toggle a named attribute to add it to the object; untick to remove it.
    // Any bits not shown here are preserved untouched on save.
    VanGui::Separator();
    VanGui::Text("Attributes");
    VanGui::SameLine();
    VanGui::TextDisabled("(tick to add, then Save)");

    auto markDirty = [&]() { flagsDirty_ = (pendingFlags_ != selObj_->flags); };

    struct ObjAttr { uint32_t mask; const char* label; const char* help; bool primary; };
    static const ObjAttr kAttrs[] = {
        { 0x40000000, "Smackable (destructible)",
          "Marks the object as a breakable/destructible prop.", true },
        { 0x00200000, "Sway / wind (foliage)",
          "Marks the object as foliage that bends in the wind.", true },
        { 0x00000010, "Animated",
          "Marks the object as animated (driven by an animation bank).", true },
        { 0x00100000, "Structure",          "Structural object class.",        false },
        { 0x00400000, "Building",            "Building object class.",          false },
        { 0x08000000, "Wall",                "Wall object class.",              false },
        { 0x02000000, "Street furniture",    "Street-furniture object class.",  false },
        { 0x00040000, "Road / path",         "Road / path surface class.",      false },
        { 0x00008000, "Active / placed",     "Active, placed-in-world object.", false },
    };

    auto attrToggle = [&](const ObjAttr& a) {
        bool on = (pendingFlags_ & a.mask) == a.mask;
        VanGui::PushID(static_cast<int>(a.mask));
        if (VanGui::Checkbox(a.label, &on)) {
            pendingFlags_ = on ? (pendingFlags_ | a.mask) : (pendingFlags_ & ~a.mask);
            markDirty();
        }
        VanGui::SameLine();
        VanGui::TextDisabled("(?)");
        if (VanGui::IsItemHovered()) VanGui::SetTooltip("%s", a.help);
        VanGui::PopID();
    };

    for (const auto& a : kAttrs) if (a.primary) attrToggle(a);
    if (VanGui::TreeNode("More attributes"))  {
        for (const auto& a : kAttrs) if (!a.primary) attrToggle(a);
        VanGui::TreePop();
    }

    // Save / revert
    const bool canSave = flagsDirty_ && !loadedPath_.empty() && !savingFlags_;
    if (!canSave) VanGui::BeginDisabled();
    if (VanGui::Button(savingFlags_ ? "Saving\xe2\x80\xa6##saveflags"
                                    : "Save properties to file##saveflags",
                       VanVec2(-1.0f, 0.0f)))
        SaveFlagsToFile(queue);
    if (!canSave) VanGui::EndDisabled();

    if (flagsDirty_) {
        VanGui::TextColored(VanVec4(0.98f, 0.75f, 0.30f, 1.0f), "Unsaved changes");
        VanGui::SameLine();
        if (VanGui::SmallButton("Revert##flags")) {
            pendingFlags_ = selObj_->flags;
            flagsDirty_   = false;
        }
    }
    VanGui::TextDisabled("Edits are written in place; a .bak backup is kept.");

    // ── Transform + sub-mesh editing (production-grade depth) ─────────────────
    DrawTransformEditor(queue);
    DrawSubMeshInspector();

    // ── Wind-sway preview ─────────────────────────────────────────────────────
    VanGui::Separator();
    (void)VanGui::Checkbox("Animate wind sway (preview)", &swayPreview_);
    VanGui::SameLine();
    VanGui::TextDisabled("(?)");
    if (VanGui::IsItemHovered())
        VanGui::SetTooltip(
            "Bends the object about its base like the game's foliage wind\n"
            "(the same motion the map editor's PLAY mode animates). Auto-on for\n"
            "trees/bushes. Preview only \xe2\x80\x94 not written to the file.");

    // ── World-animation bank (cranes / ships / blimp / airliners) ─────────────
    if (selAnim_) {
        VanGui::Separator();
        VanGui::TextColored(VanVec4(0.55f, 0.80f, 0.95f, 1.0f),
                            "World-animation bank");
        VanGui::BulletText("Drives object: %s", selAnim_->object.c_str());
        std::string chans;
        if (selAnim_->hasTranslation) chans += "translation  ";
        if (selAnim_->hasRotation)    chans += "rotation  ";
        if (selAnim_->hasScale)       chans += "scale";
        VanGui::BulletText("Channels: %s", chans.empty() ? "-" : chans.c_str());
        VanGui::BulletText("Clips: %d", selAnim_->clipCount);
        (void)VanGui::Checkbox("Play animation (preview)", &animatePreview_);
    }

    // ── Texture reassignment (no export/import needed) ────────────────────────
    DrawTextureEditor(queue);

    // ── Emitter / FX attachments (shared component) ───────────────────────────
    emitters_.DrawPanel();

    if (!lastFlagStatus_.empty()) {
        VanGui::Separator();
        VanGui::TextWrapped("%s", lastFlagStatus_.c_str());
    }
}

void ObjectExportPanel::SaveFlagsToFile(TaskQueue& queue) {
    if (!selObj_ || loadedPath_.empty()) {
        lastFlagStatus_ = "Select an object first, then edit and save its flags.";
        return;
    }
    if (!flagsDirty_) return;

    const uint32_t    hash     = selObj_->nameHash;
    const uint32_t    oldFlags = selObj_->flags;
    const uint32_t    newFlags = pendingFlags_;
    const std::string objName  = selObj_->name;
    const std::string file     = loadedPath_;
    const int si = selectedSection_, li = selectedList_, oi = selectedObject_;

    savingFlags_    = true;
    lastFlagStatus_ = "Saving flags\xe2\x80\xa6";

    struct FlagOutcome { bool ok = false; std::string status; uint32_t applied = 0; };
    auto out = std::make_shared<FlagOutcome>();
    out->applied = newFlags;

    queue.Submit("Saving flags for " + (objName.empty() ? std::string("object") : objName),
        [out, file, hash, oldFlags, newFlags, objName, this](ProgressState& p) {
            p.fraction = -1.0f;
            auto r = SolidWriter::PatchObjectFlags(file, hash, newFlags, backup_, &oldFlags);
            p.fraction = 1.0f;
            if (!r) { out->status = "Save failed: " + r.error; return; }
            char buf[320];
            std::snprintf(buf, sizeof buf,
                "Saved flags 0x%08X \xe2\x86\x92 0x%08X for '%s' (backup: %s.bak)",
                oldFlags, newFlags, objName.c_str(), file.c_str());
            out->ok     = true;
            out->status = buf;
        },
        [out, si, li, oi, this]() {                // runs on the UI thread
            savingFlags_    = false;
            lastFlagStatus_ = out->status;
            if (!out->ok) return;
            // Reflect the new value in memory so 'dirty' clears without a reload.
            if (si >= 0 && si < (int)sections_.size()) {
                auto& lists = sections_[si].solidLists;
                if (li >= 0 && li < (int)lists.size()) {
                    auto& objs = lists[li].objects;
                    if (oi >= 0 && oi < (int)objs.size())
                        objs[oi].flags = out->applied;
                }
            }
            if (si == selectedSection_ && li == selectedList_ && oi == selectedObject_) {
                pendingFlags_ = out->applied;
                flagsDirty_   = false;
            }
        });
}

// ─── Texture editor (reassign an object's texture slot, no re-export) ──────────

void ObjectExportPanel::RebuildAvailableTextures() {
    availTextures_.clear();
    std::unordered_set<uint32_t> seen;
    auto addPack = [&](const TPKBlock& tp) {
        for (const auto& t : tp.textures) {
            if (!t.nameHash || !seen.insert(t.nameHash).second) continue;
            availTextures_.emplace_back(t.nameHash,
                t.name.empty() ? std::string() : t.name);
        }
    };
    for (const auto& s : sections_)
        for (const auto& tp : s.texturePacks) addPack(tp);
    for (const auto& tp : extraTPKs_) addPack(tp);

    std::sort(availTextures_.begin(), availTextures_.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
}

void ObjectExportPanel::DrawTextureEditor(TaskQueue& queue) {
    if (!selObj_) return;
    VanGui::Separator();
    VanGui::Text("Textures");
    VanGui::SameLine();
    VanGui::TextDisabled("(swap without export/import)");

    if (selObj_->textureHashes.empty()) {
        VanGui::TextDisabled("    this object references no texture slots.");
        return;
    }

    auto& res = HashResolver::Instance();
    for (int slot = 0; slot < (int)selObj_->textureHashes.size(); ++slot) {
        const uint32_t h = selObj_->textureHashes[slot];
        const std::string* nm = res.TryResolve(h);
        VanGui::PushID(slot);
        VanGui::BulletText("Slot %d: %s", slot, nm ? nm->c_str() : "(unnamed texture)");
        VanGui::SameLine();
        if (VanGui::SmallButton(texEditSlot_ == slot ? "cancel" : "change")) {
            texEditSlot_ = (texEditSlot_ == slot) ? -1 : slot;
            texNewHash_  = 0;
            lastTexStatus_.clear();
        }

        if (texEditSlot_ == slot) {
            const std::string* pickNm = texNewHash_ ? res.TryResolve(texNewHash_) : nullptr;
            const std::string preview = texNewHash_
                ? (pickNm ? *pickNm : std::string("(selected)"))
                : std::string("choose replacement\xe2\x80\xa6");
            VanGui::SetNextItemWidth(-1.0f);
            if (VanGui::BeginCombo("##texpick", preview.c_str())) {
                for (const auto& [th, tn] : availTextures_) {
                    if (tn.empty()) continue;                 // only offer named textures
                    const bool sel = (th == texNewHash_);
                    if (VanGui::Selectable(tn.c_str(), sel)) texNewHash_ = th;
                    if (sel) VanGui::SetItemDefaultFocus();
                }
                VanGui::EndCombo();
            }
            const bool canApply = texNewHash_ && texNewHash_ != h &&
                                  !loadedPath_.empty() && !retexBusy_;
            if (!canApply) VanGui::BeginDisabled();
            if (VanGui::Button(retexBusy_ ? "Applying\xe2\x80\xa6##applytex"
                                          : "Apply texture##applytex",
                               VanVec2(-1.0f, 0.0f)))
                ApplyTextureSwap(queue);
            if (!canApply) VanGui::EndDisabled();
        }
        VanGui::PopID();
    }

    if (availTextures_.empty())
        VanGui::TextDisabled("    (no texture packs loaded to pick from)");
    if (!lastTexStatus_.empty())
        VanGui::TextWrapped("%s", lastTexStatus_.c_str());
}

void ObjectExportPanel::ApplyTextureSwap(TaskQueue& queue) {
    if (!selObj_ || loadedPath_.empty() || texEditSlot_ < 0) return;
    if (texEditSlot_ >= (int)selObj_->textureHashes.size()) return;

    const uint32_t    hash    = selObj_->nameHash;
    const uint32_t    slot    = (uint32_t)texEditSlot_;
    const uint32_t    oldTex  = selObj_->textureHashes[texEditSlot_];
    const uint32_t    newTex  = texNewHash_;
    const std::string objName = selObj_->name;
    const std::string file    = loadedPath_;
    const int si = selectedSection_, li = selectedList_, oi = selectedObject_;
    const int editedSlot = texEditSlot_;

    retexBusy_     = true;
    lastTexStatus_ = "Applying texture\xe2\x80\xa6";

    struct TexOutcome { bool ok = false; std::string status; uint32_t applied = 0; };
    auto out = std::make_shared<TexOutcome>();
    out->applied = newTex;

    queue.Submit("Retexturing " + (objName.empty() ? std::string("object") : objName),
        [out, file, hash, slot, oldTex, newTex, objName, this](ProgressState& p) {
            p.fraction = -1.0f;
            auto r = SolidWriter::PatchObjectTexture(file, hash, slot, newTex, backup_, &oldTex);
            p.fraction = 1.0f;
            if (!r) { out->status = "Texture swap failed: " + r.error; return; }
            char buf[320];
            std::snprintf(buf, sizeof buf,
                "Retextured '%s' slot %u (backup: %s.bak)",
                objName.c_str(), slot, file.c_str());
            out->ok     = true;
            out->status = buf;
        },
        [out, si, li, oi, editedSlot, this]() {          // UI thread
            retexBusy_     = false;
            lastTexStatus_ = out->status;
            if (!out->ok) return;
            // Reflect the swap in memory (and re-upload so the preview updates).
            if (si >= 0 && si < (int)sections_.size()) {
                auto& lists = sections_[si].solidLists;
                if (li >= 0 && li < (int)lists.size()) {
                    auto& objs = lists[li].objects;
                    if (oi >= 0 && oi < (int)objs.size() &&
                        editedSlot < (int)objs[oi].textureHashes.size()) {
                        objs[oi].textureHashes[editedSlot] = out->applied;
                    }
                }
            }
            if (si == selectedSection_ && li == selectedList_ && oi == selectedObject_) {
                texEditSlot_ = -1;
                texNewHash_  = 0;
                previewDirty_ = true;   // re-upload to reflect the new texture
            }
        });
}

// ─── Transform editor (move / rotate / scale the whole object) ──────────────────

namespace {

void DecomposeMat4(const glm::mat4& m, float pos[3], float rotDeg[3], float scale[3]) {
    const glm::vec3 c0(m[0]), c1(m[1]), c2(m[2]);
    scale[0] = glm::length(c0); scale[1] = glm::length(c1); scale[2] = glm::length(c2);
    glm::mat3 r(c0 / (scale[0] > 1e-6f ? scale[0] : 1.f),
                c1 / (scale[1] > 1e-6f ? scale[1] : 1.f),
                c2 / (scale[2] > 1e-6f ? scale[2] : 1.f));
    // Euler XYZ (matches SolidObject::Compose order used on write).
    float sy = -r[0][2];
    sy = glm::clamp(sy, -1.f, 1.f);
    float ry = std::asin(sy), rx, rz;
    if (std::abs(sy) < 0.99999f) {
        rx = std::atan2(r[1][2], r[2][2]);
        rz = std::atan2(r[0][1], r[0][0]);
    } else {
        rx = std::atan2(-r[2][1], r[1][1]);
        rz = 0.f;
    }
    glm::vec3 e = glm::degrees(glm::vec3(rx, ry, rz));
    rotDeg[0] = e.x; rotDeg[1] = e.y; rotDeg[2] = e.z;
    pos[0] = m[3][0]; pos[1] = m[3][1]; pos[2] = m[3][2];
}

glm::mat4 ComposeMat4(const float pos[3], const float rotDeg[3], const float scale[3]) {
    const glm::vec3 e = glm::radians(glm::vec3(rotDeg[0], rotDeg[1], rotDeg[2]));
    glm::mat4 m = glm::rotate(glm::mat4(1.f), e.z, glm::vec3(0, 0, 1)) *
                  glm::rotate(glm::mat4(1.f), e.y, glm::vec3(0, 1, 0)) *
                  glm::rotate(glm::mat4(1.f), e.x, glm::vec3(1, 0, 0));
    m[0] *= scale[0]; m[1] *= scale[1]; m[2] *= scale[2];
    m[3] = glm::vec4(pos[0], pos[1], pos[2], 1.f);
    return m;
}

} // namespace

void ObjectExportPanel::SyncTransformFromSelection() {
    if (!selObj_) return;
    DecomposeMat4(selObj_->transform, editPos_, editRot_, editScale_);
    for (int i = 0; i < 3; ++i) {
        origPos_[i]   = editPos_[i];
        origRot_[i]   = editRot_[i];
        origScale_[i] = editScale_[i];
    }
    xformActive_ = false;
    lastXformStatus_.clear();
}

void ObjectExportPanel::DrawTransformEditor(TaskQueue& queue) {
    if (!selObj_) return;
    VanGui::Separator();
    VanGui::Text("Transform");
    VanGui::SameLine();
    VanGui::TextDisabled("(move / rotate / scale)");

    bool changed = false;
    VanGui::SetNextItemWidth(240.f);
    if (VanGui::DragFloat3("position", editPos_, 0.05f))   changed = true;
    VanGui::SetNextItemWidth(240.f);
    if (VanGui::DragFloat3("rotation\xc2\xb0", editRot_, 0.5f)) changed = true;
    VanGui::SetNextItemWidth(240.f);
    if (VanGui::DragFloat3("scale", editScale_, 0.01f, 0.001f, 1000.f)) changed = true;
    if (changed) xformActive_ = true;

    auto dirty = [&]() {
        for (int i = 0; i < 3; ++i)
            if (editPos_[i]  != origPos_[i] || editRot_[i] != origRot_[i] ||
                editScale_[i] != origScale_[i]) return true;
        return false;
    };
    const bool isDirty = dirty();

    const bool canSave = isDirty && !loadedPath_.empty() && !xformSaving_;
    if (!canSave) VanGui::BeginDisabled();
    if (VanGui::Button(xformSaving_ ? "Saving\xe2\x80\xa6##xf" : "Apply transform##xf"))
        ApplyTransform(queue);
    if (!canSave) VanGui::EndDisabled();
    VanGui::SameLine();
    if (VanGui::SmallButton("Reset##xf")) SyncTransformFromSelection();
    VanGui::SameLine();
    (void)VanGui::Checkbox("Live preview##xf", &xformActive_);
    if (isDirty)
        VanGui::TextColored(VanVec4(0.98f, 0.75f, 0.30f, 1.f), "Unsaved transform");
    if (!lastXformStatus_.empty()) VanGui::TextDisabled("%s", lastXformStatus_.c_str());
}

void ObjectExportPanel::ApplyTransform(TaskQueue& queue) {
    if (!selObj_ || loadedPath_.empty()) return;
    const glm::mat4 m = ComposeMat4(editPos_, editRot_, editScale_);
    std::array<float, 16> data{};
    std::memcpy(data.data(), glm::value_ptr(m), 64);

    const uint32_t    hash    = selObj_->nameHash;
    const std::string file    = loadedPath_;
    const std::string objName = selObj_->name;
    const int si = selectedSection_, li = selectedList_, oi = selectedObject_;
    xformSaving_    = true;
    lastXformStatus_ = "Saving transform\xe2\x80\xa6";

    struct XfOutcome { bool ok = false; std::string status; std::array<float,16> m; };
    auto out = std::make_shared<XfOutcome>();
    out->m = data;

    queue.Submit("Saving transform for " + (objName.empty() ? std::string("object") : objName),
        [out, file, hash, this](ProgressState& p) {
            p.fraction = -1.0f;
            auto r = SolidWriter::PatchObjectTransform(file, hash, out->m.data(), backup_);
            p.fraction = 1.0f;
            if (!r) { out->status = "Save failed: " + r.error; return; }
            out->ok = true;
            out->status = "Transform saved (backup kept).";
        },
        [out, si, li, oi, this]() {
            xformSaving_ = false;
            lastXformStatus_ = out->status;
            if (!out->ok) return;
            glm::mat4 nm;
            std::memcpy(glm::value_ptr(nm), out->m.data(), 64);
            if (si >= 0 && si < (int)sections_.size()) {
                auto& lists = sections_[si].solidLists;
                if (li >= 0 && li < (int)lists.size()) {
                    auto& objs = lists[li].objects;
                    if (oi >= 0 && oi < (int)objs.size()) objs[oi].transform = nm;
                }
            }
            if (si == selectedSection_ && li == selectedList_ && oi == selectedObject_) {
                SyncTransformFromSelection();
            }
        });
}

// ─── Sub-mesh inspector ─────────────────────────────────────────────────────────

void ObjectExportPanel::DrawSubMeshInspector() {
    if (!selObj_) return;
    VanGui::Separator();
    VanGui::Text("Sub-meshes");
    VanGui::SameLine();
    VanGui::TextDisabled("(%zu)", selObj_->meshes.size());
    VanGui::SameLine();
    if (VanGui::SmallButton("Show all")) {
        meshVisible_.assign(selObj_->meshes.size(), 1);
    }

    if (meshVisible_.size() != selObj_->meshes.size())
        meshVisible_.assign(selObj_->meshes.size(), 1);

    auto& res = HashResolver::Instance();
    for (size_t i = 0; i < selObj_->meshes.size(); ++i) {
        const auto& m = selObj_->meshes[i];
        VanGui::PushID((int)i);
        bool vis = meshVisible_[i] != 0;
        if (VanGui::Checkbox("##vis", &vis)) meshVisible_[i] = vis ? 1 : 0;
        VanGui::SameLine();
        const std::string* tn = res.TryResolve(m.textureHash);
        const std::string mat = m.materialName.empty() ? "(no material)" : m.materialName;
        if (VanGui::TreeNodeEx("##sm", 0, "Mesh %zu  \xe2\x80\x94  %zu tris  [%s]",
                               i, m.indices.size() / 3, SimSurfaceName(m.surface))) {
            VanGui::BulletText("Material: %s", mat.c_str());
            VanGui::BulletText("Texture: %s", tn ? tn->c_str() : "(unnamed)");
            VanGui::BulletText("Vertices: %zu", m.vertices.size());
            if (VanGui::SmallButton("Isolate")) {
                meshVisible_.assign(selObj_->meshes.size(), 0);
                meshVisible_[i] = 1;
            }
            VanGui::TreePop();
        }
        VanGui::PopID();
    }
}

void ObjectExportPanel::DrawExportOptions(TaskQueue& queue) {
    VanGui::SeparatorText("Export Options");

    // Format
    VanGui::Text("Format:");
    VanGui::SameLine();
    if (VanGui::RadioButton("OBJ + MTL", exportFmt_ == ExportFormat::OBJ))
        exportFmt_ = ExportFormat::OBJ;
    VanGui::SameLine();
    if (VanGui::RadioButton("glTF/.glb", exportFmt_ == ExportFormat::GLTF))
        exportFmt_ = ExportFormat::GLTF;
    if (exportFmt_ == ExportFormat::GLTF)
        VanGui::TextDisabled("  Single .glb: mesh (= collision geometry) + embedded "
                            "textures; surface type tagged per material. Imports in "
                            "Blender / 3ds Max.");

    (void)VanGui::Checkbox("Apply world transform", &exportApplyXform_);
    VanGui::SameLine(); VanGui::TextDisabled("(?)");
    if (VanGui::IsItemHovered())
        VanGui::SetTooltip("Bakes SolidObject::transform (game placement) into vertex positions.\n"
                          "Disable to export in local object space.");

    (void)VanGui::Checkbox("Export textures alongside", &exportTextures_);
    if (exportTextures_) {
        VanGui::SameLine();
        VanGui::SetNextItemWidth(160.0f);
        (void)VanGui::InputText("##texsubdir", exportTexSubdir_, sizeof(exportTexSubdir_));
        VanGui::SameLine(); VanGui::TextDisabled("subdir");
    }

    VanGui::Separator();
    (void)VanGui::Checkbox("Wireframe overlay", &showWireframe_);

    VanGui::Separator();

    // Export buttons
    const bool hasObj  = selObj_ != nullptr;
    const bool hasSect = selSect_ != nullptr;

    if (!hasObj) VanGui::BeginDisabled();
    if (VanGui::Button("Export Selected Object##exp", VanVec2(-1.0f, 0.0f))) {
        if (hasObj && selSect_)
            RunExportObject(*selObj_, *selSect_, queue);
    }
    if (!hasObj) VanGui::EndDisabled();

    if (!hasSect) VanGui::BeginDisabled();
    if (VanGui::Button("Export Entire Section##expsec", VanVec2(-1.0f, 0.0f))) {
        if (hasSect)
            RunExportSection(*selSect_, queue);
    }
    if (!hasSect) VanGui::EndDisabled();

    if (!HasContent()) VanGui::BeginDisabled();
    if (VanGui::Button("Export All Sections##expall", VanVec2(-1.0f, 0.0f))) {
        for (const auto& s : sections_)
            RunExportSection(s, queue);
    }
    if (!HasContent()) VanGui::EndDisabled();

    // Last export status
    if (!lastExportStatus_.empty()) {
        VanGui::Separator();
        VanGui::TextWrapped("%s", lastExportStatus_.c_str());
    }
}

// ─── Main draw ────────────────────────────────────────────────────────────────

void ObjectExportPanel::Draw(TaskQueue& queue) {
    // A finished import wrote the file on disk; reload to reflect the new geometry.
    if (reloadRequested_) {
        reloadRequested_ = false;
        if (!reloadPath_.empty()) LoadFileAsync(reloadPath_, queue);
    }

    // Drain any pending async load first.
    PumpPending();

    // Three-pane layout: tree | preview | export options. Drag the right edge
    // of the tree/preview panes to resize (VanGuiChildFlags_ResizeX), same
    // pattern as the Cars panel. A BeginTable was tried here first, but a
    // scrollable child nested inside a table cell rendered its background
    // using the wrong (absolute, not column-relative) offset — visible as
    // content bleeding back over the nav sidebar — so plain side-by-side
    // resizable children are used instead.
    const float availW = VanGui::GetContentRegionAvail().x;
    const float totalH = VanGui::GetContentRegionAvail().y;
    const float spacing = VanGui::GetStyle().ItemSpacing.x;

    treeWidth_    = std::min(treeWidth_, std::max(kPaneWidthMin, availW - 2 * kPaneWidthMin - 2 * spacing));
    previewWidth_ = std::min(previewWidth_, std::max(kPaneWidthMin, availW - treeWidth_ - kPaneWidthMin - 2 * spacing));

    // ── Pane 0: Object tree ──────────────────────────────────────────────────
    (void)VanGui::BeginChild("##OEPTreePane", VanVec2(treeWidth_, totalH),
                       VanGuiChildFlags_ResizeX | VanGuiChildFlags_Borders);
    VanGui::Text("Objects");
    VanGui::Separator();
    DrawObjectTree();
    treeWidth_ = std::max(kPaneWidthMin, VanGui::GetWindowWidth());
    VanGui::EndChild();

    VanGui::SameLine();

    // ── Pane 1: Live preview ─────────────────────────────────────────────────
    (void)VanGui::BeginChild("##OEPPreviewPane", VanVec2(previewWidth_, totalH),
                       VanGuiChildFlags_ResizeX | VanGuiChildFlags_Borders);
    VanGui::Text("Preview  (MMB/Alt+LMB orbit  Scroll zoom)");
    VanGui::Separator();
    DrawPreviewViewport();
    previewWidth_ = std::max(kPaneWidthMin, VanGui::GetWindowWidth());
    VanGui::EndChild();

    VanGui::SameLine();

    // ── Pane 2: Export options (fills the remainder) ─────────────────────────
    const float exportW = availW - treeWidth_ - previewWidth_ - 2 * spacing;
    (void)VanGui::BeginChild("##OEPExportPane", VanVec2(exportW, totalH), VanGuiChildFlags_Borders);
    DrawObjectProperties(queue);
    VanGui::Spacing();
    DrawExportOptions(queue);
    VanGui::EndChild();
}

} // namespace nfsmw
