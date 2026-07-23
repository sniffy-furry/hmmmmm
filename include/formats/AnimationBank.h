#pragma once
#include "Common.h"
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// AnimationBank — inventory of the game's world-ambient / gameplay animation
// banks (the "unhooked" animations: tower cranes, sailboat, cargo ship, blimp,
// airliners). All three animation systems (cutscene, world-ambient, gameplay)
// serialise to the same __AnimationBank ELF; each clip is a named animation
// split into _q (rotation), _t (translation) and _s (scale) channels, and the
// clip name encodes which world object it drives:
//
//     ANM_<AnimObject>_<NN>_XO_<BoundObject>_..._q|_t|_s
//     e.g. ANM_AirlinerA_10_XO_AirlinerA_1b_LB_10_q
//
// This parser recovers that binding from a loaded bundle so the object viewer
// can tell which objects are animated, by which bank, on which channels — and
// drive their motion in the preview. GL/VanGUI-free (nfsmw_core, C++20).
// ─────────────────────────────────────────────────────────────────────────────

enum class AnimChannel : uint8_t { None, Rotation /*_q*/, Translation /*_t*/, Scale /*_s*/ };

inline const char* AnimChannelName(AnimChannel c) {
    switch (c) {
        case AnimChannel::Rotation:    return "rotation";
        case AnimChannel::Translation: return "translation";
        case AnimChannel::Scale:       return "scale";
        default:                       return "?";
    }
}

/// One clip parsed out of a bank's name table.
struct AnimClip {
    std::string name;         ///< full symbol, e.g. ANM_AirlinerA_10_XO_AirlinerA_1b_LB_10_q
    std::string animObject;   ///< the ANM_ object stem, e.g. "AirlinerA"
    std::string boundObject;  ///< the XO_ object it drives, e.g. "AirlinerA_1b_LB_10"
    AnimChannel channel = AnimChannel::None;
};

/// Channels present for one animated object (bitwise OR of 1<<AnimChannel).
struct AnimatedObject {
    std::string object;       ///< bound object stem (matched against SolidObject names)
    bool hasRotation    = false;
    bool hasTranslation = false;
    bool hasScale       = false;
    int  clipCount      = 0;
};

/// Everything recovered from a bundle's animation banks.
struct AnimBankInventory {
    std::vector<AnimClip>       clips;    ///< every _q/_t/_s clip found
    std::vector<AnimatedObject> objects;  ///< grouped by bound object

    bool empty() const { return clips.empty(); }

    /// Find the animation record for a SolidObject name (case-insensitive,
    /// substring match against the bound-object stem). nullptr if not animated.
    const AnimatedObject* ForObjectName(const std::string& solidName) const;
};

class AnimationBankParser {
public:
    /// Scan a raw byte buffer for __AnimationBank clip names and build the
    /// inventory. Works on decompressed bundle bytes.
    static AnimBankInventory Scan(std::span<const uint8_t> bytes);

    /// Read `path` (JDLZ-decompressing the whole file if needed) and scan it.
    static Result<AnimBankInventory> ScanFile(const std::filesystem::path& path);
};

} // namespace nfsmw
