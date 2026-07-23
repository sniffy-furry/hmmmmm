#pragma once
// ─── patch/MwMod.h ────────────────────────────────────────────────────────────
// Reader + planner for the .mwmod distribution format (roadmap D1; see
// reference/MWMOD_FORMAT.md). This is the engine half: it parses a manifest
// into a typed edit list, builds a resolved EDIT PLAN, and detects conflicts
// (two edits hitting the same file+locator). Actually writing bytes is a thin
// final step that delegates to the existing patchers (BunRepacker,
// MinimapRepacker, the audio replace paths) through BackupManager — kept
// separate so the parse/plan/conflict logic is pure and unit-testable.
//
// Manifest input: to stay dependency-free in the headless core, the engine
// reads a minimal line-based form (one edit per line) that is a 1:1 lowering
// of the JSON schema in data/schema/mwmod.schema.json. A GUI/front-end may
// parse the richer JSON and feed MwModEdit structs directly.
//
//   # comment
//   <action> <file> <locator> [asset]
//   replace-texture  TRACKS/STREAML2RA.BUN  name:SKY_MIDDAY_A_CLOUDS_A  assets/clouds.dds
//   replace-minimap-tile  TRACKS/L2RA/MINI_MAP.BIN  tile:0  assets/map0.dds
//   set-vault-field  GLOBAL/attributes.bin  key:0x0743CFB1  value:240.0
//
// Locator forms: name:<str> | hash:0x.. | object:0x.. | key:0x.. | tile:<n> |
//                offset:<n>  (offset requires a value: token for raw patches)
// GL/GLM-free; depends only on std + the project Result type.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include <string>
#include <vector>
#include <cstdint>

namespace nfsmw {

enum class MwAction {
    ReplaceTexture, ReplaceMesh, SetVaultField, ReplaceAudio, ReplaceMinimapTile, RawPatch, Unknown
};

enum class MwLocatorKind { Name, Hash, Object, VaultKey, TileIndex, Offset, None };

struct MwModEdit {
    MwAction       action      = MwAction::Unknown;
    std::string    actionStr;                 ///< original action token (for messages)
    std::string    file;                      ///< game-relative target path
    MwLocatorKind  locKind     = MwLocatorKind::None;
    std::string    locText;                   ///< name string, hex, or asset-relative path
    uint64_t       locNum      = 0;           ///< tile index / raw offset
    std::string    asset;                     ///< assets/ path or value:/bytes: payload
    int            line        = 0;           ///< source line (for diagnostics)

    /// Conflict identity: two edits with the same key fight over the same data.
    std::string ConflictKey() const { return file + "|" + locText + "|" + std::to_string(locNum); }
};

struct MwMod {
    std::string              name;
    std::string              author;
    std::string              targetVersion;
    std::vector<MwModEdit>   edits;
};

/// One detected conflict between two edits (same file+locator).
struct MwConflict { size_t a = 0, b = 0; std::string key; };

class MwModParser {
public:
    /// Parse the line-based manifest text. Unknown actions/locators are flagged
    /// via `warnings` but kept (action=Unknown) so callers can report them.
    static Result<MwMod> Parse(const std::string& text, std::vector<std::string>* warnings = nullptr);

    /// Detect conflicts (edits sharing a ConflictKey). Order is the apply order;
    /// a conflict means a later edit would overwrite an earlier one's target.
    static std::vector<MwConflict> FindConflicts(const MwMod& mod);

    static MwAction      ParseAction(const std::string& s);
    static const char*   ActionName(MwAction a);
};

} // namespace nfsmw
