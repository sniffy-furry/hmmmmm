#pragma once
#include "formats/TPKBlock.h"
#include <cstdint>
#include <vector>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// Shared texture-thumbnail helper.
//
// Factored out of TexturePackPanel::GetOrCreateThumb so the car panels
// (CarTexturePanel, CarVinylPanel) get the same real DXT-decode → box-filter →
// GL-upload pipeline instead of the old hardcoded "return 0" placeholder
// (roadmap item P1-5). One canonical decoder, called from every grid.
// ─────────────────────────────────────────────────────────────────────────────

/// Decode a TPK Texture (ARGB32 / DXT1 / DXT3 / DXT5) into tightly-packed
/// RGBA8 pixels (width*height*4). Returns empty on unsupported/empty input
/// (PAL8, Unknown, zero-size, or a payload too small for the declared size).
std::vector<uint8_t> DecodeTextureToRGBA(const Texture& tex);

/// Decode `tex`, downscale to `size`×`size` with a box filter, and upload it as
/// a fresh GL texture. Returns the GL handle, or 0 when the texture can't be
/// decoded (caller should fall back to a plain text row). The caller owns the
/// returned handle and must glDeleteTextures it.
uint32_t CreateThumbnailGL(const Texture& tex, int size = 32);

} // namespace nfsmw
