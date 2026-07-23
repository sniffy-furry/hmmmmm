#include "core/FlagReference.h"
#include <array>
#include <cstdio>

namespace nfsmw {

namespace {

struct BitDef { uint32_t mask; const char* label; FlagConfidence conf; };

// Scenery flagsA — see header for the flagscan evidence behind each entry.
constexpr std::array<BitDef, 12> kSceneryA = {{
    { 0x00008000, "active / placed scenery",        FlagConfidence::Strong },
    { 0x00000010, "animated (ANM_)",                FlagConfidence::Candidate },
    { 0x00000020, "surface/detail sub-flag",        FlagConfidence::Candidate },
    { 0x00000040, "surface/detail sub-flag",        FlagConfidence::Candidate },
    { 0x00000080, "surface/detail sub-flag",        FlagConfidence::Candidate },
    { 0x00040000, "road / path class",              FlagConfidence::Candidate },
    { 0x00100000, "structure class",                FlagConfidence::Candidate },
    { 0x00200000, "vegetation / terrain class",     FlagConfidence::Candidate },
    { 0x00400000, "building class",                 FlagConfidence::Candidate },
    { 0x02000000, "street-furniture class",         FlagConfidence::Candidate },
    { 0x08000000, "wall class",                     FlagConfidence::Candidate },
    { 0x40000000, "object class",                   FlagConfidence::Candidate },
}};

// Geometry SolidObject.flags.
constexpr std::array<BitDef, 3> kGeometry = {{
    { 0x00000100, "base render (lit/opaque; road=always)", FlagConfidence::Strong },
    { 0x00000200, "render variant",                        FlagConfidence::Candidate },
    { 0x00000400, "render variant",                        FlagConfidence::Candidate },
}};

void AppendKnown(std::vector<DecodedFlag>& out, uint32_t value,
                 const BitDef* defs, size_t n, uint32_t& consumed) {
    for (size_t i = 0; i < n; ++i) {
        if (value & defs[i].mask) {
            out.push_back({ defs[i].mask, defs[i].label, defs[i].conf });
            consumed |= defs[i].mask;
        }
    }
}

void AppendUnmapped(std::vector<DecodedFlag>& out, uint32_t value, uint32_t consumed) {
    const uint32_t leftover = value & ~consumed;
    if (!leftover) return;
    char buf[48];
    std::snprintf(buf, sizeof buf, "unmapped bits 0x%08X", leftover);
    out.push_back({ leftover, buf, FlagConfidence::Candidate });
}

} // namespace

const char* FlagConfidenceTag(FlagConfidence c) {
    switch (c) {
        case FlagConfidence::Strong:    return "strong";
        case FlagConfidence::Candidate: return "candidate";
        case FlagConfidence::Reserved:  return "reserved";
    }
    return "";
}

std::vector<DecodedFlag> AllKnownFlags(FlagField field) {
    std::vector<DecodedFlag> out;
    auto fill = [&](const BitDef* a, size_t n) {
        for (size_t i = 0; i < n; ++i) out.push_back({ a[i].mask, a[i].label, a[i].conf });
    };
    switch (field) {
        case FlagField::GeometryFlags: fill(kGeometry.data(), kGeometry.size()); break;
        case FlagField::SceneryFlagsA: fill(kSceneryA.data(), kSceneryA.size()); break;
        case FlagField::SceneryFlagsB: break;   // sentinel/id, no per-bit toggles
    }
    return out;
}

std::vector<DecodedFlag> DecodeFlags(FlagField field, uint32_t value) {
    std::vector<DecodedFlag> out;
    uint32_t consumed = 0;

    switch (field) {
        case FlagField::GeometryFlags:
            AppendKnown(out, value, kGeometry.data(), kGeometry.size(), consumed);
            AppendUnmapped(out, value, consumed);
            break;

        case FlagField::SceneryFlagsA:
            AppendKnown(out, value, kSceneryA.data(), kSceneryA.size(), consumed);
            AppendUnmapped(out, value, consumed);
            break;

        case FlagField::SceneryFlagsB:
            // High word is a constant 0xFFFF sentinel on all retail data; the
            // low word is a small per-instance id / sub-flag set.
            if ((value & 0xFFFF0000u) == 0xFFFF0000u) {
                out.push_back({ 0xFFFF0000u, "reserved sentinel (0xFFFF)",
                                FlagConfidence::Reserved });
            } else if (value & 0xFFFF0000u) {
                char buf[48];
                std::snprintf(buf, sizeof buf, "high word 0x%04X (expected 0xFFFF)",
                              value >> 16);
                out.push_back({ 0xFFFF0000u, buf, FlagConfidence::Candidate });
            }
            if (value & 0x0000FFFFu) {
                char buf[48];
                std::snprintf(buf, sizeof buf, "id / sub-flags = 0x%04X",
                              value & 0xFFFFu);
                out.push_back({ 0x0000FFFFu, buf, FlagConfidence::Candidate });
            }
            break;
    }
    return out;
}

} // namespace nfsmw
