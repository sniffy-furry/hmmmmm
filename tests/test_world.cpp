// WorldData structural parser: counts/offsets of sections, scenery, triggers,
// and presence of the preserve-raw subsystems (roadmap W1/W2/W3 read side).
#include "microtest.h"
#include "formats/WorldData.h"
#include <vector>
#include <cstdint>
#include <cstring>

using namespace nfsmw;

static void put(std::vector<uint8_t>& b, uint32_t id, const std::vector<uint8_t>& p) {
    for (int s = 0; s < 4; ++s) b.push_back((id >> (8 * s)) & 0xFF);
    uint32_t sz = (uint32_t)p.size();
    for (int s = 0; s < 4; ++s) b.push_back((sz >> (8 * s)) & 0xFF);
    b.insert(b.end(), p.begin(), p.end());
}

TEST(worlddata_enumerates_records) {
    std::vector<uint8_t> buf;
    put(buf, 0x00034110u, std::vector<uint8_t>(92 * 2, 0xAA));  // 2 sections
    put(buf, 0x00034102u, std::vector<uint8_t>(72 * 3, 0xBB));  // 3 scenery infos
    put(buf, 0x00034103u, std::vector<uint8_t>(64 * 4, 0xCC));  // 4 instances
    std::vector<uint8_t> inner;
    put(inner, 0x0003414Au, std::vector<uint8_t>(16 * 2, 0xDD));// trigger chunk
    put(buf, 0x80034147u, inner);                              // container -> recurse -> 2 triggers
    put(buf, 0x0003B800u, std::vector<uint8_t>(100, 0));       // road net
    put(buf, 0x80036000u, std::vector<uint8_t>(40, 0));        // lights

    auto r = WorldDataParser::Parse(buf);
    CHECK(bool(r));
    if (!r) return;
    auto& w = r.value;
    CHECK_EQ(w.sections.size(),         (size_t)2);
    CHECK_EQ(w.sceneryInfos.size(),     (size_t)3);
    CHECK_EQ(w.sceneryInstances.size(), (size_t)4);
    CHECK_EQ(w.triggers.size(),         (size_t)2);   // found by recursing the container
    CHECK_EQ(w.roadNetworkBytes,        100u);
    CHECK_EQ(w.lightBytes,              40u);
    CHECK_EQ(w.weatherBytes,            0u);
    CHECK_EQ(w.parseWarnings,           0);
    CHECK_EQ(w.sections[0].word0,       0xAAAAAAAAu);
    CHECK_EQ(w.sceneryInstances[3].index, 3u);
}

TEST(worlddata_ignores_unknown_and_pad) {
    std::vector<uint8_t> buf;
    // a null/pad chunk, an unknown leaf, then real scenery instances
    put(buf, 0x00000000u, {});
    put(buf, 0x12345678u, std::vector<uint8_t>(10, 0xEE));
    put(buf, 0x00034103u, std::vector<uint8_t>(64, 0x01));
    auto r = WorldDataParser::Parse(buf);
    CHECK(bool(r));
    if (r) {
        CHECK_EQ(r.value.sceneryInstances.size(), (size_t)1);
        CHECK_EQ(r.value.parseWarnings, 0);
    }
}

TEST(worlddata_rejects_tiny_buffer) {
    std::vector<uint8_t> tiny = { 1, 2, 3 };
    auto r = WorldDataParser::Parse(tiny);
    CHECK(!r);  // structurally impossible
}
