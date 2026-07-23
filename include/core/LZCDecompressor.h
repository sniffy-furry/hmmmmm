#pragma once
#include "Common.h"
#include <vector>
#include <span>

namespace nfsmw {

/// EA Black Box JDLZ decompressor.
///
/// NFSMW (PC) compresses some files (e.g. GLOBAL/GlobalB.lzc) with JDLZ,
/// a byte-oriented LZ77 variant with two interleaved flag streams.
///
/// Verified against retail data: GLOBAL/GlobalB.lzc (1,520,744 bytes)
/// decompresses to exactly 2,803,648 bytes of valid chunk data.
///
/// Header layout (16 bytes):
///   [0]  magic 'JDLZ'
///   [4]  u8 version (0x02)
///   [5]  u8 header size (0x10)
///   [6]  u16 reserved
///   [8]  u32 decompressed size
///   [12] u32 compressed size (== file size)
class LZCDecompressor {
public:
    /// Check whether the given buffer starts with a known compression magic.
    static bool IsCompressed(std::span<const uint8_t> data);

    /// Decompress a JDLZ buffer (including its 16-byte header).
    static Result<std::vector<uint8_t>> Decompress(std::span<const uint8_t> data);

    /// Compress a raw buffer into a JDLZ stream (16-byte header + LZ77 body)
    /// that the game's loader — and Decompress() above — accept.
    ///
    /// A real back-reference encoder (greedy hash-chain matcher) mirroring the
    /// exact near/far window the decoder reads:
    ///   near (dist 1..16,   len 3..4098): runs / short-range repeats
    ///   far  (dist 17..2064, len 3..34) : general matches
    /// Distances beyond 2064 fall back to literals. The two interleaved flag
    /// streams (flags1 = literal/reference, flags2 = near/far) are emitted in
    /// the decoder's exact refill order, so output round-trips byte-exact.
    ///
    /// Verified against retail GLOBAL/GlobalB.lzc (2,803,648 bytes) and
    /// InGameB.lzc: Decompress(Compress(x)) == x at ~0.54 ratio (within a
    /// fraction of a percent of the original compressor). Use it to write
    /// edited compressed TPK/minimap payloads or to re-pack rebuilt files
    /// rather than leaving them uncompressed.
    static Result<std::vector<uint8_t>> Compress(std::span<const uint8_t> data);

private:
    static Result<std::vector<uint8_t>> DecompressJDLZ(std::span<const uint8_t> data);
    static std::vector<uint8_t>         CompressJDLZ(std::span<const uint8_t> data);
};

} // namespace nfsmw
