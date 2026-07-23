// ─── ui/CarViewerPanel.cpp ────────────────────────────────────────────────────
#include "ui/CarViewerPanel.h"
#include "ui/CarPanel.h"    // for CarContext
#include <vangui.h>
#include "renderer/GLCompat.h"

namespace nfsmw {

Result<void> CarViewerPanel::Init() {
    // Compile shaders (same source strings as ObjectExportPanel)
    constexpr const char* kPhongVert = R"glsl(
        #version 330 core
        layout(location=0) in vec3 aPos;
        layout(location=1) in vec3 aNormal;
        layout(location=2) in vec2 aUV;
        layout(location=3) in vec4 aColor;
        uniform mat4 uMVP;
        uniform mat4 uModel;
        out vec3 vNormal;
        out vec2 vUV;
        out vec4 vColor;
        void main() {
            vNormal = mat3(transpose(inverse(uModel))) * aNormal;
            vUV     = aUV;
            vColor  = aColor;
            gl_Position = uMVP * vec4(aPos, 1.0);
        }
    )glsl";
    constexpr const char* kPhongFrag = R"glsl(
        #version 330 core
        in vec3 vNormal;
        in vec2 vUV;
        in vec4 vColor;
        uniform sampler2D uTex;
        uniform bool uHasTex;
        out vec4 fragColor;
        void main() {
            vec3 light = normalize(vec3(0.6, 1.0, 0.8));
            float diff  = max(dot(normalize(vNormal), light), 0.0);
            vec4  base  = uHasTex ? texture(uTex, vUV) : vColor;
            fragColor   = vec4(base.rgb * (0.3 + 0.7 * diff), base.a);
        }
    )glsl";
    constexpr const char* kWireVert = R"glsl(
        #version 330 core
        layout(location=0) in vec3 aPos;
        uniform mat4 uMVP;
        void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
    )glsl";
    constexpr const char* kWireFrag = R"glsl(
        #version 330 core
        out vec4 fragColor;
        void main() { fragColor = vec4(1.0, 0.6, 0.0, 1.0); }
    )glsl";

    if (auto r = shaderPhong_.Build(kPhongVert, kPhongFrag); !r) return r;
    if (auto r = shaderWire_.Build(kWireVert, kWireFrag);   !r) return r;
    shadersOk_ = true;
    camera_.FrameAll(AABB{});
    return Result<void>::Ok();
}

void CarViewerPanel::Shutdown() {
    DeleteFBO();
    meshRenderer_.Clear();
    texManager_.Clear();
}

void CarViewerPanel::OnCarLoaded(const CarContext& ctx) {
    meshRenderer_.Clear();
    texManager_.Clear();
    currentGeom_ = 0;
    selectedPart_ = 0;
    previewDirty_ = true;

    // Upload textures from the body BUN
    for (const auto& tpk : ctx.bodyTPKs)
        for (auto tex : tpk.textures)   // copy: Upload takes non-const ref
            texManager_.Upload(tex);

    // Frame the camera on the body's combined bounding box so the part is
    // centered and visible immediately, rather than waiting for the user to
    // interact with the viewport first.
    if (!ctx.bodySection.solidLists.empty() &&
        !ctx.bodySection.solidLists[0].objects.empty()) {
        AABB combined;
        for (const auto& obj : ctx.bodySection.solidLists[0].objects)
            combined.Expand(obj.bbox);
        if (combined.Valid())
            camera_.FrameAll(combined);
    }

    // Emitter/FX authoring for this car: per-car sidecar, subject = car id.
    emitters_.Clear();
    if (!ctx.carDir.empty() && !ctx.id.empty()) {
        emitters_.SetSidecar(ctx.carDir / (ctx.id + ".fx.json"));
        emitters_.Load();
    }
    emitters_.SetSubject(ctx.id.empty() ? std::string("car") : ctx.id, 0.f, 0.f, 0.4f);
}

bool CarViewerPanel::EnsureFBO(int w, int h) {
    if (fboReady_ && fboW_ == w && fboH_ == h) return true;
    DeleteFBO();
    if (w <= 0 || h <= 0) return false;

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    glGenTextures(1, &fboColorTex_);
    glBindTexture(GL_TEXTURE_2D, fboColorTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboColorTex_, 0);

    glGenRenderbuffers(1, &fboDepthRbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, fboDepthRbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, fboDepthRbo_);

    bool ok = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!ok) { DeleteFBO(); return false; }

    fboW_ = w; fboH_ = h;
    fboReady_ = true;
    return true;
}

void CarViewerPanel::DeleteFBO() {
    if (fbo_)         { glDeleteFramebuffers(1, &fbo_);       fbo_         = 0; }
    if (fboColorTex_) { glDeleteTextures(1, &fboColorTex_);   fboColorTex_ = 0; }
    if (fboDepthRbo_) { glDeleteRenderbuffers(1,&fboDepthRbo_);fboDepthRbo_= 0; }
    fboReady_ = false; fboW_ = fboH_ = 0;
}

void CarViewerPanel::UploadPart(const CarContext& ctx) {
    meshRenderer_.Clear();
    currentGeom_ = 0;
    wheelGeom_ = 0;   // meshRenderer_.Clear() above already invalidated this handle
    if (ctx.bodySection.solidLists.empty()) return;
    const auto& sl = ctx.bodySection.solidLists[0];
    if (selectedPart_ >= (int)sl.objects.size()) return;
    currentGeom_ = meshRenderer_.UploadGeometry(sl.objects[selectedPart_]);

    if (showWheels_ && !ctx.wheelSection.solidLists.empty() &&
        !ctx.wheelSection.solidLists[0].objects.empty()) {
        // Use the first object — typically the wheel mesh at its highest LOD.
        wheelGeom_ = meshRenderer_.UploadGeometry(
            ctx.wheelSection.solidLists[0].objects[0]);
    }

    previewDirty_ = false;
}

// ─── Part name LOD helpers (Bug 6) ─────────────────────────────────────────
// NFS MW naming convention: PARTNAME_A (highest LOD) .. PARTNAME_D (lowest /
// collision proxy). Strip the suffix to group LOD variants under one entry
// in the part picker.
std::string CarViewerPanel::GetPartBaseName(const std::string& name) {
    if (name.size() >= 2 && name.back() >= 'A' && name.back() <= 'D'
        && name[name.size() - 2] == '_')
        return name.substr(0, name.size() - 2);
    return name;
}

char CarViewerPanel::GetPartLOD(const std::string& name) {
    if (name.size() >= 2 && name.back() >= 'A' && name.back() <= 'D'
        && name[name.size() - 2] == '_')
        return name.back();
    return 'A';
}

void CarViewerPanel::UpdateCameraInput(const VanVec2& size, bool hovered) {
    auto& io = VanGui::GetIO();
    bool lmb = io.MouseDown[0];
    bool rmb = io.MouseDown[1];
    camInput_.orbiting   = hovered && lmb && !io.KeyShift;
    camInput_.panning    = hovered && lmb && io.KeyShift;
    camInput_.looking    = hovered && rmb;
    camInput_.deltaX     = io.MouseDelta.x;
    camInput_.deltaY     = io.MouseDelta.y;
    camInput_.scroll     = hovered ? io.MouseWheel : 0.f;
    camInput_.viewportH  = size.y;
}

void CarViewerPanel::Draw(const CarContext& ctx, TaskQueue& /*tasks*/) {
    if (!shadersOk_) {
        VanGui::TextColored({1,0.3f,0.3f,1}, "Shaders failed to compile.");
        return;
    }
    if (!ctx.geometryReady) {
        VanGui::TextDisabled("Geometry not loaded.");
        return;
    }

    const auto& sl = ctx.bodySection.solidLists.empty()
                     ? SolidList{} : ctx.bodySection.solidLists[0];
    const bool hasParts = !sl.objects.empty();

    // ── Part picker ───────────────────────────────────────────────────────
    VanGui::AlignTextToFramePadding();
    VanGui::TextUnformatted("Part:");
    VanGui::SameLine();
    VanGui::SetNextItemWidth(200.f);
    if (hasParts) {
        // Build the deduplicated list of base names, in order of first
        // appearance, so LOD variants (PART_A/_B/_C/_D) collapse into one
        // entry instead of flooding the picker with hundreds of raw parts.
        std::vector<std::string> baseNames;
        std::unordered_map<std::string, int> firstIndex;
        for (int i = 0; i < (int)sl.objects.size(); ++i) {
            auto base = GetPartBaseName(sl.objects[i].name);
            if (firstIndex.find(base) == firstIndex.end()) {
                firstIndex[base] = i;
                baseNames.push_back(base);
            }
        }
        if (selectedBase_ >= (int)baseNames.size()) selectedBase_ = 0;

        const char* preview = !baseNames.empty()
                              ? baseNames[selectedBase_].c_str() : "(none)";
        if (VanGui::BeginCombo("##part", preview)) {
            for (int i = 0; i < (int)baseNames.size(); ++i) {
                bool sel = (i == selectedBase_);
                if (VanGui::Selectable(baseNames[i].c_str(), sel)) {
                    selectedBase_ = i;
                    selectedPart_ = firstIndex[baseNames[i]]; // default to _A (or bare name)
                    previewDirty_ = true;
                }
                if (sel) VanGui::SetItemDefaultFocus();
            }
            VanGui::EndCombo();
        }

        // LOD selector (A/B/C/D), only enabled for LODs that actually exist
        // for the currently-selected base part.
        VanGui::SameLine();
        if (!baseNames.empty()) {
            for (char lod : {'A', 'B', 'C', 'D'}) {
                const std::string target = baseNames[selectedBase_] + "_" + lod;
                int foundIdx = -1;
                for (int i = 0; i < (int)sl.objects.size(); ++i) {
                    if (sl.objects[i].name == target) { foundIdx = i; break; }
                }
                char btn[2] = { lod, '\0' };
                if (foundIdx >= 0) {
                    bool active = (selectedPart_ == foundIdx);
                    if (active) VanGui::PushStyleColor(VanGuiCol_Button,
                                    VanGui::GetStyleColorVec4(VanGuiCol_ButtonActive));
                    if (VanGui::Button(btn)) { selectedPart_ = foundIdx; previewDirty_ = true; }
                    if (active) VanGui::PopStyleColor();
                } else {
                    VanGui::BeginDisabled();
                    (void)VanGui::Button(btn);
                    VanGui::EndDisabled();
                }
                VanGui::SameLine();
            }
            VanGui::NewLine();
        }
    } else {
        VanGui::TextDisabled("(no parts)");
    }

    VanGui::SameLine();
    (void)VanGui::Checkbox("Wireframe", &showWireframe_);

    VanGui::SameLine();
    if (VanGui::Checkbox("Wheels", &showWheels_))
        previewDirty_ = true;

    // ── Emitters / FX (attach smoke, lights, birds, NOS…) ────────────────────
    if (VanGui::CollapsingHeader("Emitters / FX"))
        emitters_.DrawPanel();

    // ── Viewport ──────────────────────────────────────────────────────────
    VanVec2 vpSize = VanGui::GetContentRegionAvail();
    if (vpSize.x < 16) vpSize.x = 16;
    if (vpSize.y < 16) vpSize.y = 16;

    if (previewDirty_) UploadPart(ctx);

    bool ok = EnsureFBO((int)vpSize.x, (int)vpSize.y);
    if (ok && currentGeom_ != 0) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glViewport(0, 0, fboW_, fboH_);
        glClearColor(0.12f, 0.12f, 0.14f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glm::mat4 model(1.f);
        float aspect = (float)fboW_ / (float)fboH_;
        glm::mat4 vp   = camera_.Proj(aspect) * camera_.View();
        glm::mat4 mvp  = vp * model;

        if (showWireframe_) {
            shaderWire_.Bind();
            shaderWire_.SetMat4("uMVP", mvp);
#if !defined(__ANDROID__)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
            meshRenderer_.RenderGeometryImmediate(currentGeom_, shaderWire_,
                                                   camera_.View(),
                                                   camera_.Proj((float)fboW_/(float)fboH_),
                                                   nullptr);  // no textures in wireframe
#if !defined(__ANDROID__)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
        } else {
            shaderPhong_.Bind();
            shaderPhong_.SetMat4("uMVP",   mvp);
            shaderPhong_.SetMat4("uModel", model);
            meshRenderer_.RenderGeometryImmediate(currentGeom_, shaderPhong_,
                                                   camera_.View(),
                                                   camera_.Proj((float)fboW_/(float)fboH_),
                                                   &texManager_);
        }

        // ── Wheels (Bug 7) ──────────────────────────────────────────────────
        // CARINFO.BIN suspension offsets aren't decoded yet (STUB A), so use
        // reasonable hardcoded corner positions until real track/wheelbase
        // values are available. Game coordinate system is Z-up.
        if (showWheels_ && wheelGeom_) {
            static const glm::vec3 kCorners[4] = {
                { 0.75f,  1.30f, 0.0f },  // front-left
                {-0.75f,  1.30f, 0.0f },  // front-right
                { 0.75f, -1.30f, 0.0f },  // rear-left
                {-0.75f, -1.30f, 0.0f },  // rear-right
            };
            ShaderProgram& wheelShader = showWireframe_ ? shaderWire_ : shaderPhong_;
#if !defined(__ANDROID__)
            if (showWireframe_) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
            wheelShader.Bind();
            for (const auto& corner : kCorners) {
                glm::mat4 wheelModel = glm::translate(glm::mat4(1.f), corner);
                // Mirror right-side wheels so the wheel mesh (modeled for one
                // side) faces the correct way on the opposite side.
                if (corner.x < 0.f)
                    wheelModel = glm::scale(wheelModel, { -1.f, 1.f, 1.f });
                glm::mat4 wheelMVP = vp * wheelModel;
                wheelShader.SetMat4("uMVP",   wheelMVP);
                wheelShader.SetMat4("uModel", wheelModel);
                meshRenderer_.RenderGeometryImmediate(
                    wheelGeom_, wheelShader,
                    camera_.View(), camera_.Proj((float)fboW_/(float)fboH_),
                    showWireframe_ ? nullptr : &texManager_);
            }
#if !defined(__ANDROID__)
            if (showWireframe_) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    (void)VanGui::InvisibleButton("##vp", vpSize);
    bool hovered = VanGui::IsItemHovered();
    UpdateCameraInput(vpSize, hovered);
    camera_.Update(camInput_);

    if (ok) {
        VanVec2 p = VanGui::GetItemRectMin();
        VanDrawList* dl = VanGui::GetWindowDrawList();
        dl->AddImage(
            (VanTextureID)(uintptr_t)fboColorTex_,
            p, {p.x + vpSize.x, p.y + vpSize.y},
            {0,1}, {1,0});

        // Emitter markers + live looping particles over the car (car space =
        // preview space, model is identity).
        const float aspect = vpSize.y > 0.f ? vpSize.x / vpSize.y : 1.f;
        const glm::mat4 mvp = camera_.Proj(aspect) * camera_.View();
        emitters_.DrawOverlay(dl, mvp, p.x, p.y, vpSize.x, vpSize.y);
    }

    if (!hasParts)
        VanGui::TextDisabled("(no geometry in body BUN)");
}

} // namespace nfsmw
