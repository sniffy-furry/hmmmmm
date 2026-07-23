// DXT block-size invariants — guards the 128x128 DXT3 = 16384 regression
// (the /2 DXT1 formula once caused an out-of-bounds read on every minimap load).
#include "microtest.h"
#include "formats/MinimapFile.h"
#include <cstdint>

using namespace nfsmw;

// Block-compressed surface size: blocks(ceil w/4 * ceil h/4) * bytesPerBlock.
static uint32_t dxt_size(uint32_t w, uint32_t h, uint32_t blockBytes) {
    uint32_t bx = (w + 3) / 4, by = (h + 3) / 4;
    if (bx == 0) bx = 1; if (by == 0) by = 1;
    return bx * by * blockBytes;
}

TEST(minimap_tile_is_dxt3_16384) {
    // The fixed constant in the minimap parser must equal 128*128 (DXT3 = 1 B/px).
    CHECK_EQ((uint32_t)kTilePixelBytes, 16384u);
    CHECK_EQ((uint32_t)kTileWidth,  128u);
    CHECK_EQ((uint32_t)kTileHeight, 128u);
    CHECK_EQ((uint32_t)kTilePixelBytes, dxt_size(128, 128, 16)); // DXT3 = 16 B / 4x4 block
}

TEST(dxt_block_size_formula) {
    CHECK_EQ(dxt_size(128, 128, 8),  8192u);   // DXT1: 0.5 B/px
    CHECK_EQ(dxt_size(128, 128, 16), 16384u);  // DXT3/DXT5: 1 B/px
    CHECK_EQ(dxt_size(256, 256, 16), 65536u);
    CHECK_EQ(dxt_size(1,   1,   8),  8u);       // sub-block rounds up to one block
    CHECK_EQ(dxt_size(5,   5,   8),  4u * 8u);  // ceil(5/4)=2 -> 2*2 blocks
}
