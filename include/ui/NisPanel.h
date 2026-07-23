#pragma once
// ─── ui/NisPanel.h ────────────────────────────────────────────────────────────
// NIS cutscene editor panel.
//
// Loads NIS/Scene_*_BundleB.bun files and surfaces every embedded EAGLAnim
// chunk (0x00E34009) as a named skeleton clip. Supports:
//   • Browsing skeletons + bone lists (from the MIPS ELF .symtab)
//   • Live cutscene playback: a transport + scrub timeline driven by the REAL
//     event schedule (EventSequenceChunk 0x0003B811 — clip length + event
//     markers), the scene rendered into an FBO, and the skeleton posed per
//     frame. The authored bone/camera keyframe motion is an 8-bit-quantised
//     stream whose dequantisation is still unsolved (encyclopedia C10.3), so
//     the rig plays at its bind pose (with an optional, clearly-labelled
//     synthetic sway) behind a clean hook for real sample evaluation.
//   • 3D viewport: textured character mesh from SolidList chunks + skeleton
//     overlay rendered from the decoded bind-pose (forward kinematics)
//   • Exporting a clip's raw chunk payload to a .bin file for external editing
//     (e.g. icebreaker / other EAGLAnim tools)
//   • Importing a replacement payload → splices it back into the BUN buffer,
//     recalculates all parent container sizes, re-parses affected clips
//   • Saving the modified BUN (always uncompressed — game accepts both forms)
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/NisAnim.h"
#include "formats/NisEventSequence.h"
#include "formats/TPKBlock.h"
#include "formats/SolidList.h"
#include "renderer/ShaderProgram.h"
#include "renderer/OrbitCamera.h"
#include "renderer/TextureManager.h"
#include "renderer/MeshRenderer.h"
#include "ui/FileDialog.h"
#include "async/TaskQueue.h"
#include <filesystem>
#include <string>
#include <vector>

namespace nfsmw {

class NisPanel {
public:
    /// Call once after the GL context is current (compiles shaders, creates VAO).
    Result<void> Init();
    /// Release all GL resources.
    void Shutdown();

    void Load(const std::filesystem::path& path, TaskQueue& tasks);
    void Close();

    void Draw(float w, float h, FileDialog& fd, TaskQueue& tasks);

    bool IsLoaded()  const { return state_ == State::Ready; }
    bool IsModified() const { return dirty_; }

private:
    enum class State { Empty, Loading, Ready, Error };
    State       state_    = State::Empty;
    std::string errorMsg_;
    std::string statusMsg_;

    std::filesystem::path        filePath_;
    std::string                  sourceName_;
    std::vector<uint8_t>         rawBytes_;    // decompressed BUN bytes
    bool                         wasCompressed_ = false;
    bool                         dirty_         = false;

    struct Clip {
        size_t      absOffset   = 0;  // offset of ChunkHeader in rawBytes_
        uint32_t    payloadSize = 0;  // bytes in rawBytes_[absOffset+8 ..]
        NisAnimClip anim;
    };
    std::vector<Clip> clips_;
    int               selectedClip_ = -1;
    char              boneFilter_[128] = {};
    float             listPaneW_    = 200.f;  // user-resizable clip list width
    float             detailPaneW_  = 280.f;  // user-resizable detail panel width

    // ── Drawing helpers ───────────────────────────────────────────────────────
    void DrawEmptyState(float w, float h);
    void DrawLoadingState();
    void DrawErrorState();
    void DrawReadyState(float w, float h, FileDialog& fd);
    void DrawClipList(float listW, float h);
    void DrawClipDetail(float detailW, float h, FileDialog& fd);

    // ── Timeline / playback ───────────────────────────────────────────────────
    NisSceneTimeline timeline_;             // real event schedule (may be invalid)
    bool        playing_    = false;
    float       timeSec_    = 0.f;          // playhead (seconds)
    float       duration_   = 10.f;         // clip length (from timeline_ or default)
    bool        loop_       = true;
    float       playSpeed_  = 1.f;
    bool        autoOrbit_  = false;        // preview-camera spin (NOT the authored cam)
    NisPoseMode poseMode_   = NisPoseMode::BindPose;
    int         curEventIdx_ = -1;          // most-recent fired event (for readout)

    void UpdatePlayback();
    void DrawTimelineBar(float w);

    // ── Skeleton 3D viewport ──────────────────────────────────────────────────
    void DrawSkeletonViewport(float w, float h);
    void BuildSkeletonLines();              // poses the rig at timeSec_/poseMode_
    bool EnsureNisFBO(int w, int h);
    void DeleteNisFBO();
    void UpdateNisCamInput(float vpW, float vpH, bool hovered);

    // ── Mesh + texture loading ─────────────────────────────────────────────────
    void LoadMeshAndTextures();   // scan rawBytes_ for TPK + SolidList, upload to GL
    void FreeMeshAndTextures();   // delete GPU resources

    // ── GL state: FBO + skeleton lines ───────────────────────────────────────
    ShaderProgram      skelShader_;       // GL_LINES pass-through
    OrbitCamera        nisCam_;
    OrbitCamera::InputState camInput_{};
    uint32_t           nisFbo_        = 0;
    uint32_t           nisFboColor_   = 0;
    uint32_t           nisFboDepth_   = 0;
    int                nisFboW_       = 0;
    int                nisFboH_       = 0;
    uint32_t           lineVAO_       = 0;
    uint32_t           lineVBO_       = 0;
    int                lineVertCount_ = 0;
    int                skelBuiltFor_  = -2;   // clip index the lines were built for
    float              poseBuiltTime_ = -1.f; // time the lines were posed at
    int                poseBuiltMode_ = -1;   // NisPoseMode the lines were built with
    bool               skelShaderOk_  = false;

    // ── GL state: textured mesh ───────────────────────────────────────────────
    ShaderProgram      meshShader_;       // Phong + texture (same as ObjectExportPanel)
    TextureManager     nisTexMgr_;
    MeshRenderer       nisMeshRenderer_;
    std::vector<uint64_t> meshHandles_;
    bool               hasMesh_       = false;
    bool               meshShaderOk_  = false;

    // ── Chunk I/O ──────────────────────────────────────────────────────────────
    void          ExportChunk(size_t clipIdx, const std::filesystem::path& outPath);
    Result<void>  ImportChunk(size_t clipIdx, const std::filesystem::path& srcPath);

    void Reparse();

    static void FixAllContainerSizes(std::vector<uint8_t>& buf);
    static size_t FixContainerSizesAt(std::vector<uint8_t>& buf,
                                      size_t pos, size_t bufEnd);
};

} // namespace nfsmw
