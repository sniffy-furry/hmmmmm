#pragma once
// ─── NisEventSequence.h ───────────────────────────────────────────────────────
// Parser for the NIS scene event timeline — the EventSequenceChunk (0x0003B811)
// embedded in NIS/Scene_*_BundleB.bun bundles.
//
// The payload is an 8-byte 0x11 sentinel followed by a 'CARP' v26 registry
// script (stored "PRAC" on disk). It carries the scene's *timeline*: one
// per-subject action schedule with an authored `duration` and a list of timed
// ENIS*/E* events (cop lights, burnouts, overlay messages, world smacks…).
// This is the ONE part of a cutscene whose timing is fully decoded — the bone
// and camera keyframe motion itself is an 8-bit-quantised stream whose
// dequantisation math is still unsolved (see formats/NisAnim.h + encyclopedia
// C10.3). We therefore drive the player's timeline (clip length + event
// markers) from this real data, and never fabricate motion.
//
// Format + reference parser: encyclopedia C10.12 (parse_nis_events.py). This is
// a defensive port: any structural inconsistency yields `valid == false` so the
// caller falls back to a manual/default duration and shows NO markers rather
// than inventing a schedule. GL/VanGUI-free (nfsmw_core, C++20).
// ─────────────────────────────────────────────────────────────────────────────
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace nfsmw {

/// One scheduled event on the timeline. `time` is authored seconds; `label` is
/// the resolved verb (ENIS*/E*) or "0x…" when the hash doesn't resolve; `actor`
/// is the owning subject ("Car1", "Cop3", "Global", …).
struct NisTimelineEvent {
    float       time = 0.f;
    std::string label;
    std::string actor;
};

/// One per-subject action schedule (a lane on the timeline).
struct NisActionLane {
    std::string name;          ///< e.g. "ActionCar1" (or a 0x-hash if unresolved)
    std::string actor;         ///< e.g. "Car1"
    float       duration   = 0.f;
    int         eventCount  = 0;
};

/// The decoded scene timeline. When `valid` is false the chunk was absent or the
/// script did not parse cleanly; callers MUST NOT fabricate a timeline in that
/// case.
struct NisSceneTimeline {
    bool        valid    = false;
    std::string sceneName;
    float       duration = 0.f;             ///< longest action duration (seconds)
    std::vector<NisActionLane>    lanes;
    std::vector<NisTimelineEvent> events;   ///< sorted by time
    int         actionCount = 0;
    int         itemCount   = 0;            ///< timeline items whose events decoded
};

class NisEventSequenceParser {
public:
    /// Parse one EventSequenceChunk (0x0003B811) payload — the bytes after the
    /// 8-byte chunk header, i.e. starting at the 8-byte 0x11 sentinel.
    static NisSceneTimeline Parse(std::span<const uint8_t> payload);

    /// Walk a whole (decompressed) bundle buffer, find the first
    /// EventSequenceChunk, and parse it. Returns an invalid timeline if none.
    static NisSceneTimeline ParseBundle(std::span<const uint8_t> bundleBytes);
};

} // namespace nfsmw
