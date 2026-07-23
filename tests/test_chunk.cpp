// EAGL chunk-model invariants and BinaryReader bounds-safety.
#include "microtest.h"
#include "core/ChunkReader.h"
#include <vector>
#include <cstdint>

using namespace nfsmw;

TEST(container_bit_rule) {
    CHECK(ChunkReader::IsContainer(0x80134000u));   // GeometryContainer
    CHECK(ChunkReader::IsContainer(0xB3300000u));   // TPKContainer
    CHECK(!ChunkReader::IsContainer(0x00134011u));  // object header (leaf)
    CHECK(!ChunkReader::IsContainer(0x33310004u));  // TPK entries (leaf)
    CHECK(!ChunkReader::IsContainer(0x00000000u));  // null/pad (leaf)
}

TEST(binaryreader_clamps_on_overrun) {
    std::vector<uint8_t> data = { 0x01, 0x02, 0x03 };
    BinaryReader r(data);
    CHECK_EQ(r.ReadU8(), (uint8_t)0x01);
    (void)r.ReadU32();                 // reads past end -> clamped to 0
    CHECK(r.Pos() >= data.size());     // cursor clamped, no OOB
    CHECK_EQ(r.ReadU32(), (uint32_t)0);// further reads return zero-initialised
}

TEST(strip_align_pad_removes_0x11) {
    std::vector<uint8_t> data = { 0x11, 0x11, 0x11, 0x42, 0x11 };
    auto s = StripAlignPad(std::span<const uint8_t>(data));
    CHECK_EQ((int)s.size(), 2);        // leading 0x11s stripped; trailing kept
    CHECK_EQ((int)s[0], 0x42);
}

TEST(chunk_walk_tiles_buffer) {
    // Two leaf chunks back to back: [id=1,size=2,AA BB][id=2,size=1,CC]
    std::vector<uint8_t> buf = {
        0x01,0,0,0,  0x02,0,0,0,  0xAA,0xBB,
        0x02,0,0,0,  0x01,0,0,0,  0xCC
    };
    int seen = 0; size_t consumed = 0;
    ChunkReader rd;
    rd.SetUnknownHandler([&](uint32_t id, std::span<const uint8_t> p, size_t off){
        (void)id; (void)off; ++seen; consumed += 8 + p.size();
    });
    rd.Parse(buf);
    CHECK_EQ(seen, 2);
    CHECK_EQ(consumed, buf.size());    // walker tiles the whole buffer losslessly
}
