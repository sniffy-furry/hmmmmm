// JDLZ round-trip regression: Decompress(Compress(x)) == x, and the header is
// well-formed. This is the safety net for the new compressor (roadmap C1).
#include "microtest.h"
#include "core/LZCDecompressor.h"
#include <vector>
#include <cstdint>
#include <string>

using namespace nfsmw;

static std::vector<uint8_t> make_buf() {
    // A buffer with runs (exercises near refs), repeats at varied distances
    // (exercises far refs), and incompressible noise (exercises literals).
    std::vector<uint8_t> b;
    for (int i = 0; i < 300; ++i) b.push_back(0xAB);                 // run -> near ref, dist 1
    const char* s = "MINI_MAP_CHOP0 ARC_BRICK01_NORMAL EngineRacer ";
    for (int rep = 0; rep < 40; ++rep)
        for (const char* p = s; *p; ++p) b.push_back((uint8_t)*p);   // repeated phrase -> far refs
    uint32_t x = 0x12345678u;
    for (int i = 0; i < 500; ++i) { x = x * 1103515245u + 12345u; b.push_back((uint8_t)(x >> 16)); } // noise
    return b;
}

TEST(jdlz_roundtrip_synthetic) {
    auto src = make_buf();
    auto c = LZCDecompressor::Compress(src);
    CHECK(bool(c));
    if (!c) return;
    // header sanity
    CHECK(c.value.size() >= 16);
    CHECK(c.value[0] == 'J' && c.value[1] == 'D' && c.value[2] == 'L' && c.value[3] == 'Z');
    CHECK(LZCDecompressor::IsCompressed(c.value));
    // decompressed-size field matches the source length
    uint32_t declared = c.value[8] | (c.value[9] << 8) | (c.value[10] << 16) | (c.value[11] << 24);
    CHECK_EQ(declared, (uint32_t)src.size());
    // round-trip
    auto back = LZCDecompressor::Decompress(c.value);
    CHECK(bool(back));
    if (back) CHECK(back.value == src);
}

TEST(jdlz_roundtrip_runs_only) {
    std::vector<uint8_t> src(4096, 0x00);   // pure run; long near-ref lengths
    auto c = LZCDecompressor::Compress(src);
    CHECK(bool(c));
    if (!c) return;
    auto back = LZCDecompressor::Decompress(c.value);
    CHECK(back && back.value == src);
    CHECK(c.value.size() < src.size());     // a run must compress
}

TEST(jdlz_roundtrip_small) {
    std::vector<uint8_t> src = { 1, 2, 3 };
    auto c = LZCDecompressor::Compress(src);
    CHECK(bool(c));
    if (c) { auto back = LZCDecompressor::Decompress(c.value); CHECK(back && back.value == src); }
}
