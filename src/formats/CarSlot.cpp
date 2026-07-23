// ─── formats/CarSlot.cpp ──────────────────────────────────────────────────────
#include "formats/CarSlot.h"
#include <fstream>
#include <cstring>

// ┌─ TODO: binary offsets ────────────────────────────────────────────────────┐
// │  CARSLOT.BIN layout is unverified.  See the header TODO block for the     │
// │  validation workflow before enabling Load() / Save().                      │
// │                                                                            │
// │  Once confirmed, set kLayerStride and kOff_* constants below, remove the  │
// │  early-return stubs, and set CarContext::perfReady after Load().           │
// └────────────────────────────────────────────────────────────────────────────┘

namespace nfsmw {

namespace {
    constexpr uint32_t kMaxLayers   = 32;
    constexpr uint32_t kLayerStride = 0;  // TODO: size in bytes of one VinylLayer record
    constexpr uint32_t kOff_Header  = 0;  // TODO: offset of layer count or first layer

    // VinylLayer field offsets relative to start of each layer record
    constexpr uint32_t kOff_TextureHash = 0; // TODO
    constexpr uint32_t kOff_PosX        = 0; // TODO
    constexpr uint32_t kOff_PosY        = 0; // TODO
    constexpr uint32_t kOff_ScaleX      = 0; // TODO
    constexpr uint32_t kOff_ScaleY      = 0; // TODO
    constexpr uint32_t kOff_Rotation    = 0; // TODO
    constexpr uint32_t kOff_TintBGRA    = 0; // TODO

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

    Result<std::vector<uint8_t>> ReadAll(const std::filesystem::path& path) {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) return Result<std::vector<uint8_t>>::Err("Cannot open " + path.string());
        auto sz = static_cast<size_t>(f.tellg());
        f.seekg(0);
        std::vector<uint8_t> buf(sz);
        if (!f.read(reinterpret_cast<char*>(buf.data()), sz))
            return Result<std::vector<uint8_t>>::Err("Read error: " + path.string());
        return Result<std::vector<uint8_t>>::Ok(std::move(buf));
    }
} // namespace

Result<CarSlot> CarSlotParser::Load(const std::filesystem::path& path,
                                     const std::string& carId) {
    // ── STUB ─────────────────────────────────────────────────────────────────
    (void)path; (void)carId;
    return Result<CarSlot>::Err(
        "[STUB] CarSlotParser::Load — CARSLOT.BIN layer layout not yet verified. "
        "See formats/CarSlot.cpp TODO block.");

    // TODO: remove early return above, then uncomment:
    /*
    auto raw = ReadAll(path);
    if (!raw) return Result<CarSlot>::Err(raw.error);
    const auto& buf = raw.value;

    CarSlot slot;
    slot.path  = path;
    slot.carId = carId;

    // TODO: read layer count from header once its offset is known
    uint32_t layerCount = 0; // ReadU32(buf, kOff_Header);
    layerCount = std::min(layerCount, kMaxLayers);

    for (uint32_t i = 0; i < layerCount; ++i) {
        uint32_t base = kOff_Header + i * kLayerStride; // TODO: adjust
        VinylLayer vl;
        vl.textureHash = ReadU32(buf, base + kOff_TextureHash);
        vl.posX        = ReadF32(buf, base + kOff_PosX);
        vl.posY        = ReadF32(buf, base + kOff_PosY);
        vl.scaleX      = ReadF32(buf, base + kOff_ScaleX);
        vl.scaleY      = ReadF32(buf, base + kOff_ScaleY);
        vl.rotation    = ReadF32(buf, base + kOff_Rotation);
        vl.tintBGRA    = ReadU32(buf, base + kOff_TintBGRA);
        slot.layers.push_back(vl);
    }
    return Result<CarSlot>::Ok(std::move(slot));
    */
}

Result<void> CarSlotParser::Save(const CarSlot& slot) {
    // ── STUB ─────────────────────────────────────────────────────────────────
    (void)slot;
    return Result<void>::Err(
        "[STUB] CarSlotParser::Save — not implemented until Load() is verified.");

    // TODO: uncomment and adapt once kOff_* constants are known:
    /*
    auto raw = ReadAll(slot.path);
    if (!raw) return Result<void>::Err(raw.error);
    auto& buf = raw.value;

    for (uint32_t i = 0; i < slot.layers.size() && i < kMaxLayers; ++i) {
        uint32_t base = kOff_Header + i * kLayerStride; // TODO: adjust
        const auto& vl = slot.layers[i];
        WriteU32(buf, base + kOff_TextureHash, vl.textureHash);
        WriteF32(buf, base + kOff_PosX,        vl.posX);
        WriteF32(buf, base + kOff_PosY,        vl.posY);
        WriteF32(buf, base + kOff_ScaleX,      vl.scaleX);
        WriteF32(buf, base + kOff_ScaleY,      vl.scaleY);
        WriteF32(buf, base + kOff_Rotation,    vl.rotation);
        WriteU32(buf, base + kOff_TintBGRA,    vl.tintBGRA);
    }

    std::ofstream f(slot.path, std::ios::binary | std::ios::trunc);
    if (!f) return Result<void>::Err("Cannot write " + slot.path.string());
    f.write(reinterpret_cast<const char*>(buf.data()), buf.size());
    return Result<void>::Ok();
    */
}

} // namespace nfsmw
