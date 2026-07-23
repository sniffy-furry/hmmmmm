#pragma once
// ─── ui/MinimapPanel.h ────────────────────────────────────────────────────────
// Viewer and tile-replacement editor for MINI_MAP.BIN.
//
// MINI_MAP.BIN structure (verified against retail PC v1.3):
//   N × { 0x0003A100 chunk header (8 B) + JDLZ-compressed payload }
//   Each payload decompresses to a standard 0xB3300000 TPK block containing
//   exactly one 128×128 DXT3 texture named MINI_MAP_CHOPn.
//
//   Tile grid: tiles 0..N-1 fill an (cols × rows) grid, where
//     cols = ceil(sqrt(N))  (8 for the standard 64-tile Rockport map)
//     rows = ceil(N / cols)
//   col = n % cols,  row = n / cols  (left-to-right, top-to-bottom)
//
// Panel responsibilities:
//   • Load MINI_MAP.BIN via MinimapFile (formats/MinimapFile.h).
//   • Upload all tile RGBA data to GL textures (one per tile).
//   • Composite them into a single atlas GL texture for the zoomable viewer.
//   • Allow the user to click a tile and replace it with a new DDS/PNG file.
//   • Write the replacement back to disk via MinimapRepacker::ReplaceTile().
//   • Export the full composited map as a PNG via stb_image_write.
//
// The panel does NOT parse the CARP/RNnd GPS road graph (WorldMapData chunk
// 0x0003B800 in L2RA.BUN) — that is a separate future subsystem.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/MinimapFile.h"
#include "patch/BackupManager.h"
#include "ui/FileDialog.h"
#include "async/TaskQueue.h"
#include <vangui.h>
#include "renderer/GLCompat.h"
#include <string>
#include <vector>
#include <filesystem>

namespace nfsmw {

class MinimapPanel {
public:
    MinimapPanel()  = default;
    ~MinimapPanel() { FreeGLResources(); }

    MinimapPanel(const MinimapPanel&)            = delete;
    MinimapPanel& operator=(const MinimapPanel&) = delete;

    // ── Lifecycle ────────────────────────────────────────────────────────────

    /// Load MINI_MAP.BIN from `path`.  Spawns a background task via `tasks`
    /// so the UI stays responsive; call Draw() every frame as normal.
    void Load(const std::filesystem::path& path, TaskQueue& tasks);

    /// Release all GL textures and reset to the empty state.
    void Close();

    // ── Per-frame draw ───────────────────────────────────────────────────────

    /// Render the panel into the current VanGui content area (w × h pixels).
    /// `fd` is borrowed — the panel shows it only when the user clicks Replace.
    void Draw(float w, float h, FileDialog& fd, TaskQueue& tasks);

    bool IsLoaded() const { return state_ == State::Ready; }

private:
    // ── State machine ────────────────────────────────────────────────────────
    enum class State { Empty, Loading, Ready, Error };
    State       state_   = State::Empty;
    std::string errorMsg_;

    // ── Data ─────────────────────────────────────────────────────────────────
    MinimapFile file_;                   ///< parsed tile data (set on Ready)
    int         gridCols_ = 0;           ///< computed from tile count
    int         gridRows_ = 0;

    // ── GL resources ─────────────────────────────────────────────────────────
    std::vector<GLuint> tileTextures_;   ///< one GL texture per tile (128×128 RGBA)
    GLuint              atlasTexture_ = 0; ///< composited full-map texture
    int                 atlasW_ = 0, atlasH_ = 0;

    // ── Viewer state ─────────────────────────────────────────────────────────
    float   zoom_       = 1.0f;          ///< current zoom level (1.0 = fit)
    VanVec2  pan_        = VanVec2(0.0f, 0.0f); ///< pan offset in pixels
    bool    panning_    = false;
    VanVec2  panStart_   = VanVec2(0.0f, 0.0f);
    VanVec2  panOrigin_  = VanVec2(0.0f, 0.0f);
    int     hoveredTile_ = -1;           ///< tile index under mouse (-1 = none)
    int     selectedTile_ = -1;          ///< tile index last clicked

    // ── Replacement state ────────────────────────────────────────────────────
    BackupManager backup_;
    std::string   replaceStatus_;        ///< empty = no message, else shown in toolbar
    bool          replaceBusy_ = false;

    // ── Export state ─────────────────────────────────────────────────────────
    std::string exportStatus_;

    // ── GL helpers ───────────────────────────────────────────────────────────

    /// Decode all tile pixel data to RGBA, upload to tileTextures_, then
    /// composite into atlasTexture_.  Called on the main thread after loading.
    void UploadToGL();

    /// Build (or rebuild) the atlas from tileTextures_ current contents.
    void RebuildAtlas();

    /// Free all GL objects.
    void FreeGLResources();

    // ── Draw helpers ─────────────────────────────────────────────────────────
    void DrawEmptyState(float w, float h);
    void DrawLoadingState(float w, float h);
    void DrawErrorState(float w, float h);
    void DrawReadyState(float w, float h, FileDialog& fd, TaskQueue& tasks);

    void DrawToolbar(FileDialog& fd, TaskQueue& tasks);
    void DrawViewer(float viewW, float viewH);
    void DrawTileInfo();

    // ── Tile interaction ─────────────────────────────────────────────────────

    /// Map a canvas-space point to a tile index, or -1 if outside the grid.
    int HitTestTile(VanVec2 canvasPos, VanVec2 imageOrigin, float scale) const;

    // ── Replacement ──────────────────────────────────────────────────────────

    /// Replace tile `tileIdx` with an image file at `srcPath` (DDS or PNG).
    /// Spawns a background task; updates replaceStatus_ and tileTextures_ on
    /// completion.
    void DoReplace(int tileIdx,
                   const std::filesystem::path& srcPath,
                   TaskQueue& tasks);

    // ── Export ───────────────────────────────────────────────────────────────

    /// Write the composited atlas to a PNG file chosen via `fd`.
    void DoExportPNG(FileDialog& fd);

    // ── DXT3 decode (software fallback — used during load) ───────────────────

    /// Decode a single 128×128 DXT3 texture to RGBA (128×128×4 bytes).
    static std::vector<uint8_t> DecodeDXT3(const MinimapTile& tile);
};

} // namespace nfsmw
