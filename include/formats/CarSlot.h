#pragma once
// ─── formats/CarSlot.h ────────────────────────────────────────────────────────
// Vinyl/livery layer stack from CARSLOT.BIN.
//
// MW's livery system composites up to 32 vinyl layers at runtime, each
// referencing a texture in GLOBAL/CARSKINS.BUN by hash, plus a 2-D transform
// and a colour tint applied on top of the base body paint.
//
// ┌─ TODO (binary reverse-engineering) ──────────────────────────────────────┐
// │  VinylLayer field offsets and CarSlot header layout are PLACEHOLDERS.    │
// │  Validate against CARS/<id>/CARSLOT.BIN before enabling Save().          │
// │                                                                           │
// │  Suggested workflow:                                                      │
// │    1. Pick a car with a visible vinyl (e.g. M3G race version).           │
// │    2. Open CARSLOT.BIN in a hex editor; note the CARSKINS.BUN hash of    │
// │       the first vinyl texture and search for it as a uint32 to locate    │
// │       the start of the first VinylLayer record.                           │
// │    3. Confirm posX/posY, scale, rotation, tintBGRA offsets by            │
// │       correlating with in-game layer position (pause the game, note       │
// │       where the decal sits on the hood, match to float values).           │
// │    4. Update kLayerStride and field offsets in CarSlot.cpp.              │
// └───────────────────────────────────────────────────────────────────────────┘
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// VinylLayer — one compositing layer
// ─────────────────────────────────────────────────────────────────────────────
struct VinylLayer {
    uint32_t textureHash = 0;    ///< Joaat hash — resolves into CARSKINS.BUN
    float    posX        = 0.f;  ///< normalised body-UV X position 0..1
    float    posY        = 0.f;  ///< normalised body-UV Y position 0..1
    float    scaleX      = 1.f;
    float    scaleY      = 1.f;
    float    rotation    = 0.f;  ///< radians
    uint32_t tintBGRA    = 0xFFFFFFFFu;  ///< packed colour tint (BGRA byte order)
};

// ─────────────────────────────────────────────────────────────────────────────
// CarSlot — one paint slot (one CARSLOT.BIN record)
// ─────────────────────────────────────────────────────────────────────────────
struct CarSlot {
    std::filesystem::path    path;
    std::string              carId;
    std::vector<VinylLayer>  layers;   ///< up to 32 layers, in composite order
};

// ─────────────────────────────────────────────────────────────────────────────
// CarSlotParser
// ─────────────────────────────────────────────────────────────────────────────
class CarSlotParser {
public:
    /// Parse vinyl layer stack from CARSLOT.BIN.
    /// Returns Err() with a stub-notice until on-disk layout is confirmed.
    static Result<CarSlot>  Load(const std::filesystem::path& path,
                                 const std::string& carId);

    /// Write modified layer stack back to CARSLOT.BIN in-place.
    /// Stub: writes nothing and returns Err() until Load() is implemented.
    static Result<void>     Save(const CarSlot& slot);
};

} // namespace nfsmw
