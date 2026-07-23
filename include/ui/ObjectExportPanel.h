#pragma once
// ─── ObjectExportPanel.h ─────────────────────────────────────────────────────
// Phase 6: browse SolidObjects from a loaded STREAM*.BUN, preview them in a
// live OpenGL viewport, and export to OBJ+MTL or glTF/.glb.
//
// Mirrors the layout described in plan.md §6.2:
//   Left  — hierarchical tree: file → section → SolidObject (name + tri count)
//   Centre — live preview FBO rendered with MeshRenderer + OrbitCamera
//   Right  — export options (format, transform, texture dump) + export buttons
//
// The panel owns its own MeshRenderer, TextureManager, and OrbitCamera.
// The ShaderProgram instances (phong + wireframe) are built once on Init().
//
// All BUN loading and geometry upload runs on the TaskQueue worker thread;
// only the GL draw-call for the selected object executes on the main thread.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "core/FlagReference.h"
#include "core/ObjectClassifier.h"
#include "formats/AnimationBank.h"
#include "ui/EmitterEditor.h"
#include "formats/SolidList.h"
#include "formats/StreamBundle.h"
#include "formats/TPKBlock.h"
#include "renderer/MeshRenderer.h"
#include "renderer/ShaderProgram.h"
#include "renderer/TextureManager.h"
#include "renderer/OrbitCamera.h"
#include "editor/Exporter.h"
#include "patch/BackupManager.h"
#include "ui/FilterBox.h"
#include "async/TaskQueue.h"
#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <unordered_map>
#include <vangui.h>

namespace nfsmw {

class ObjectExportPanel {
public:
    // ── Lifecycle ────────────────────────────────────────────────────────────

    /// Call once after the GL context is current.
    /// Compiles the phong + wireframe shaders used by the preview viewport.
    Result<void> Init();

    /// Release all GL resources (FBO, textures, VAOs/VBOs).
    void Shutdown();

    // ── File loading ─────────────────────────────────────────────────────────

    /// Begin an async load of a BUN/BIN file. Progress is reported through
    /// `queue`; the panel's state is updated on the UI thread when done.
    void LoadFileAsync(const std::filesystem::path& path, TaskQueue& queue);

    /// Supply additional (e.g. GLOBAL) TPKBlocks for texture resolution.
    void SetExtraTPKs(std::vector<TPKBlock> globalPacks);

    // ── Per-frame ────────────────────────────────────────────────────────────

    /// Draw the panel into the current VanGui docking layout.
    /// Must be called from the UI (main/GL) thread each frame.
    void Draw(TaskQueue& queue);

    bool HasContent() const { return !sections_.empty(); }
    const std::string& LoadedPath() const { return loadedPath_; }

    /// Unload the current file (clears geometry + selection, keeps shaders)
    /// so a different file can be opened without restarting the app.
    void CloseFile();

    /// True when a single object is selected (so it can be replaced).
    bool HasSelectedObject() const { return selObj_ != nullptr && !loadedPath_.empty(); }

    /// Import `src` (.obj/.glb/.gltf) and replace the selected object's geometry
    /// in the loaded file (full rebuild). Runs on the TaskQueue; reloads on done.
    void ImportReplaceSelected(const std::filesystem::path& src, TaskQueue& queue);

private:
    // ── Loaded content ───────────────────────────────────────────────────────
    std::string              loadedPath_;
    std::vector<StreamSection> sections_;      // populated on worker thread, swapped in on UI
    std::vector<TPKBlock>    extraTPKs_;       // GLOBAL texture packs

    // ── Selection state ──────────────────────────────────────────────────────
    int  selectedSection_ = -1;
    int  selectedList_    = -1;
    int  selectedObject_  = -1;

    // Pointer to selected object (valid only while sections_ is unchanged)
    const SolidObject*  selObj_  = nullptr;
    const StreamSection* selSect_ = nullptr;

    // ── GL preview state ─────────────────────────────────────────────────────
    MeshRenderer    meshRenderer_;
    TextureManager  texManager_;
    OrbitCamera     camera_;
    ShaderProgram   shaderPhong_;
    ShaderProgram   shaderWire_;

    // Offscreen FBO for the preview
    uint32_t fbo_        = 0;
    uint32_t fboColorTex_= 0;
    uint32_t fboDepthRbo_= 0;
    int      fboW_       = 0;
    int      fboH_       = 0;

    // Currently uploaded geometry handle (one at a time)
    uint64_t currentGeom_ = 0;

    bool showWireframe_  = false;
    bool fboReady_       = false;
    bool shadersOk_      = false;
    bool previewDirty_   = true;   // re-upload geometry on next draw

    // ── Object properties / flag editing (smackable, sway, wind, …) ───────────
    // Edited copy of selObj_->flags; committed to disk by SaveFlagsToFile().
    uint32_t     pendingFlags_ = 0;
    bool         flagsDirty_   = false;   // pendingFlags_ != selObj_->flags
    bool         savingFlags_  = false;   // writeback in flight (busy state)
    std::string  lastFlagStatus_;         // status line under the flag editor

    // Animated wind-sway preview (bends the object about its base like the game's
    // foliage). Auto-suggested for swayable objects; user can force it on/off.
    bool         swayPreview_  = false;

    // ── World-animation banks (cranes, ships, blimp, airliners) ───────────────
    // Inventoried from the loaded bundle. selAnim_ points at the record for the
    // current selection (nullptr if the object isn't animated).
    AnimBankInventory     animBanks_;
    const AnimatedObject* selAnim_       = nullptr;
    bool                  animatePreview_ = false;  // play the bank motion in preview

    // ── Transform editor (in-place edit of the object's world matrix) ─────────
    float editPos_[3]   = { 0.f, 0.f, 0.f };
    float editRot_[3]   = { 0.f, 0.f, 0.f };   // euler degrees
    float editScale_[3] = { 1.f, 1.f, 1.f };
    float origPos_[3]   = { 0.f, 0.f, 0.f };
    float origRot_[3]   = { 0.f, 0.f, 0.f };
    float origScale_[3] = { 1.f, 1.f, 1.f };
    bool  xformActive_  = false;   // preview the edited rotation/scale
    bool  xformSaving_  = false;
    std::string lastXformStatus_;

    // ── Sub-mesh inspector (per-mesh visibility / isolate) ────────────────────
    std::vector<uint8_t> meshVisible_;   // 1 = drawn, 0 = hidden (per SolidMesh)

    // ── Emitter / FX attachments (shared component; .fx.json sidecar) ─────────
    EmitterEditor emitters_;

    // ── Texture swap (reassign an object's texture slot in place) ─────────────
    // All textures the loaded packs provide (hash + name), built on load. Lets
    // the user retexture an object without re-exporting geometry.
    std::vector<std::pair<uint32_t, std::string>> availTextures_;
    int          texEditSlot_  = -1;    // which textureHashes[] slot is being edited
    uint32_t     texNewHash_   = 0;     // chosen replacement texture hash
    bool         retexBusy_    = false; // writeback in flight
    std::string  lastTexStatus_;

    // Input state passed to OrbitCamera::Update()
    OrbitCamera::InputState camInput_;

    // ── Export options ───────────────────────────────────────────────────────
    enum class ExportFormat { OBJ, GLTF };
    ExportFormat   exportFmt_          = ExportFormat::OBJ;
    bool           exportApplyXform_   = true;
    bool           exportTextures_     = true;
    char           exportTexSubdir_[64]= "textures";

    // Last export/import result (shown as a status line)
    std::string    lastExportStatus_;

    // Object-tree search box (filters the leaf nodes by name / hash).
    FilterBox      objFilter_;

    // ── Layout: tree | preview | export panes, drag-resizable (ChildFlags_ResizeX)
    // like the Cars panel — persisted across frames, not across restarts.
    static constexpr float kTreeWidthDefault    = 240.0f;
    static constexpr float kPreviewWidthDefault = 540.0f;
    static constexpr float kPaneWidthMin        = 160.0f;
    float treeWidth_    = kTreeWidthDefault;
    float previewWidth_ = kPreviewWidthDefault;

    // ── Import / replace state ───────────────────────────────────────────────
    BackupManager  backup_;
    bool           reloadRequested_ = false;   // set on worker, drained in Draw()
    std::string    reloadPath_;
    // After a replace+reload, re-select the same object (by name-hash) so the
    // preview updates in place instead of clearing and forcing a manual re-click.
    uint32_t       reselectHash_    = 0;        // 0 = nothing to re-select
    bool           importInProgress_ = false;   // drives the in-panel busy spinner

    // ── Async load staging ───────────────────────────────────────────────────
    // Worker thread fills pendingSections_, then signals pendingReady_.
    // PumpPending() drains it on the UI thread.
    std::vector<StreamSection>   pendingSections_;
    std::string                  pendingPath_;
    bool                         pendingReady_ = false;  // guarded by TaskQueue onDone

    // ── Private methods ──────────────────────────────────────────────────────

    // Sub-panels
    void DrawObjectTree();
    void DrawPreviewViewport();
    void DrawExportOptions(TaskQueue& queue);
    void DrawObjectProperties(TaskQueue& queue);   // classification + flag editor
    void DrawTransformEditor(TaskQueue& queue);    // move/rotate/scale + in-place save
    void DrawSubMeshInspector();                   // per-mesh material/isolate/hide
    void DrawTextureEditor(TaskQueue& queue);      // per-slot texture reassignment

    // Decompose the selection's matrix into edit* fields; commit edited matrix.
    void SyncTransformFromSelection();
    void ApplyTransform(TaskQueue& queue);

    // Commit pendingFlags_ to the loaded file (offline, in-place, backed up).
    void SaveFlagsToFile(TaskQueue& queue);

    // Reassign textureHashes[texEditSlot_] to texNewHash_ on disk (in-place).
    void ApplyTextureSwap(TaskQueue& queue);

    // Rebuild availTextures_ from the loaded section packs + extra (GLOBAL) TPKs.
    void RebuildAvailableTextures();

    // Model matrix for the current preview frame (identity, or the sway bend).
    glm::mat4 PreviewModelMatrix() const;

    // Selection handling
    void SelectObject(int sectionIdx, int listIdx, int objIdx);
    void UploadSelectedGeometry();

    // FBO management
    bool EnsureFBO(int w, int h);
    void DeleteFBO();

    // Export helpers
    void RunExportObject(const SolidObject& obj, const StreamSection& sect,
                         TaskQueue& queue);
    void RunExportSection(const StreamSection& sect, TaskQueue& queue);

    // Pending-load drain (UI thread)
    void PumpPending();

    // Orbit camera GLFW-style input from VanGui mouse state
    void UpdateCameraInput(const VanVec2& contentSize, bool hovered);
};

} // namespace nfsmw
