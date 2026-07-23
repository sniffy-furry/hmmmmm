// ─── formats/CarInfo.cpp ──────────────────────────────────────────────────────
#include "formats/CarInfo.h"
#include "core/BINFile.h"
#include <fstream>
#include <cstring>
#include <cstdio>

// ┌─ TODO: binary offsets ────────────────────────────────────────────────────┐
// │  Replace every kOff_* constant with the correct byte offset verified      │
// │  against a real CARINFO.BIN (see header for validation workflow).          │
// │  kRecordSize must also be confirmed.                                       │
// │                                                                            │
// │  Once offsets are known:                                                   │
// │    1. Remove the early-return stubs from Load() / Save().                  │
// │    2. Set CarContext::perfReady = true in CarPanel after a successful load. │
// │    3. Clear stubMode_ in CarPerfPanel / CarPursuitPanel.                   │
// └────────────────────────────────────────────────────────────────────────────┘

namespace nfsmw {

// ── Placeholder record layout ────────────────────────────────────────────────
// All offsets are UNVERIFIED.  Do not enable Load()/Save() until confirmed.
namespace {
    constexpr uint32_t kRecordSize = 0; // TODO: set to actual CARINFO.BIN record size

    // CarInfo offsets
    constexpr uint32_t kOff_TopSpeedMph      = 0; // TODO
    constexpr uint32_t kOff_Accel0_60        = 0; // TODO
    constexpr uint32_t kOff_EngineTorque     = 0; // TODO
    constexpr uint32_t kOff_EngineRPM        = 0; // TODO
    constexpr uint32_t kOff_TurboBoost       = 0; // TODO
    constexpr uint32_t kOff_GearCount        = 0; // TODO
    constexpr uint32_t kOff_GearRatios       = 0; // TODO (8 * float)
    constexpr uint32_t kOff_FinalDrive       = 0; // TODO
    constexpr uint32_t kOff_Mass             = 0; // TODO
    constexpr uint32_t kOff_FrontWeightBias  = 0; // TODO
    constexpr uint32_t kOff_FrontDownforce   = 0; // TODO
    constexpr uint32_t kOff_RearDownforce    = 0; // TODO
    constexpr uint32_t kOff_SteeringLock     = 0; // TODO
    constexpr uint32_t kOff_BrakeBias        = 0; // TODO
    constexpr uint32_t kOff_BrakeStrength    = 0; // TODO
    constexpr uint32_t kOff_FrontSuspHeight  = 0; // TODO
    constexpr uint32_t kOff_RearSuspHeight   = 0; // TODO
    constexpr uint32_t kOff_FrontSuspTrack   = 0; // TODO
    constexpr uint32_t kOff_RearSuspTrack    = 0; // TODO
    constexpr uint32_t kOff_TyreFricFront    = 0; // TODO
    constexpr uint32_t kOff_TyreFricRear     = 0; // TODO
    constexpr uint32_t kOff_TyreWidth        = 0; // TODO

    // CarPursuit offsets (non-overlapping region of the same record)
    constexpr uint32_t kOff_BaseHeat         = 0; // TODO
    constexpr uint32_t kOff_BustedPenalty    = 0; // TODO
    constexpr uint32_t kOff_EscapeRadius     = 0; // TODO
    constexpr uint32_t kOff_HeatTable        = 0; // TODO (5 * kHeatEntryStride)
    constexpr uint32_t kHeatEntryStride      = 0; // TODO

    // Heat entry field offsets relative to the start of each entry
    constexpr uint32_t kHeat_HeatLevel       = 0; // TODO
    constexpr uint32_t kHeat_AggressionScale = 0; // TODO
    constexpr uint32_t kHeat_TrafficDensity  = 0; // TODO
    constexpr uint32_t kHeat_FlagsUnlocked   = 0; // TODO

    // ── Helpers to read primitives out of a byte buffer ──────────────────────
    float ReadF32(const std::vector<uint8_t>& buf, uint32_t off) {
        float v; std::memcpy(&v, buf.data() + off, 4); return v;
    }
    uint32_t ReadU32(const std::vector<uint8_t>& buf, uint32_t off) {
        uint32_t v; std::memcpy(&v, buf.data() + off, 4); return v;
    }
    void WriteF32(std::vector<uint8_t>& buf, uint32_t off, float v) {
        std::memcpy(buf.data() + off, &v, 4);
    }
    void WriteU32(std::vector<uint8_t>& buf, uint32_t off, uint32_t v) {
        std::memcpy(buf.data() + off, &v, 4);
    }

    /// Load the raw CARINFO.BIN record into a byte buffer.
    Result<std::vector<uint8_t>> ParseRaw(const std::filesystem::path& path) {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) return Result<std::vector<uint8_t>>::Err("Cannot open " + path.string());
        auto sz = static_cast<size_t>(f.tellg());
        f.seekg(0);
        std::vector<uint8_t> buf(sz);
        if (!f.read(reinterpret_cast<char*>(buf.data()), sz))
            return Result<std::vector<uint8_t>>::Err("Read error: " + path.string());
        return Result<std::vector<uint8_t>>::Ok(std::move(buf));
    }

    Result<void> WriteRaw(const std::filesystem::path& path,
                           const std::vector<uint8_t>& buf) {
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        if (!f) return Result<void>::Err("Cannot write " + path.string());
        f.write(reinterpret_cast<const char*>(buf.data()), buf.size());
        return Result<void>::Ok();
    }
} // namespace

// ── CarInfoParser ─────────────────────────────────────────────────────────────

Result<CarInfo> CarInfoParser::Load(const std::filesystem::path& path,
                                    const std::string& carId) {
    // ── STUB ─────────────────────────────────────────────────────────────────
    // Binary offsets in CARINFO.BIN have not been verified yet.
    // Remove this early return and fill in the ReadF32/ReadU32 calls below
    // once kOff_* constants are confirmed.
    // ─────────────────────────────────────────────────────────────────────────
    (void)path; (void)carId;
    return Result<CarInfo>::Err(
        "[STUB] CarInfoParser::Load — CARINFO.BIN offsets not yet verified. "
        "See formats/CarInfo.cpp TODO block.");

    // TODO: remove early return above, then uncomment and fill:
    /*
    auto raw = ParseRaw(path);
    if (!raw) return Result<CarInfo>::Err(raw.error);
    const auto& buf = raw.value;
    if (buf.size() < kRecordSize)
        return Result<CarInfo>::Err("CARINFO.BIN too small for expected record");

    CarInfo ci;
    ci.path  = path;
    ci.carId = carId;

    ci.topSpeedMph     = ReadF32(buf, kOff_TopSpeedMph);
    ci.accel0_60       = ReadF32(buf, kOff_Accel0_60);
    ci.engineTorque    = ReadF32(buf, kOff_EngineTorque);
    ci.engineRPM       = ReadF32(buf, kOff_EngineRPM);
    ci.turboBoost      = ReadF32(buf, kOff_TurboBoost);
    ci.gearCount       = ReadU32(buf, kOff_GearCount);
    for (int i = 0; i < 8; ++i)
        ci.gearRatios[i] = ReadF32(buf, kOff_GearRatios + i * 4);
    ci.finalDrive      = ReadF32(buf, kOff_FinalDrive);
    ci.mass            = ReadF32(buf, kOff_Mass);
    ci.frontWeightBias = ReadF32(buf, kOff_FrontWeightBias);
    ci.frontDownforce  = ReadF32(buf, kOff_FrontDownforce);
    ci.rearDownforce   = ReadF32(buf, kOff_RearDownforce);
    ci.steeringLock    = ReadF32(buf, kOff_SteeringLock);
    ci.brakeBias       = ReadF32(buf, kOff_BrakeBias);
    ci.brakeStrength   = ReadF32(buf, kOff_BrakeStrength);
    ci.frontSuspHeight = ReadF32(buf, kOff_FrontSuspHeight);
    ci.rearSuspHeight  = ReadF32(buf, kOff_RearSuspHeight);
    ci.frontSuspTrack  = ReadF32(buf, kOff_FrontSuspTrack);
    ci.rearSuspTrack   = ReadF32(buf, kOff_RearSuspTrack);
    ci.tyreFrictionFront = ReadF32(buf, kOff_TyreFricFront);
    ci.tyreFrictionRear  = ReadF32(buf, kOff_TyreFricRear);
    ci.tyreWidth       = ReadF32(buf, kOff_TyreWidth);

    return Result<CarInfo>::Ok(std::move(ci));
    */
}

Result<void> CarInfoParser::Save(const CarInfo& info) {
    // ── STUB ─────────────────────────────────────────────────────────────────
    (void)info;
    return Result<void>::Err(
        "[STUB] CarInfoParser::Save — not implemented until Load() is verified.");

    // TODO: uncomment and adapt once offsets are known:
    /*
    auto raw = ParseRaw(info.path);
    if (!raw) return Result<void>::Err(raw.error);
    auto& buf = raw.value;

    WriteF32(buf, kOff_TopSpeedMph,     info.topSpeedMph);
    WriteF32(buf, kOff_Accel0_60,       info.accel0_60);
    // ... all fields ...
    return WriteRaw(info.path, buf);
    */
}

// ── CarPursuitParser ──────────────────────────────────────────────────────────

Result<CarPursuit> CarPursuitParser::Load(const std::filesystem::path& path,
                                           const std::string& carId) {
    // ── STUB ─────────────────────────────────────────────────────────────────
    (void)path; (void)carId;
    return Result<CarPursuit>::Err(
        "[STUB] CarPursuitParser::Load — CARINFO.BIN offsets not yet verified. "
        "See formats/CarInfo.cpp TODO block.");

    // TODO: uncomment once kOff_* constants for the pursuit region are known:
    /*
    auto raw = ParseRaw(path);
    if (!raw) return Result<CarPursuit>::Err(raw.error);
    const auto& buf = raw.value;

    CarPursuit cp;
    cp.path  = path;
    cp.carId = carId;
    cp.baseHeat      = ReadU32(buf, kOff_BaseHeat);
    cp.bustedPenalty = ReadF32(buf, kOff_BustedPenalty);
    cp.escapeRadius  = ReadF32(buf, kOff_EscapeRadius);
    for (int i = 0; i < 5; ++i) {
        uint32_t base = kOff_HeatTable + i * kHeatEntryStride;
        cp.heatTable[i].heatLevel       = ReadU32(buf, base + kHeat_HeatLevel);
        cp.heatTable[i].aggressionScale = ReadF32(buf, base + kHeat_AggressionScale);
        cp.heatTable[i].trafficDensity  = ReadF32(buf, base + kHeat_TrafficDensity);
        cp.heatTable[i].flagsUnlocked   = ReadU32(buf, base + kHeat_FlagsUnlocked);
    }
    return Result<CarPursuit>::Ok(std::move(cp));
    */
}

Result<void> CarPursuitParser::Save(const CarPursuit& pursuit) {
    // ── STUB ─────────────────────────────────────────────────────────────────
    (void)pursuit;
    return Result<void>::Err(
        "[STUB] CarPursuitParser::Save — not implemented until Load() is verified.");
}

} // namespace nfsmw
