#pragma once
// ─── formats/CarInfo.h ────────────────────────────────────────────────────────
// Performance, handling, gearbox, and suspension data from CARINFO.BIN.
// Also houses CarPursuit (pursuit-specific heat table), which lives in the
// continuation of the same binary record.
//
// ┌─ TODO (binary reverse-engineering) ──────────────────────────────────────┐
// │  The struct field offsets below are PLACEHOLDERS derived from the design  │
// │  document.  They must be validated against a hex dump of a known car's    │
// │  CARINFO.BIN before CarInfoParser::Load() will return correct values.     │
// │                                                                           │
// │  Suggested verification workflow:                                         │
// │    1. Open CARS/M3G/CARINFO.BIN in a hex editor.                         │
// │    2. The BMW M3 GTR top speed is ~160 mph — search for 0x43200000        │
// │       (160.0f little-endian) to anchor kOff_TopSpeedMph.                 │
// │    3. Walk the remaining fields from that anchor.                         │
// │    4. Update kOff_* constants and kRecordSize in CarInfo.cpp.             │
// │    5. Replace STUB_FLOAT / STUB_U32 sentinel returns in Load().           │
// └───────────────────────────────────────────────────────────────────────────┘
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include <filesystem>
#include <string>
#include <array>
#include <cstdint>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// CarInfo — performance and handling data
// ─────────────────────────────────────────────────────────────────────────────
struct CarInfo {
    std::filesystem::path path;
    std::string           carId;

    // Engine
    float    topSpeedMph    = 0.f;   ///< maximum speed
    float    accel0_60      = 0.f;   ///< 0–60 mph time in seconds
    float    engineTorque   = 0.f;   ///< Nm
    float    engineRPM      = 0.f;   ///< redline
    float    turboBoost     = 0.f;   ///< multiplier (0 = naturally aspirated)

    // Gearbox
    uint32_t             gearCount  = 0;
    std::array<float, 8> gearRatios = {};  ///< [0]=reverse, [1..N]=forward
    float                finalDrive = 0.f;

    // Handling
    float mass            = 0.f;   ///< kg
    float frontWeightBias = 0.f;   ///< fraction 0..1
    float frontDownforce  = 0.f;
    float rearDownforce   = 0.f;
    float steeringLock    = 0.f;   ///< degrees
    float brakeBias       = 0.f;   ///< fraction 0..1 (front)
    float brakeStrength   = 0.f;

    // Suspension (also used for wheel placement in CarViewerPanel)
    float frontSuspHeight = 0.f;
    float rearSuspHeight  = 0.f;
    float frontSuspTrack  = 0.f;   ///< half-width, metres
    float rearSuspTrack   = 0.f;

    // Tyre
    float tyreFrictionFront = 0.f;
    float tyreFrictionRear  = 0.f;
    float tyreWidth         = 0.f;  ///< metres
};

// ─────────────────────────────────────────────────────────────────────────────
// CarPursuit — pursuit aggression and heat table (continuation of CARINFO.BIN)
// ─────────────────────────────────────────────────────────────────────────────
struct HeatEntry {
    uint32_t heatLevel       = 0;    ///< 1..5
    float    aggressionScale = 0.f;  ///< multiplier applied to base pursuit AI
    float    trafficDensity  = 0.f;  ///< 0..1 fractional override
    uint32_t flagsUnlocked   = 0;    ///< bitmask: roadblocks | spike strips | helicopters
};

namespace HeatFlags {
    constexpr uint32_t Roadblocks  = 1u << 0;
    constexpr uint32_t SpikeStrips = 1u << 1;
    constexpr uint32_t Helicopters = 1u << 2;
}

struct CarPursuit {
    std::filesystem::path    path;
    std::string              carId;
    uint32_t                 baseHeat      = 0;    ///< heat level when car first appears
    float                    bustedPenalty = 0.f;  ///< fine multiplier
    float                    escapeRadius  = 0.f;  ///< metres before pursuit drops
    std::array<HeatEntry, 5> heatTable     = {};
};

// ─────────────────────────────────────────────────────────────────────────────
// CarInfoParser
// ─────────────────────────────────────────────────────────────────────────────
class CarInfoParser {
public:
    /// Load performance + handling fields from CARINFO.BIN.
    /// Returns Err() with a stub-notice until offsets are confirmed.
    static Result<CarInfo>   Load(const std::filesystem::path& path,
                                  const std::string& carId);

    /// Write modified fields back to CARINFO.BIN.
    /// Stub: writes nothing and returns Err() until Load() is implemented.
    static Result<void>      Save(const CarInfo& info);
};

// ─────────────────────────────────────────────────────────────────────────────
// CarPursuitParser
// ─────────────────────────────────────────────────────────────────────────────
class CarPursuitParser {
public:
    /// Load pursuit fields from CARINFO.BIN (non-overlapping region vs CarInfo).
    /// Returns Err() with a stub-notice until offsets are confirmed.
    static Result<CarPursuit> Load(const std::filesystem::path& path,
                                   const std::string& carId);

    /// Write modified pursuit fields back to CARINFO.BIN.
    /// Stub: writes nothing and returns Err() until Load() is implemented.
    static Result<void>       Save(const CarPursuit& pursuit);
};

} // namespace nfsmw
