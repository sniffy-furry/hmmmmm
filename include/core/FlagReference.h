#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// FlagReference — evidence-based decoder for the raw object flag fields.
//
// SolidObject.flags (geometry) and SceneryInstance flagsA/flagsB are stored raw.
// Their meanings were derived by correlating every object on the retail Rockport
// track against its model-name category (MapCLI `flagscan` on TRACKS/L2RA.BUN),
// NOT from an authoritative schema — so each decoded bit carries an explicit
// confidence. Strong = near-universal / exclusive in the data; Candidate =
// correlated with a category but not exclusive; Reserved = constant on all
// retail data. In-game probing is still needed to promote Candidates to facts.
//
// This is the same table the NFSMapEditor Properties panel uses; it is ported
// here (GL/VanGUI-free, C++20) so the MWTools object viewer can present and edit
// the geometry flags with named, confidence-tagged bits instead of raw hex.
//
// Findings the decode below is built on:
//   scenery flagsB : high word is ALWAYS 0xFFFF (reserved); low word a small id
//   scenery flagsA : bit 0x00008000 set on ~every placed object (active);
//                    low bits 0x20/0x40/0x80 are surface/detail sub-flags;
//                    high bits correlate with class — road 0x00040000,
//                    structure 0x00100000/0x00200000/0x00400000,
//                    street-furniture/object 0x02000000/0x40000000, wall 0x08000000
//   geometry flags : road == 0x00000100 (always); 0x200/0x400 render variants
// ─────────────────────────────────────────────────────────────────────────────

enum class FlagField : uint8_t {
    GeometryFlags,   ///< SolidObject.flags
    SceneryFlagsA,   ///< SceneryInstance.flagsA
    SceneryFlagsB,   ///< SceneryInstance.flagsB
};

enum class FlagConfidence : uint8_t {
    Strong,     ///< near-universal or exclusive in the retail correlation
    Candidate,  ///< correlated with a category, not exclusive — needs probing
    Reserved,   ///< constant across all retail data
};

struct DecodedFlag {
    uint32_t       mask  = 0;     ///< the bit(s) this annotation covers
    std::string    label;         ///< human-readable meaning
    FlagConfidence confidence = FlagConfidence::Candidate;
};

const char* FlagConfidenceTag(FlagConfidence c);

/// Decode the SET, known bits of `value` for the given field into annotations.
/// Bits that are set but not recognised are reported once as a trailing
/// "unmapped bits 0x…" entry (confidence Candidate) so nothing is hidden.
std::vector<DecodedFlag> DecodeFlags(FlagField field, uint32_t value);

/// Every named flag defined for a field (whether set or not) — lets the UI
/// present each as a labelled, toggleable checkbox instead of raw hex.
std::vector<DecodedFlag> AllKnownFlags(FlagField field);

} // namespace nfsmw
