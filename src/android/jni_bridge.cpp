// ─── src/android/jni_bridge.cpp ────────────────────────────────────────────
//
// Lean JNI bridge around nfsmw_core for a plain Kotlin Activity UI.
//
// This is the "take just the C++ engine, put a different UI on it" path:
// no VanGUI, no GLFW/FFmpeg, no NativeActivity/native_app_glue, no C++23.
// It links only against nfsmw_core (glm/spdlog/nlohmann-json), exactly like
// the existing `mwcli` desktop tool - same engine, Kotlin UI instead of a
// terminal.
//
// Exposed to Kotlin as com.teamvanilla.mwtools.NativeBridge:
//   external fun identify(path: String): String
//   external fun info(path: String): String
//
// Both take an on-disk file path (get one via Android's file picker /
// Storage Access Framework on the Kotlin side, then copy/resolve it to a
// real path or read bytes into a temp file) and return a human-readable
// report, mirroring `mwcli identify` / `mwcli info`.

#include <jni.h>

#include "core/BINFile.h"
#include "core/ChunkReader.h"
#include "core/LZCDecompressor.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace nfsmw;

namespace {

std::string JStringToStd(JNIEnv* env, jstring s) {
    if (!s) return {};
    const char* chars = env->GetStringUTFChars(s, nullptr);
    std::string out(chars ? chars : "");
    if (chars) env->ReleaseStringUTFChars(s, chars);
    return out;
}

jstring StdToJString(JNIEnv* env, const std::string& s) {
    return env->NewStringUTF(s.c_str());
}

bool LoadRaw(const fs::path& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    out.resize(sz);
    return static_cast<bool>(f.read(reinterpret_cast<char*>(out.data()),
                                     static_cast<std::streamsize>(sz)));
}

const char* ChunkName(uint32_t id) {
    switch (id) {
        case 0xB3300000u: return "TPKContainer";
        case 0x80134000u: return "GeometryContainer";
        default:          return "(chunk)";
    }
}

// Mirrors mwcli's CmdIdentify, writing to a stream instead of stdout.
void Identify(const fs::path& path, std::ostringstream& out) {
    std::vector<uint8_t> b;
    if (!LoadRaw(path, b)) {
        out << "error: cannot read " << path.string() << "\n";
        return;
    }
    auto m4 = [&](const char* s) { return b.size() >= 4 && std::memcmp(b.data(), s, 4) == 0; };

    if (m4("JDLZ")) { out << "JDLZ-compressed stream (decompress, then re-identify)\n"; return; }
    if (m4("RAWW")) { out << "RAWW stored (uncompressed body after 16-byte header)\n"; return; }
    if (m4("VPAK")) { out << "VPAK attribute vault\n"; return; }
    if (m4("DDS ")) { out << "DDS texture\n"; return; }

    if (b.size() >= 8) {
        uint32_t id; std::memcpy(&id, b.data(), 4);
        if ((id & 0x80000000u) != 0) {
            out << "EAGL chunk tree (first id 0x" << std::hex << id << std::dec
                << " is a container)\n";
            return;
        }
    }
    out << "unrecognised (" << b.size() << " bytes)\n";
}

// Mirrors mwcli's CmdInfo/PrintTree, writing to a stream instead of stdout.
void PrintTree(std::span<const uint8_t> data, int depth, size_t baseOff,
               size_t& chunkCount, std::ostringstream& out) {
    size_t pos = 0;
    while (pos + 8 <= data.size()) {
        ChunkHeader h{};
        std::memcpy(&h, data.data() + pos, 8);
        if (h.id == 0 && h.size == 0) break;
        size_t payloadOff = pos + 8;
        size_t payloadEnd = payloadOff + h.size;
        if (payloadEnd > data.size()) {
            out << std::string(depth * 2, ' ') << "! truncated chunk 0x"
                << std::hex << h.id << std::dec << " at +0x" << std::hex
                << (baseOff + pos) << std::dec << "\n";
            break;
        }
        ++chunkCount;
        out << std::string(depth * 2, ' ') << "0x" << std::hex << h.id << std::dec
            << "  " << ChunkName(h.id) << "  size=" << h.size
            << "  @+0x" << std::hex << (baseOff + pos) << std::dec << "\n";
        if (ChunkReader::IsContainer(h.id) && h.size >= 8 && depth < 12) {
            PrintTree(data.subspan(payloadOff, h.size), depth + 1,
                      baseOff + payloadOff, chunkCount, out);
        }
        pos = payloadEnd;
    }
}

void Info(const fs::path& path, std::ostringstream& out) {
    std::vector<uint8_t> raw;
    if (!LoadRaw(path, raw)) {
        out << "error: cannot read " << path.string() << "\n";
        return;
    }
    std::vector<uint8_t> bytes;
    bool wasCompressed = false;
    if (LZCDecompressor::IsCompressed(raw)) {
        auto r = LZCDecompressor::Decompress(raw);
        if (!r) {
            out << "error: JDLZ decompress failed: " << r.error << "\n";
            return;
        }
        bytes = std::move(r.value);
        wasCompressed = true;
    } else {
        bytes = std::move(raw);
    }

    out << "file: " << path.string() << "\n";
    out << "on-disk size: " << (wasCompressed ? bytes.size() : bytes.size());
    if (wasCompressed) out << " bytes (decompressed; JDLZ-compressed on disk)";
    out << "\n----------------------------------------\n";
    size_t count = 0;
    PrintTree(bytes, 0, 0, count, out);
    out << "----------------------------------------\n";
    out << count << " chunk(s)\n";
}

} // namespace

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_teamvanilla_mwtools_NativeBridge_identify(JNIEnv* env, jobject /*thiz*/, jstring jpath) {
    fs::path path = JStringToStd(env, jpath);
    std::ostringstream out;
    if (!fs::exists(path)) {
        out << "error: no such file: " << path.string() << "\n";
    } else {
        Identify(path, out);
    }
    return StdToJString(env, out.str());
}

JNIEXPORT jstring JNICALL
Java_com_teamvanilla_mwtools_NativeBridge_info(JNIEnv* env, jobject /*thiz*/, jstring jpath) {
    fs::path path = JStringToStd(env, jpath);
    std::ostringstream out;
    if (!fs::exists(path)) {
        out << "error: no such file: " << path.string() << "\n";
    } else {
        Info(path, out);
    }
    return StdToJString(env, out.str());
}

} // extern "C"
