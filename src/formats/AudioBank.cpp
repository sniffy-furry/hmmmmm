// ─── formats/AudioBank.cpp ───────────────────────────────────────────────────
#include "formats/AudioBank.h"
#include "core/Logger.h"
#include <fstream>
#include <cstring>
#include <algorithm>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// SNR on-disk layout helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace {

constexpr uint32_t kSNRMagic   = 0x5246534E; // "NSFR" little-endian
constexpr uint32_t kSNRVersion = 1;
constexpr size_t   kSNREntrySize = 32;

#pragma pack(push, 1)
struct SNRFileHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t entryCount;
    uint32_t namePoolOffset;
    uint32_t namePoolSize;
};
struct SNREntryRaw {
    uint32_t id;
    uint32_t nameOffset;
    uint32_t sptOffset;
    uint32_t sptSize;
    uint32_t sampleRate;
    uint16_t channels;
    uint16_t bitsPerSample;
    uint32_t codecTag;
    uint32_t durationMs;
};
#pragma pack(pop)
static_assert(sizeof(SNRFileHeader) == 20, "SNRFileHeader size mismatch");
static_assert(sizeof(SNREntryRaw)   == kSNREntrySize, "SNREntryRaw size mismatch");

// ── Read whole file ──────────────────────────────────────────────────────────
Result<std::vector<uint8_t>> ReadFile(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) return Result<std::vector<uint8_t>>::Err("Cannot open: " + p.string());
    auto sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> buf(sz);
    if (!f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz)))
        return Result<std::vector<uint8_t>>::Err("Read failed: " + p.string());
    return Result<std::vector<uint8_t>>::Ok(std::move(buf));
}

// ── IMA-ADPCM decoder ────────────────────────────────────────────────────────
// Minimal mono/stereo IMA (DVI) ADPCM → s16 PCM. Handles the MS-ADPCM
// block-header variant that NFSMW sometimes uses.
static const int kStepTable[89] = {
    7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,
    60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,
    337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,
    1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,
    4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,
    15289,16818,18500,20350,22385,24623,27086,29794,32767
};
static const int kIndexTable[16] = {-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};

static int16_t IMAClamp(int v) {
    return static_cast<int16_t>(std::clamp(v, -32768, 32767));
}

static std::vector<int16_t> DecodeIMABlock(const uint8_t* src, size_t blockSize,
                                            int channels) {
    // Each IMA block begins with (channels × 4-byte header): predictor(s16) + index(u8) + pad(u8)
    std::vector<int16_t> out;
    if (blockSize < static_cast<size_t>(4 * channels)) return out;

    struct Chan { int pred; int idx; };
    std::vector<Chan> ch(static_cast<size_t>(channels));

    size_t pos = 0;
    for (int c = 0; c < channels; ++c) {
        int16_t pred; std::memcpy(&pred, src + pos, 2);
        ch[static_cast<size_t>(c)].pred = pred;
        ch[static_cast<size_t>(c)].idx  = std::clamp<int>(src[pos + 2], 0, 88);
        pos += 4;
        out.push_back(pred); // first sample from header
    }

    // Remaining bytes: 4-byte groups per channel (mono) or interleaved pairs
    while (pos + static_cast<size_t>(4 * channels) <= blockSize) {
        for (int c = 0; c < channels; ++c) {
            auto& state = ch[static_cast<size_t>(c)];
            // 8 nibbles in 4 bytes → 8 samples
            for (int b = 0; b < 4; ++b) {
                uint8_t byte = src[pos++];
                for (int n = 0; n < 2; ++n) {
                    int nibble = (n == 0) ? (byte & 0xF) : ((byte >> 4) & 0xF);
                    int step   = kStepTable[state.idx];
                    int delta  = step >> 3;
                    if (nibble & 1) delta += step >> 2;
                    if (nibble & 2) delta += step >> 1;
                    if (nibble & 4) delta += step;
                    if (nibble & 8) delta = -delta;
                    state.pred = std::clamp(state.pred + delta, -32768, 32767);
                    state.idx  = std::clamp(state.idx + kIndexTable[nibble & 7], 0, 88);
                    if (channels == 1) {
                        out.push_back(IMAClamp(state.pred));
                    } else {
                        out.push_back(IMAClamp(state.pred));
                    }
                }
            }
            (void)c;
        }
    }
    return out;
}

// ── IMA-ADPCM encoder ────────────────────────────────────────────────────────
// Encodes interleaved s16 PCM to MS-IMA ADPCM block format.
// Block size: 512 bytes for mono (505 samples), 1024 bytes for stereo (1010 samples).
// Each block starts with (channels × 4-byte header), followed by nibble data.
static std::vector<uint8_t> EncodeIMABlock(const int16_t* pcm, size_t numFrames,
                                            int channels) {
    const size_t blockSize = (channels == 2) ? 1024u : 512u;
    // Samples per block = 1 (header pred) + (blockSize/channels - 4) * 2 nibbles per byte
    const size_t samplesPerBlock = 1 + ((blockSize / static_cast<size_t>(channels) - 4) * 2);

    std::vector<uint8_t> out;
    out.reserve(((numFrames + samplesPerBlock - 1) / samplesPerBlock) * blockSize);

    struct ChanState { int pred = 0; int idx = 0; };
    std::vector<ChanState> state(static_cast<size_t>(channels));

    size_t frame = 0;
    while (frame < numFrames) {
        const size_t framesThisBlock = std::min(samplesPerBlock, numFrames - frame);
        std::vector<uint8_t> block(blockSize, 0);
        size_t bpos = 0;

        // Write per-channel headers using first frame's sample as predictor
        for (int c = 0; c < channels; ++c) {
            const int16_t pred = (framesThisBlock > 0)
                ? pcm[(frame * static_cast<size_t>(channels)) + static_cast<size_t>(c)]
                : 0;
            state[static_cast<size_t>(c)].pred = pred;
            state[static_cast<size_t>(c)].idx  = 0;
            block[bpos + 0] = static_cast<uint8_t>(pred & 0xFF);
            block[bpos + 1] = static_cast<uint8_t>((pred >> 8) & 0xFF);
            block[bpos + 2] = 0; // step index
            block[bpos + 3] = 0; // pad
            bpos += 4;
        }

        // Encode remaining frames (skip first frame — already in header)
        size_t f = frame + 1;
        const size_t frameEnd = frame + framesThisBlock;

        while (bpos < blockSize && f < frameEnd) {
            for (int c = 0; c < channels && bpos < blockSize; ++c) {
                auto& st = state[static_cast<size_t>(c)];
                // Pack 8 nibbles into 4 bytes
                for (int b = 0; b < 4 && bpos < blockSize; ++b) {
                    uint8_t packed = 0;
                    for (int n = 0; n < 2; ++n) {
                        int nib = 0;
                        if (f < frameEnd) {
                            const int16_t sample = pcm[f * static_cast<size_t>(channels) +
                                                       static_cast<size_t>(c)];
                            const int diff = static_cast<int>(sample) - st.pred;
                            const int step = kStepTable[st.idx];
                            nib = (std::abs(diff) * 4) / step;
                            if (nib > 7) nib = 7;
                            if (diff < 0) nib |= 8;

                            // Reconstruct to keep encoder/decoder in sync
                            int delta = step >> 3;
                            if (nib & 1) delta += step >> 2;
                            if (nib & 2) delta += step >> 1;
                            if (nib & 4) delta += step;
                            if (nib & 8) delta = -delta;
                            st.pred = std::clamp(st.pred + delta, -32768, 32767);
                            st.idx  = std::clamp(st.idx + kIndexTable[nib & 7], 0, 88);
                            ++f;
                        }
                        if (n == 0) packed |= static_cast<uint8_t>(nib & 0xF);
                        else        packed |= static_cast<uint8_t>((nib & 0xF) << 4);
                    }
                    block[bpos++] = packed;
                }
            }
        }

        out.insert(out.end(), block.begin(), block.end());
        frame = frameEnd;
    }
    return out;
}

// ── WAV reader — interleaved stereo or mono, 16-bit PCM ─────────────────────
// Returns interleaved s16 samples preserving the original channel count.
// outChannels is set to the WAV's channel count.
Result<std::vector<int16_t>> ReadWAVInterleaved(const std::filesystem::path& path,
                                                  uint32_t& outRate,
                                                  uint32_t& outChannels) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return Result<std::vector<int16_t>>::Err("Cannot open WAV: " + path.string());
    const size_t sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> d(sz);
    f.read(reinterpret_cast<char*>(d.data()), static_cast<std::streamsize>(sz));

    if (sz < 44 || std::memcmp(d.data(), "RIFF", 4) != 0 ||
        std::memcmp(d.data() + 8, "WAVE", 4) != 0)
        return Result<std::vector<int16_t>>::Err("Not a valid WAV file");

    auto u16 = [&](size_t o) -> uint16_t {
        return static_cast<uint16_t>(d[o] | (d[o + 1] << 8));
    };
    auto u32 = [&](size_t o) -> uint32_t {
        return static_cast<uint32_t>(d[o]) | (static_cast<uint32_t>(d[o+1]) << 8) |
               (static_cast<uint32_t>(d[o+2]) << 16) | (static_cast<uint32_t>(d[o+3]) << 24);
    };

    uint16_t fmt = 0, channels = 0, bits = 0;
    uint32_t rate = 0;
    size_t dataOff = 0, dataLen = 0;
    size_t p = 12;
    while (p + 8 <= sz) {
        const uint32_t id   = u32(p);
        const uint32_t clen = u32(p + 4);
        const size_t   body = p + 8;
        if (id == 0x20746D66u /* "fmt " */ && body + 16 <= sz) {
            fmt      = u16(body);
            channels = u16(body + 2);
            rate     = u32(body + 4);
            bits     = u16(body + 14);
        } else if (id == 0x61746164u /* "data" */) {
            dataOff = body;
            dataLen = std::min<size_t>(clen, sz - body);
        }
        p = body + clen + (clen & 1); // chunks are word-aligned
    }

    if (channels == 0 || rate == 0 || dataOff == 0)
        return Result<std::vector<int16_t>>::Err("WAV missing fmt/data chunk");
    if (fmt != 1 || bits != 16)
        return Result<std::vector<int16_t>>::Err(
            "Only 16-bit PCM WAV is supported (got fmt=" + std::to_string(fmt) +
            " bits=" + std::to_string(bits) + ")");

    outRate     = rate;
    outChannels = channels;

    const size_t totalSamples = dataLen / 2; // each sample is 2 bytes
    std::vector<int16_t> pcm(totalSamples);
    std::memcpy(pcm.data(), d.data() + dataOff, totalSamples * 2);
    return Result<std::vector<int16_t>>::Ok(std::move(pcm));
}

// ── WAV writer ───────────────────────────────────────────────────────────────
// Writes a standard RIFF WAV (PCM, 16-bit) from interleaved s16 samples.
Result<void> WriteWAV(const std::filesystem::path& path,
                      const std::vector<int16_t>& pcm,
                      uint32_t sampleRate,
                      uint16_t channels) {
    if (channels == 0 || sampleRate == 0)
        return Result<void>::Err("Invalid WAV parameters");

    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return Result<void>::Err("Cannot open for writing: " + path.string());

    const uint32_t dataBytes   = static_cast<uint32_t>(pcm.size() * 2);
    const uint32_t byteRate    = sampleRate * channels * 2;
    const uint16_t blockAlign  = static_cast<uint16_t>(channels * 2);
    const uint32_t riffSize    = 36 + dataBytes;

    auto w16 = [&](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };
    auto w32 = [&](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
    auto wcc = [&](const char* s) { f.write(s, 4); };

    wcc("RIFF");  w32(riffSize);
    wcc("WAVE");
    wcc("fmt ");  w32(16);          // chunk size
    w16(1);                         // PCM
    w16(channels);
    w32(sampleRate);
    w32(byteRate);
    w16(blockAlign);
    w16(16);                        // bits per sample
    wcc("data");  w32(dataBytes);
    f.write(reinterpret_cast<const char*>(pcm.data()), static_cast<std::streamsize>(dataBytes));

    if (!f) return Result<void>::Err("WAV write failed: " + path.string());
    return Result<void>::Ok();
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// AudioBankParser::LoadSNR
// ─────────────────────────────────────────────────────────────────────────────
Result<AudioBank> AudioBankParser::LoadSNR(const std::filesystem::path& snrPath) {
    auto rawRes = ReadFile(snrPath);
    if (!rawRes) return Result<AudioBank>::Err(rawRes.error);
    const auto& raw = rawRes.value;

    if (raw.size() < sizeof(SNRFileHeader))
        return Result<AudioBank>::Err("File too small for SNR header");

    SNRFileHeader hdr;
    std::memcpy(&hdr, raw.data(), sizeof(hdr));

    if (hdr.magic != kSNRMagic)
        return Result<AudioBank>::Err("Not an NFSMW SNR file (bad magic)");
    if (hdr.version != kSNRVersion)
        return Result<AudioBank>::Err("Unsupported SNR version: " + std::to_string(hdr.version));

    const size_t entryBase = sizeof(SNRFileHeader);
    const size_t entryEnd  = entryBase + hdr.entryCount * kSNREntrySize;
    if (raw.size() < entryEnd)
        return Result<AudioBank>::Err("SNR truncated in entry table");
    if (raw.size() < hdr.namePoolOffset + hdr.namePoolSize)
        return Result<AudioBank>::Err("SNR truncated in name pool");

    AudioBank bank;
    bank.name    = snrPath.stem().string();
    bank.snrPath = snrPath;
    bank.sptPath = snrPath.parent_path() / (snrPath.stem().string() + ".SPT");
    if (!std::filesystem::exists(bank.sptPath)) {
        // Try lowercase
        bank.sptPath = snrPath.parent_path() / (snrPath.stem().string() + ".spt");
    }

    for (uint32_t i = 0; i < hdr.entryCount; ++i) {
        SNREntryRaw e;
        std::memcpy(&e, raw.data() + entryBase + i * kSNREntrySize, kSNREntrySize);

        AudioEntry entry;
        entry.id           = e.id;
        entry.sptOffset    = e.sptOffset;
        entry.sptSize      = e.sptSize;
        entry.sampleRate   = e.sampleRate;
        entry.channels     = e.channels;
        entry.bitsPerSample= e.bitsPerSample;
        entry.codecTag     = e.codecTag;
        entry.durationMs   = e.durationMs;

        // Name from pool
        const size_t nameAbs = hdr.namePoolOffset + e.nameOffset;
        if (nameAbs < raw.size()) {
            entry.name = reinterpret_cast<const char*>(raw.data() + nameAbs);
        } else {
            entry.name = "sound_" + std::to_string(e.id);
        }

        bank.entries.push_back(std::move(entry));
    }

    LOG_INFO("AudioBank: loaded {} entries from {}", bank.entries.size(), snrPath.filename().string());
    return Result<AudioBank>::Ok(std::move(bank));
}

// ─────────────────────────────────────────────────────────────────────────────
// AudioBankParser::ReadPayload
// ─────────────────────────────────────────────────────────────────────────────
Result<std::vector<uint8_t>> AudioBankParser::ReadPayload(const AudioBank& bank,
                                                           const AudioEntry& entry) {
    if (bank.sptPath.empty() || !std::filesystem::exists(bank.sptPath))
        return Result<std::vector<uint8_t>>::Err("SPT file not found: " + bank.sptPath.string());
    if (entry.sptSize == 0)
        return Result<std::vector<uint8_t>>::Err("Entry has zero payload size");

    std::ifstream f(bank.sptPath, std::ios::binary);
    if (!f) return Result<std::vector<uint8_t>>::Err("Cannot open SPT: " + bank.sptPath.string());

    f.seekg(static_cast<std::streamoff>(entry.sptOffset));
    if (!f) return Result<std::vector<uint8_t>>::Err("SPT seek failed");

    std::vector<uint8_t> buf(entry.sptSize);
    if (!f.read(reinterpret_cast<char*>(buf.data()), entry.sptSize))
        return Result<std::vector<uint8_t>>::Err("SPT read failed");

    return Result<std::vector<uint8_t>>::Ok(std::move(buf));
}

// ─────────────────────────────────────────────────────────────────────────────
// AudioBankParser::DecodePCM
// ─────────────────────────────────────────────────────────────────────────────
Result<std::vector<int16_t>> AudioBankParser::DecodePCM(const std::vector<uint8_t>& raw,
                                                          const AudioEntry& entry,
                                                          uint32_t& outRate,
                                                          uint32_t& outChannels) {
    outRate     = entry.sampleRate > 0 ? entry.sampleRate : 22050;
    outChannels = entry.channels   > 0 ? entry.channels   : 1;

    switch (entry.codecTag) {
        // ── Raw signed-16 PCM ─────────────────────────────────────────────────
        case AudioCodec::PCM: {
            if (raw.size() % 2 != 0)
                return Result<std::vector<int16_t>>::Err("PCM payload not 2-byte aligned");
            std::vector<int16_t> out(raw.size() / 2);
            std::memcpy(out.data(), raw.data(), raw.size());
            return Result<std::vector<int16_t>>::Ok(std::move(out));
        }

        // ── IMA-ADPCM ─────────────────────────────────────────────────────────
        case AudioCodec::IMA_ADPCM: {
            // MS-ADPCM block size heuristic: 512 bytes for mono, 1024 for stereo.
            const size_t blockSize = (outChannels == 2) ? 1024u : 512u;
            std::vector<int16_t> out;
            size_t pos = 0;
            while (pos + 4 <= raw.size()) {
                size_t chunk = std::min(blockSize, raw.size() - pos);
                auto block = DecodeIMABlock(raw.data() + pos, chunk,
                                            static_cast<int>(outChannels));
                out.insert(out.end(), block.begin(), block.end());
                pos += blockSize;
            }
            return Result<std::vector<int16_t>>::Ok(std::move(out));
        }

        // ── EA-wrapped MP3 / bare MP3 ──────────────────────────────────────────
        // We cannot link a full MP3 decoder without adding minimp3 or dr_mp3 as a
        // vendored header.  Return the raw bytes and let AudioPanel pass them to
        // miniaudio's built-in MP3 decoder (which uses dr_mp3 internally).
        // Signal this by returning an EMPTY vector — AudioPanel detects this and
        // falls back to the raw byte path.
        case AudioCodec::EA_MP3:
        case AudioCodec::MP3:
            // Intentional: caller must use raw bytes with miniaudio.
            return Result<std::vector<int16_t>>::Ok({});

        // ── EA proprietary ADPCM ───────────────────────────────────────────────
        // Same situation — return empty; AudioPanel feeds raw to miniaudio which
        // will attempt direct decode.
        case AudioCodec::EA_EAADPCM:
        default:
            return Result<std::vector<int16_t>>::Ok({});
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AudioBankParser::ExportWAV
//
// Decodes the SPT payload for one entry and writes a standard 16-bit PCM WAV.
// For codecs the in-process decoder handles (PCM, IMA-ADPCM) this is lossless.
// For EA-MP3 / EA-ADPCM entries the raw payload is returned to the caller for
// miniaudio-side decode; if DecodePCM returns an empty vector we cannot
// produce a WAV and return an error explaining why.
// ─────────────────────────────────────────────────────────────────────────────
Result<void> AudioBankParser::ExportWAV(const AudioBank& bank,
                                         const AudioEntry& entry,
                                         const std::filesystem::path& dstPath) {
    auto rawRes = ReadPayload(bank, entry);
    if (!rawRes) return Result<void>::Err(rawRes.error);

    uint32_t rate = 0, ch = 0;
    auto pcmRes = DecodePCM(rawRes.value, entry, rate, ch);
    if (!pcmRes) return Result<void>::Err(pcmRes.error);

    if (pcmRes.value.empty()) {
        // DecodePCM returns empty for EA-MP3 / EA-ADPCM — we have no in-process
        // decoder for these codecs.  Export the raw payload as a .bin instead so
        // the user at least gets something; AudioPanel shows a more specific toast.
        return Result<void>::Err(
            "In-process decoder unavailable for codec " +
            std::string(entry.CodecName()) +
            ". Use 'Export raw…' to save the original payload.");
    }

    return WriteWAV(dstPath, pcmRes.value, rate, static_cast<uint16_t>(ch));
}

// ─────────────────────────────────────────────────────────────────────────────
// AudioBankParser::ExportRaw
//
// Writes the raw SPT payload bytes to dstPath with no decode step.
// Useful for EA-MP3 / EA-ADPCM entries that ExportWAV cannot handle, or for
// advanced users who want the original codec data.
// ─────────────────────────────────────────────────────────────────────────────
Result<void> AudioBankParser::ExportRaw(const AudioBank& bank,
                                         const AudioEntry& entry,
                                         const std::filesystem::path& dstPath) {
    auto rawRes = ReadPayload(bank, entry);
    if (!rawRes) return Result<void>::Err(rawRes.error);

    std::ofstream f(dstPath, std::ios::binary | std::ios::trunc);
    if (!f) return Result<void>::Err("Cannot write: " + dstPath.string());
    f.write(reinterpret_cast<const char*>(rawRes.value.data()),
            static_cast<std::streamsize>(rawRes.value.size()));
    if (!f) return Result<void>::Err("Write failed: " + dstPath.string());
    return Result<void>::Ok();
}

// ─────────────────────────────────────────────────────────────────────────────
// AudioBankParser::ReplaceFromWAV
//
// Accepts a standard 16-bit PCM WAV, re-encodes it to the entry's original
// codec, and patches the SPT + SNR files in-place (with backup).
//
// Codec support:
//   PCM        — raw s16 bytes written directly (no re-encode).
//   IMA_ADPCM  — encoded with the MS-IMA ADPCM block encoder above.
//   EA_EAADPCM — not yet supported; returns an error directing the user to
//                ReplacePayload with a pre-encoded blob.
//   EA_MP3/MP3 — not yet supported; same fallback.
//
// Channel / sample-rate mismatch: the WAV's values are accepted as-is and
// used to update the SNR entry metadata so the game/tool decode correctly.
// ─────────────────────────────────────────────────────────────────────────────
Result<void> AudioBankParser::ReplaceFromWAV(AudioBank& bank,
                                              AudioEntry& entry,
                                              const std::filesystem::path& srcWav,
                                              const std::filesystem::path& backupDir) {
    // ── Read and validate WAV ─────────────────────────────────────────────────
    uint32_t wavRate = 0, wavChannels = 0;
    auto pcmRes = ReadWAVInterleaved(srcWav, wavRate, wavChannels);
    if (!pcmRes) return Result<void>::Err(pcmRes.error);
    const std::vector<int16_t>& pcm = pcmRes.value;

    if (pcm.empty())
        return Result<void>::Err("WAV contains no audio data");

    // ── Re-encode to the entry's original codec ───────────────────────────────
    std::vector<uint8_t> newPayload;

    switch (entry.codecTag) {
        case AudioCodec::PCM: {
            // Raw s16 — copy bytes directly.
            newPayload.resize(pcm.size() * 2);
            std::memcpy(newPayload.data(), pcm.data(), newPayload.size());
            break;
        }

        case AudioCodec::IMA_ADPCM: {
            // MS-IMA ADPCM block encoding.
            // Frames = total samples / channels (interleaved layout).
            const size_t numFrames = pcm.size() / std::max<uint32_t>(1, wavChannels);
            newPayload = EncodeIMABlock(pcm.data(), numFrames,
                                        static_cast<int>(wavChannels));
            if (newPayload.empty())
                return Result<void>::Err("IMA-ADPCM encode produced no output");
            break;
        }

        case AudioCodec::EA_EAADPCM:
            return Result<void>::Err(
                "EA-ADPCM re-encode not yet implemented. "
                "Use 'Replace raw…' with a pre-encoded payload.");

        case AudioCodec::EA_MP3:
        case AudioCodec::MP3:
            return Result<void>::Err(
                "MP3 re-encode not yet implemented. "
                "Use 'Replace raw…' with a pre-encoded payload.");

        default:
            return Result<void>::Err(
                "Unknown codec 0x" + [&]{
                    char buf[16]; snprintf(buf, sizeof(buf), "%04X", entry.codecTag);
                    return std::string(buf);
                }() + " — use 'Replace raw…' with a pre-encoded payload.");
    }

    // ── Patch SNR metadata (rate / channels / duration) ───────────────────────
    // Update the in-memory entry so ReplacePayload writes the correct values.
    const uint32_t numFrames   = static_cast<uint32_t>(pcm.size() /
                                  std::max<uint32_t>(1, wavChannels));
    entry.sampleRate   = wavRate;
    entry.channels     = static_cast<uint16_t>(wavChannels);
    entry.bitsPerSample= 16;
    entry.durationMs   = (wavRate > 0)
        ? static_cast<uint32_t>((static_cast<uint64_t>(numFrames) * 1000u) / wavRate)
        : 0;

    // ── Patch SNR entry metadata on disk before writing payload ───────────────
    // We need to update rate/channels/duration in the SNR buffer as well.
    // ReplacePayload rewrites the SNR, so we patch entry fields first and let
    // it handle the serialisation.
    auto snrRes = ReadFile(bank.snrPath);
    if (!snrRes) return Result<void>::Err(snrRes.error);
    std::vector<uint8_t>& snrBuf = snrRes.value;

    // Find entry index
    int entryIdx = -1;
    for (int i = 0; i < static_cast<int>(bank.entries.size()); ++i) {
        if (bank.entries[static_cast<size_t>(i)].id == entry.id) { entryIdx = i; break; }
    }
    if (entryIdx < 0) return Result<void>::Err("Entry not found in bank");

    const size_t eOff = sizeof(SNRFileHeader) + static_cast<size_t>(entryIdx) * kSNREntrySize;
    if (eOff + kSNREntrySize > snrBuf.size())
        return Result<void>::Err("SNR buffer too small for entry patch");

    // Patch rate, channels, bits, duration in the SNR buffer.
    std::memcpy(snrBuf.data() + eOff + offsetof(SNREntryRaw, sampleRate),   &entry.sampleRate,    4);
    std::memcpy(snrBuf.data() + eOff + offsetof(SNREntryRaw, channels),     &entry.channels,      2);
    std::memcpy(snrBuf.data() + eOff + offsetof(SNREntryRaw, bitsPerSample),&entry.bitsPerSample, 2);
    std::memcpy(snrBuf.data() + eOff + offsetof(SNREntryRaw, durationMs),   &entry.durationMs,    4);

    // Write the patched SNR now; ReplacePayload will rewrite it again with the
    // new sptOffset/sptSize — but we need the metadata written first in case
    // ReplacePayload reads it back. Simpler: just let ReplacePayload do it all.
    // Instead of writing here, stash the patched metadata in the in-memory bank
    // entry (already done above) and let ReplacePayload handle SNR serialisation.
    bank.entries[static_cast<size_t>(entryIdx)].sampleRate    = entry.sampleRate;
    bank.entries[static_cast<size_t>(entryIdx)].channels      = entry.channels;
    bank.entries[static_cast<size_t>(entryIdx)].bitsPerSample = entry.bitsPerSample;
    bank.entries[static_cast<size_t>(entryIdx)].durationMs    = entry.durationMs;

    // ── Write payload (also handles backup, SNR sptOffset/sptSize patch, SPT rewrite)
    return ReplacePayload(bank, entry, newPayload, backupDir);
}

// ─────────────────────────────────────────────────────────────────────────────
// AudioBankParser::ReplacePayload
//
// Rebuilds the SPT file by splicing in the new payload at the chosen entry's
// slot.  If the new payload fits within the original slot the surrounding data
// is kept verbatim; if it is larger, the entry is appended at the end and the
// freed slot is zero-padded (simple approach that avoids complex offset fixup
// across all entries while keeping the file consistent).
// SNR entry sptOffset / sptSize are updated and the SNR is rewritten.
// ─────────────────────────────────────────────────────────────────────────────
Result<void> AudioBankParser::ReplacePayload(AudioBank& bank,
                                              AudioEntry& entry,
                                              const std::vector<uint8_t>& newPayload,
                                              const std::filesystem::path& backupDir) {
    // ── Read current SNR ─────────────────────────────────────────────────────
    auto snrRes = ReadFile(bank.snrPath);
    if (!snrRes) return Result<void>::Err(snrRes.error);
    std::vector<uint8_t> snrBuf = std::move(snrRes.value);

    // ── Read current SPT ─────────────────────────────────────────────────────
    auto sptRes = ReadFile(bank.sptPath);
    if (!sptRes) return Result<void>::Err(sptRes.error);
    std::vector<uint8_t> sptBuf = std::move(sptRes.value);

    // ── Backup originals ─────────────────────────────────────────────────────
    if (!backupDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(backupDir, ec);
        auto bakSNR = backupDir / (bank.snrPath.filename().string() + ".bak");
        auto bakSPT = backupDir / (bank.sptPath.filename().string() + ".bak");
        if (!std::filesystem::exists(bakSNR))
            std::filesystem::copy_file(bank.snrPath, bakSNR, ec);
        if (!std::filesystem::exists(bakSPT))
            std::filesystem::copy_file(bank.sptPath, bakSPT, ec);
    }

    // ── Find entry index in bank (by id) ─────────────────────────────────────
    int entryIdx = -1;
    for (int i = 0; i < static_cast<int>(bank.entries.size()); ++i) {
        if (bank.entries[static_cast<size_t>(i)].id == entry.id) { entryIdx = i; break; }
    }
    if (entryIdx < 0) return Result<void>::Err("Entry not found in bank");

    // ── Decide strategy ───────────────────────────────────────────────────────
    uint32_t newOffset;
    if (newPayload.size() <= entry.sptSize) {
        // In-place: overwrite, zero-pad remainder
        newOffset = entry.sptOffset;
        if (entry.sptOffset + entry.sptSize > sptBuf.size())
            return Result<void>::Err("Entry SPT offset/size out of range");
        std::memcpy(sptBuf.data() + entry.sptOffset, newPayload.data(), newPayload.size());
        if (newPayload.size() < entry.sptSize)
            std::memset(sptBuf.data() + entry.sptOffset + newPayload.size(), 0,
                        entry.sptSize - newPayload.size());
    } else {
        // Append: zero-pad old slot, append new bytes at end
        if (entry.sptOffset + entry.sptSize <= sptBuf.size())
            std::memset(sptBuf.data() + entry.sptOffset, 0, entry.sptSize);
        // Align to 16 bytes
        while (sptBuf.size() % 16 != 0) sptBuf.push_back(0);
        newOffset = static_cast<uint32_t>(sptBuf.size());
        sptBuf.insert(sptBuf.end(), newPayload.begin(), newPayload.end());
    }

    // ── Patch SNR entry in buffer ─────────────────────────────────────────────
    const size_t entryBase = sizeof(SNRFileHeader);
    const size_t eOff = entryBase + static_cast<size_t>(entryIdx) * kSNREntrySize;
    if (eOff + kSNREntrySize > snrBuf.size())
        return Result<void>::Err("SNR buffer too small for entry patch");

    uint32_t newSize = static_cast<uint32_t>(newPayload.size());
    std::memcpy(snrBuf.data() + eOff + offsetof(SNREntryRaw, sptOffset),    &newOffset,                  4);
    std::memcpy(snrBuf.data() + eOff + offsetof(SNREntryRaw, sptSize),      &newSize,                    4);
    // Also write any metadata fields updated by ReplaceFromWAV (no-op if called directly)
    std::memcpy(snrBuf.data() + eOff + offsetof(SNREntryRaw, sampleRate),   &entry.sampleRate,           4);
    std::memcpy(snrBuf.data() + eOff + offsetof(SNREntryRaw, channels),     &entry.channels,             2);
    std::memcpy(snrBuf.data() + eOff + offsetof(SNREntryRaw, bitsPerSample),&entry.bitsPerSample,        2);
    std::memcpy(snrBuf.data() + eOff + offsetof(SNREntryRaw, durationMs),   &entry.durationMs,           4);

    // ── Write back ────────────────────────────────────────────────────────────
    {
        std::ofstream fsnr(bank.snrPath, std::ios::binary | std::ios::trunc);
        if (!fsnr) return Result<void>::Err("Cannot write SNR: " + bank.snrPath.string());
        fsnr.write(reinterpret_cast<const char*>(snrBuf.data()),
                   static_cast<std::streamsize>(snrBuf.size()));
    }
    {
        std::ofstream fspt(bank.sptPath, std::ios::binary | std::ios::trunc);
        if (!fspt) return Result<void>::Err("Cannot write SPT: " + bank.sptPath.string());
        fspt.write(reinterpret_cast<const char*>(sptBuf.data()),
                   static_cast<std::streamsize>(sptBuf.size()));
    }

    // ── Update in-memory entry ────────────────────────────────────────────────
    entry.sptOffset = newOffset;
    entry.sptSize   = newSize;
    bank.entries[static_cast<size_t>(entryIdx)].sptOffset = newOffset;
    bank.entries[static_cast<size_t>(entryIdx)].sptSize   = newSize;

    LOG_INFO("AudioBank: replaced entry '{}' ({} bytes)", entry.name, newSize);
    return Result<void>::Ok();
}

} // namespace nfsmw