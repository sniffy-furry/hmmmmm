// .mwmod manifest parsing + conflict detection (roadmap D1 engine).
#include "microtest.h"
#include "patch/MwMod.h"
#include <string>

using namespace nfsmw;

static const char* MANIFEST =
    "# sample mod\n"
    "name: Bright Sky + reskin\n"
    "author: tester\n"
    "target: NFSMW PC retail v1.3\n"
    "replace-texture TRACKS/STREAML2RA.BUN name:SKY_MIDDAY_A_CLOUDS_A assets/clouds.dds\n"
    "replace-minimap-tile TRACKS/L2RA/MINI_MAP.BIN tile:3 assets/map3.dds\n"
    "set-vault-field GLOBAL/attributes.bin key:0x0743CFB1 value:240.0\n"
    "replace-texture TRACKS/STREAML2RA.BUN name:SKY_MIDDAY_A_CLOUDS_A assets/other.dds\n"
    "bogus-action FOO bar:baz\n";

TEST(mwmod_parse_headers_and_edits) {
    std::vector<std::string> warn;
    auto r = MwModParser::Parse(MANIFEST, &warn);
    CHECK(bool(r));
    if (!r) return;
    auto& m = r.value;
    CHECK(m.name == "Bright Sky + reskin");
    CHECK(m.author == "tester");
    CHECK(m.targetVersion == "NFSMW PC retail v1.3");
    CHECK_EQ(m.edits.size(), (size_t)5);
    CHECK_EQ(warn.size(), (size_t)1);  // the bogus action
    CHECK(m.edits[0].action == MwAction::ReplaceTexture);
    CHECK(m.edits[0].locKind == MwLocatorKind::Name);
    CHECK(m.edits[0].locText == "SKY_MIDDAY_A_CLOUDS_A");
    CHECK(m.edits[1].locKind == MwLocatorKind::TileIndex);
    CHECK_EQ(m.edits[1].locNum, (uint64_t)3);
    CHECK(m.edits[2].action == MwAction::SetVaultField);
    CHECK(m.edits[4].action == MwAction::Unknown);
}

TEST(mwmod_detects_conflicts) {
    auto r = MwModParser::Parse(MANIFEST, nullptr);
    CHECK(bool(r));
    if (!r) return;
    auto conflicts = MwModParser::FindConflicts(r.value);
    CHECK_EQ(conflicts.size(), (size_t)1);    // edits 0 and 3 both target the same texture
    if (!conflicts.empty()) {
        CHECK_EQ(conflicts[0].a, (size_t)0);
        CHECK_EQ(conflicts[0].b, (size_t)3);
    }
}

TEST(mwmod_action_names_roundtrip) {
    CHECK(MwModParser::ParseAction("replace-minimap-tile") == MwAction::ReplaceMinimapTile);
    CHECK(std::string(MwModParser::ActionName(MwAction::SetVaultField)) == "set-vault-field");
    CHECK(MwModParser::ParseAction("nope") == MwAction::Unknown);
}
