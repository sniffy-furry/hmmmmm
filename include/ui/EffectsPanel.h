#pragma once
// ─── ui/EffectsPanel.h ────────────────────────────────────────────────────────
// Dedicated "Effects" section: browse the game's particle effects/emitters from
// GLOBAL/attributes.bin and edit their real EmitterData parameters (colours,
// gravity, life, particle count, speed, spread, …) with a live looping preview.
// Effects are pure presentation-only vault data, so this is the safest editing
// surface in the game (MWEncyclopedia C19.4).
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/VaultDecoder.h"
#include "core/VaultSchema.h"
#include "patch/BackupManager.h"
#include "ui/FileDialog.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace nfsmw {

class EffectsPanel {
public:
    void Draw();

private:
    FileDialog                          fileDlg_;
    std::filesystem::path               vaultPath_;
    std::vector<VaultDecodedRecord>     recs_;
    BackupManager                       backup_;
    std::string                         status_;
    char                                search_[64] = {0};
    int                                 selected_ = -1;
    bool                                triedAuto_ = false;

    // Float edits (offset -> value) and packed-uint edits (offset -> bits), the
    // latter for Color1-4 fields.
    std::unordered_map<uint64_t, float>    edits_;
    std::unordered_map<uint64_t, uint32_t> colEdits_;

    // One effect "collection" (all fields sharing a key), built from records.
    struct FxCollection {
        uint32_t                     keyHash = 0;
        std::string                  name;
        std::vector<const VaultTriple*> fields;
    };
    std::vector<FxCollection> collections_;

    void LoadVault(const std::filesystem::path& p);
    void RebuildCollections();
    void DrawToolbar();
    void DrawList(float w);
    void DrawEditor(float w);
    void DrawPreviewBox(const std::string& effectName, float h);
    void SaveEdits();
};

} // namespace nfsmw
