#include "renderer/TextureThumbnail.h"

#include "renderer/GLCompat.h"

#include <algorithm>
#include <cstring>

namespace nfsmw {

// DXT1/3/5 + ARGB32 software decoder. Mirrors the implementation that has
// lived (and been validated) in TexturePackPanel::DecodeToRGBA; lifted here so
// the car panels share one canonical path rather than re-deriving it.
std::vector<uint8_t> DecodeTextureToRGBA(const Texture& tex) {
    if (tex.data.empty() || tex.width == 0 || tex.height == 0)
        return {};

    const uint32_t W = tex.width, H = tex.height;
    std::vector<uint8_t> rgba(static_cast<size_t>(W) * H * 4, 0xFF);

    if (tex.format == TexFormat::ARGB32) {
        const size_t pixels = static_cast<size_t>(W) * H;
        for (size_t i = 0; i < pixels && (i + 1) * 4 <= tex.data.size(); ++i) {
            const uint8_t* src = tex.data.data() + i * 4;
            uint8_t* dst = rgba.data() + i * 4;
            dst[0] = src[1]; dst[1] = src[2]; dst[2] = src[3]; dst[3] = src[0];
        }
        return rgba;
    }

    if (tex.format != TexFormat::DXT1 && tex.format != TexFormat::DXT3 &&
        tex.format != TexFormat::DXT5)
        return {}; // PAL8 / Unknown — caller falls back to a text row.

    const uint32_t bw = (W + 3) / 4, bh = (H + 3) / 4;
    const size_t blockBytes = (tex.format == TexFormat::DXT1) ? 8 : 16;
    if (tex.data.size() < static_cast<size_t>(bw) * bh * blockBytes) return rgba;

    auto unpack565 = [](uint16_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
        r = (uint8_t)(((v >> 11) & 0x1F) * 255 / 31);
        g = (uint8_t)(((v >>  5) & 0x3F) * 255 / 63);
        b = (uint8_t)(((v      ) & 0x1F) * 255 / 31);
    };

    for (uint32_t by = 0; by < bh; ++by) {
        for (uint32_t bx = 0; bx < bw; ++bx) {
            const uint8_t* blk = tex.data.data() +
                (static_cast<size_t>(by) * bw + bx) * blockBytes;

            uint8_t alphaBlock[16];
            std::fill(alphaBlock, alphaBlock + 16, 0xFFu);

            if (tex.format == TexFormat::DXT3) {
                for (int i = 0; i < 8; ++i) {
                    alphaBlock[i * 2    ] = (uint8_t)((blk[i] & 0x0F) * 17);
                    alphaBlock[i * 2 + 1] = (uint8_t)(((blk[i] >> 4) & 0x0F) * 17);
                }
                blk += 8;
            } else if (tex.format == TexFormat::DXT5) {
                uint8_t a0 = blk[0], a1 = blk[1];
                uint64_t codes = 0;
                for (int i = 2; i < 8; ++i)
                    codes |= (uint64_t)blk[i] << ((i - 2) * 8);
                uint8_t atable[8];
                atable[0] = a0; atable[1] = a1;
                if (a0 > a1) {
                    for (int i = 1; i <= 6; ++i)
                        atable[i + 1] = (uint8_t)(((7 - i) * a0 + i * a1) / 7);
                } else {
                    for (int i = 1; i <= 4; ++i)
                        atable[i + 1] = (uint8_t)(((5 - i) * a0 + i * a1) / 5);
                    atable[6] = 0; atable[7] = 255;
                }
                for (int i = 0; i < 16; ++i)
                    alphaBlock[i] = atable[(codes >> (i * 3)) & 0x7];
                blk += 8;
            }

            uint16_t c0, c1;
            std::memcpy(&c0, blk, 2); std::memcpy(&c1, blk + 2, 2);
            uint32_t indices; std::memcpy(&indices, blk + 4, 4);

            uint8_t cr[4], cg[4], cb_[4], ca[4];
            unpack565(c0, cr[0], cg[0], cb_[0]); ca[0] = 0xFF;
            unpack565(c1, cr[1], cg[1], cb_[1]); ca[1] = 0xFF;

            if (tex.format == TexFormat::DXT1) {
                if (c0 > c1) {
                    cr[2]=(uint8_t)((2*cr[0]+cr[1])/3); cg[2]=(uint8_t)((2*cg[0]+cg[1])/3); cb_[2]=(uint8_t)((2*cb_[0]+cb_[1])/3); ca[2]=0xFF;
                    cr[3]=(uint8_t)((cr[0]+2*cr[1])/3); cg[3]=(uint8_t)((cg[0]+2*cg[1])/3); cb_[3]=(uint8_t)((cb_[0]+2*cb_[1])/3); ca[3]=0xFF;
                } else {
                    cr[2]=(uint8_t)((cr[0]+cr[1])/2); cg[2]=(uint8_t)((cg[0]+cg[1])/2); cb_[2]=(uint8_t)((cb_[0]+cb_[1])/2); ca[2]=0xFF;
                    cr[3]=0; cg[3]=0; cb_[3]=0; ca[3]=0;
                }
            } else {
                cr[2]=(uint8_t)((2*cr[0]+cr[1])/3); cg[2]=(uint8_t)((2*cg[0]+cg[1])/3); cb_[2]=(uint8_t)((2*cb_[0]+cb_[1])/3); ca[2]=0xFF;
                cr[3]=(uint8_t)((cr[0]+2*cr[1])/3); cg[3]=(uint8_t)((cg[0]+2*cg[1])/3); cb_[3]=(uint8_t)((cb_[0]+2*cb_[1])/3); ca[3]=0xFF;
            }

            for (int py = 0; py < 4; ++py) {
                for (int px = 0; px < 4; ++px) {
                    const uint32_t px_ = bx*4+px, py_ = by*4+py;
                    if (px_ >= W || py_ >= H) continue;
                    const int idx = (indices >> ((py*4+px)*2)) & 0x3;
                    const int pi  = py*4+px;
                    uint8_t* dst  = rgba.data() + (static_cast<size_t>(py_)*W+px_)*4;
                    dst[0]=cr[idx]; dst[1]=cg[idx];
                    dst[2]=cb_[idx]; dst[3]=(tex.format==TexFormat::DXT1)?ca[idx]:alphaBlock[pi];
                }
            }
        }
    }
    return rgba;
}

uint32_t CreateThumbnailGL(const Texture& tex, int size) {
    if (size <= 0) return 0;
    auto rgba = DecodeTextureToRGBA(tex);
    if (rgba.empty()) return 0;

    const int srcW = tex.width, srcH = tex.height;
    if (srcW <= 0 || srcH <= 0) return 0;

    std::vector<uint8_t> thumb(static_cast<size_t>(size) * size * 4, 0);
    for (int ty = 0; ty < size; ++ty) {
        for (int tx = 0; tx < size; ++tx) {
            const int sx0 = tx     * srcW / size;
            const int sx1 = (tx+1) * srcW / size;
            const int sy0 = ty     * srcH / size;
            const int sy1 = (ty+1) * srcH / size;
            uint32_t r=0, g=0, b=0, a=0, n=0;
            for (int sy = sy0; sy < std::max(sy0+1, sy1); ++sy)
                for (int sx = sx0; sx < std::max(sx0+1, sx1); ++sx) {
                    if (sx >= srcW || sy >= srcH) continue;
                    const uint8_t* p = rgba.data() +
                        (static_cast<size_t>(sy)*srcW+sx)*4;
                    r+=p[0]; g+=p[1]; b+=p[2]; a+=p[3]; ++n;
                }
            if (n == 0) n = 1;
            uint8_t* dst = thumb.data() + (static_cast<size_t>(ty)*size+tx)*4;
            dst[0]=(uint8_t)(r/n); dst[1]=(uint8_t)(g/n);
            dst[2]=(uint8_t)(b/n); dst[3]=(uint8_t)(a/n);
        }
    }

    GLuint handle = 0;
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, thumb.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return handle;
}

} // namespace nfsmw
