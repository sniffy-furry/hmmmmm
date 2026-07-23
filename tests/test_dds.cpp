// DDS writer: 128-byte header, correct dimensions/fourcc, exact payload append.
#include "microtest.h"
#include "core/DdsWriter.h"
#include <vector>
#include <cstdint>
#include <cstring>

using namespace nfsmw;

static uint32_t le32(const std::vector<uint8_t>& b, size_t o) {
    uint32_t v; std::memcpy(&v, b.data() + o, 4); return v;
}

TEST(dds_dxt3_header) {
    std::vector<uint8_t> px(16384, 0xAB);  // 128x128 DXT3
    auto d = BuildDDS(128, 128, DdsFormat::DXT3, px, 0);
    CHECK_EQ(d.size(), (size_t)(128 + 16384));
    CHECK(d[0] == 'D' && d[1] == 'D' && d[2] == 'S' && d[3] == ' ');
    CHECK_EQ(le32(d, 4), 124u);     // dwSize
    CHECK_EQ(le32(d, 12), 128u);    // height
    CHECK_EQ(le32(d, 16), 128u);    // width
    CHECK_EQ(le32(d, 76), 32u);     // pixelformat dwSize
    CHECK_EQ(le32(d, 80), 0x4u);    // DDPF_FOURCC
    CHECK(d[84] == 'D' && d[85] == 'X' && d[86] == 'T' && d[87] == '3');
    // payload follows the 128-byte header unchanged
    CHECK_EQ((int)d[128], 0xAB);
}

TEST(dds_dxt1_linearsize) {
    std::vector<uint8_t> px(8192, 0);      // 128x128 DXT1
    auto d = BuildDDS(128, 128, DdsFormat::DXT1, px, 0);
    CHECK_EQ(le32(d, 20), 8192u);          // dwPitchOrLinearSize = block bytes
    CHECK(d[84] == 'D' && d[85] == 'X' && d[86] == 'T' && d[87] == '1');
}

TEST(dds_argb_uncompressed) {
    std::vector<uint8_t> px(4 * 4 * 4, 0);
    auto d = BuildDDS(4, 4, DdsFormat::ARGB32, px, 0);
    CHECK_EQ(d.size(), (size_t)(128 + 64));
    CHECK_EQ(le32(d, 80), 0x41u);          // DDPF_RGB | DDPF_ALPHAPIXELS
    CHECK_EQ(le32(d, 88), 32u);            // RGB bit count
}
