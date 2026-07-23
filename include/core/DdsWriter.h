#pragma once
// ─── core/DdsWriter.h ─────────────────────────────────────────────────────────
// Build a standard 128-byte DirectDraw Surface (DDS) file in memory from raw
// texture pixels. Header-only, GL/GLM-free, dependency-free — usable from the
// CLI, the patch layer, or tests, and trivially unit-testable.
//
// Supports the formats MW textures actually use: DXT1/DXT3/DXT5 (FourCC) and
// uncompressed 32-bit colour. Mip count is written but the caller supplies the
// full pixel chain. See encyclopedia Chapter 4 for the size math.
// ─────────────────────────────────────────────────────────────────────────────
#include <cstdint>
#include <cstring>
#include <vector>
#include <span>

namespace nfsmw {

enum class DdsFormat { DXT1, DXT3, DXT5, ARGB32 };

inline std::vector<uint8_t> BuildDDS(uint32_t width, uint32_t height,
                                     DdsFormat fmt,
                                     std::span<const uint8_t> pixels,
                                     uint32_t mipCount = 0) {
    auto put32 = [](std::vector<uint8_t>& b, uint32_t v) {
        b.push_back(v & 0xFF); b.push_back((v >> 8) & 0xFF);
        b.push_back((v >> 16) & 0xFF); b.push_back((v >> 24) & 0xFF);
    };

    const bool compressed = (fmt != DdsFormat::ARGB32);

    // dwFlags: CAPS|HEIGHT|WIDTH|PIXELFORMAT + (LINEARSIZE for DXT | PITCH for raw)
    uint32_t flags = 0x1 | 0x2 | 0x4 | 0x1000 | (compressed ? 0x80000u : 0x8u);
    if (mipCount > 1) flags |= 0x20000u;  // DDSD_MIPMAPCOUNT

    uint32_t blockBytes = (fmt == DdsFormat::DXT1) ? 8u : 16u;
    uint32_t bx = (width + 3) / 4, by = (height + 3) / 4;
    if (bx == 0) bx = 1; if (by == 0) by = 1;
    uint32_t linearOrPitch = compressed ? (bx * by * blockBytes) : (width * 4u);

    std::vector<uint8_t> b;
    b.reserve(128 + pixels.size());
    b.insert(b.end(), { 'D', 'D', 'S', ' ' });
    put32(b, 124);            // dwSize
    put32(b, flags);
    put32(b, height);
    put32(b, width);
    put32(b, linearOrPitch);
    put32(b, 0);              // depth
    put32(b, mipCount);
    for (int i = 0; i < 11; ++i) put32(b, 0);  // reserved1[11]

    // DDS_PIXELFORMAT (32 bytes)
    put32(b, 32);             // dwSize
    if (compressed) {
        put32(b, 0x4);        // DDPF_FOURCC
        const char* cc = (fmt == DdsFormat::DXT1) ? "DXT1" : (fmt == DdsFormat::DXT3) ? "DXT3" : "DXT5";
        b.insert(b.end(), cc, cc + 4);
        put32(b, 0); put32(b, 0); put32(b, 0); put32(b, 0); put32(b, 0);
    } else {
        put32(b, 0x41);       // DDPF_RGB | DDPF_ALPHAPIXELS
        put32(b, 0);          // no FourCC
        put32(b, 32);         // RGB bit count
        put32(b, 0x00FF0000); // R mask (BGRA in memory -> standard DDS A8R8G8B8)
        put32(b, 0x0000FF00); // G
        put32(b, 0x000000FF); // B
        put32(b, 0xFF000000); // A
    }

    put32(b, 0x1000);         // dwCaps = DDSCAPS_TEXTURE
    put32(b, 0); put32(b, 0); put32(b, 0); // caps2/3/4
    put32(b, 0);              // reserved2
    // header is now exactly 128 bytes
    b.insert(b.end(), pixels.begin(), pixels.end());
    return b;
}

} // namespace nfsmw
