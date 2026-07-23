// ─── patch/MwMod.cpp ──────────────────────────────────────────────────────────
// See MwMod.h. Pure parse/plan/conflict logic (no I/O, no glm) so it is fully
// unit-testable; the byte-application step lives in the CLI/GUI and delegates
// to BunRepacker / MinimapRepacker / the audio replace paths via BackupManager.
#include "patch/MwMod.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>

namespace nfsmw {

MwAction MwModParser::ParseAction(const std::string& s) {
    if (s == "replace-texture")      return MwAction::ReplaceTexture;
    if (s == "replace-mesh")         return MwAction::ReplaceMesh;
    if (s == "set-vault-field")      return MwAction::SetVaultField;
    if (s == "replace-audio")        return MwAction::ReplaceAudio;
    if (s == "replace-minimap-tile") return MwAction::ReplaceMinimapTile;
    if (s == "raw-patch")            return MwAction::RawPatch;
    return MwAction::Unknown;
}

const char* MwModParser::ActionName(MwAction a) {
    switch (a) {
        case MwAction::ReplaceTexture:     return "replace-texture";
        case MwAction::ReplaceMesh:        return "replace-mesh";
        case MwAction::SetVaultField:      return "set-vault-field";
        case MwAction::ReplaceAudio:       return "replace-audio";
        case MwAction::ReplaceMinimapTile: return "replace-minimap-tile";
        case MwAction::RawPatch:           return "raw-patch";
        default:                           return "unknown";
    }
}

static void parse_locator(const std::string& tok, MwModEdit& e) {
    auto eq = tok.find(':');
    if (eq == std::string::npos) { e.locKind = MwLocatorKind::None; e.locText = tok; return; }
    std::string k = tok.substr(0, eq), v = tok.substr(eq + 1);
    if      (k == "name")   { e.locKind = MwLocatorKind::Name;      e.locText = v; }
    else if (k == "hash")   { e.locKind = MwLocatorKind::Hash;      e.locText = v; }
    else if (k == "object") { e.locKind = MwLocatorKind::Object;    e.locText = v; }
    else if (k == "key")    { e.locKind = MwLocatorKind::VaultKey;  e.locText = v; }
    else if (k == "tile")   { e.locKind = MwLocatorKind::TileIndex; e.locNum  = std::strtoull(v.c_str(), nullptr, 0); e.locText = v; }
    else if (k == "offset") { e.locKind = MwLocatorKind::Offset;    e.locNum  = std::strtoull(v.c_str(), nullptr, 0); e.locText = v; }
    else                    { e.locKind = MwLocatorKind::None;      e.locText = tok; }
}

Result<MwMod> MwModParser::Parse(const std::string& text, std::vector<std::string>* warnings) {
    MwMod mod;
    std::istringstream in(text);
    std::string line;
    int lineNo = 0;
    auto warn = [&](const std::string& m) { if (warnings) warnings->push_back(m); };

    while (std::getline(in, line)) {
        ++lineNo;
        // strip trailing CR and leading/trailing whitespace
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t b = line.find_first_not_of(" \t");
        if (b == std::string::npos) continue;
        size_t e2 = line.find_last_not_of(" \t");
        std::string s = line.substr(b, e2 - b + 1);
        if (s.empty() || s[0] == '#') continue;

        // header directives: name:/author:/target:
        auto trimv = [](std::string v) {
            size_t p = v.find_first_not_of(" \t");
            return (p == std::string::npos) ? std::string() : v.substr(p);
        };
        if (s.rfind("name:", 0) == 0)   { mod.name          = trimv(s.substr(5)); continue; }
        if (s.rfind("author:", 0) == 0) { mod.author        = trimv(s.substr(7)); continue; }
        if (s.rfind("target:", 0) == 0) { mod.targetVersion = trimv(s.substr(7)); continue; }

        std::istringstream ls(s);
        std::string act, file, loc, asset;
        ls >> act >> file >> loc;
        std::getline(ls, asset);
        size_t ab = asset.find_first_not_of(" \t");
        asset = (ab == std::string::npos) ? "" : asset.substr(ab);

        if (act.empty() || file.empty() || loc.empty()) {
            warn("line " + std::to_string(lineNo) + ": incomplete edit, skipped");
            continue;
        }
        MwModEdit ed;
        ed.line      = lineNo;
        ed.actionStr = act;
        ed.action    = ParseAction(act);
        ed.file      = file;
        ed.asset     = asset;
        parse_locator(loc, ed);
        if (ed.action == MwAction::Unknown)
            warn("line " + std::to_string(lineNo) + ": unknown action '" + act + "'");
        mod.edits.push_back(std::move(ed));
    }
    return Result<MwMod>::Ok(std::move(mod));
}

std::vector<MwConflict> MwModParser::FindConflicts(const MwMod& mod) {
    std::vector<MwConflict> out;
    std::unordered_map<std::string, size_t> seen;  // ConflictKey -> first index
    for (size_t i = 0; i < mod.edits.size(); ++i) {
        std::string k = mod.edits[i].ConflictKey();
        auto it = seen.find(k);
        if (it != seen.end()) out.push_back({ it->second, i, k });
        else                  seen[k] = i;
    }
    return out;
}

} // namespace nfsmw
