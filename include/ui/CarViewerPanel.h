#pragma once
// ─── ui/CarViewerPanel.h ──────────────────────────────────────────────────────
// 3-D body preview using MeshRenderer + OrbitCamera.
// Mirrors the ObjectExportPanel preview viewport pattern but sources geometry
// from CarContext instead of a standalone BUN file.
//
// Features:
//   • Part picker dropdown — lists every SolidObject in bodySection
//   • Wheel preview toggle — overlays wheel geometry at suspension corner positions
//     (corner offsets derived from CarContext::perfData suspension geometry)
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"

#include "renderer/MeshRenderer.h"
#include "renderer/OrbitCamera.h"
#include "renderer/ShaderProgram.h"
#include "renderer/TextureManager.h"
#include "ui/EmitterEditor.h"
#include "async/TaskQueue.h"
#include <vangui.h>
#include <cstdint>

namespace nfsmw {

struct CarContext;

class CarViewerPanel {
public:
    Result<void> Init();
    void Shutdown();

    /// Upload geometry from ctx when a new car is selected.
    void OnCarLoaded(const CarContext& ctx);

    /// Draw the 3-D preview viewport into the current VanGui region.
    void Draw(const CarContext& ctx, TaskQueue& tasks);

private:
    // GL preview resources (same FBO pattern as ObjectExportPanel)
    MeshRenderer   meshRenderer_;
    TextureManager texManager_;
    OrbitCamera    camera_;
    ShaderProgram  shaderPhong_;
    ShaderProgram  shaderWire_;

    uint32_t fbo_         = 0;
    uint32_t fboColorTex_ = 0;
    uint32_t fboDepthRbo_ = 0;
    int      fboW_        = 0;
    int      fboH_        = 0;

    uint64_t currentGeom_ = 0;   ///< handle of uploaded SolidObject
    uint64_t wheelGeom_   = 0;   ///< handle of uploaded wheel geometry (Bug 7)

    bool shadersOk_      = false;
    bool fboReady_       = false;
    bool showWireframe_  = false;
    bool showWheels_     = false;
    bool previewDirty_   = true;

    // Part picker state
    int  selectedPart_   = 0;    ///< index into bodySection.solidLists[0].objects
    int  selectedWheel_  = 0;    ///< index into wheelSection.solidLists[0].objects
    int  selectedBase_   = 0;    ///< index into the deduplicated base-name list

    /// Strip a trailing "_A".."_D" LOD suffix from a part name, if present.
    static std::string GetPartBaseName(const std::string& name);
    /// Return the LOD letter ('A'-'D') encoded in a part name, defaulting to
    /// 'A' if the name has no LOD suffix.
    static char GetPartLOD(const std::string& name);

    OrbitCamera::InputState camInput_;

    // Emitter/FX authoring for the car (attach smoke, lights, birds, NOS…).
    EmitterEditor emitters_;

    bool EnsureFBO(int w, int h);
    void DeleteFBO();
    void UploadPart(const CarContext& ctx);
    void UpdateCameraInput(const VanVec2& size, bool hovered);
};

} // namespace nfsmw
