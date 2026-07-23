#include "formats/AnimationBank.h"
#include "core/LZCDecompressor.h"
#include "core/Logger.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <unordered_map>

namespace nfsmw {

namespace {

std::string Lower(const std::string& s) {
    std::string o; o.reserve(s.size());
    for (char c : s) o.push_back((char)std::tolower((unsigned char)c));
    return o;
}

// Split a clip symbol into {animObject, boundObject, channel}.
//   ANM_AirlinerA_10_XO_AirlinerA_1b_LB_10_q
//   └ANM_┘└animObj┘└NN┘└_XO_┘└──── boundObject ────┘└_q┘
bool ParseClipName(const std::string& s, AnimClip& out) {
    if (s.rfind("ANM_", 0) != 0) return false;

    AnimChannel ch = AnimChannel::None;
    if (s.size() >= 2) {
        const std::string suf = s.substr(s.size() - 2);
        if      (suf == "_q") ch = AnimChannel::Rotation;
        else if (suf == "_t") ch = AnimChannel::Translation;
        else if (suf == "_s") ch = AnimChannel::Scale;
    }
    if (ch == AnimChannel::None) return false;

    out.name    = s;
    out.channel = ch;

    // boundObject = text after "_XO_" up to the channel suffix.
    const std::string body = s.substr(0, s.size() - 2);   // drop _q/_t/_s
    const size_t xo = body.find("_XO_");
    if (xo != std::string::npos)
        out.boundObject = body.substr(xo + 4);
    else
        out.boundObject.clear();

    // animObject = "ANM_" .. up to the first "_<digits>" or "_XO_".
    std::string mid = (xo != std::string::npos) ? body.substr(4, xo - 4) : body.substr(4);
    // strip a trailing "_NN" index if present
    size_t us = mid.rfind('_');
    if (us != std::string::npos && us + 1 < mid.size()) {
        bool allDigit = true;
        for (size_t i = us + 1; i < mid.size(); ++i)
            if (!std::isdigit((unsigned char)mid[i])) { allDigit = false; break; }
        if (allDigit) mid = mid.substr(0, us);
    }
    out.animObject = mid;
    if (out.boundObject.empty()) out.boundObject = mid;
    return true;
}

} // namespace

const AnimatedObject* AnimBankInventory::ForObjectName(const std::string& solidName) const {
    if (solidName.empty()) return nullptr;
    const std::string ln = Lower(solidName);
    const AnimatedObject* best = nullptr;
    for (const auto& ao : objects) {
        const std::string lo = Lower(ao.object);
        // match either direction: object name contains the bound stem, or the
        // bound stem contains the object name (covers XO_/instance suffixes).
        if (ln.find(lo) != std::string::npos || lo.find(ln) != std::string::npos) {
            if (!best || ao.object.size() > best->object.size()) best = &ao;
        }
    }
    return best;
}

AnimBankInventory AnimationBankParser::Scan(std::span<const uint8_t> bytes) {
    AnimBankInventory inv;

    // Scan for null-terminated printable ASCII runs that start with "ANM_".
    // The __AnimationBank .data name table stores clip names exactly this way.
    std::string cur;
    cur.reserve(96);
    auto flush = [&]() {
        if (cur.size() >= 6 && cur.rfind("ANM_", 0) == 0) {
            AnimClip clip;
            if (ParseClipName(cur, clip)) inv.clips.push_back(std::move(clip));
        }
        cur.clear();
    };
    for (uint8_t c : bytes) {
        if (c >= 0x20 && c <= 0x7E) cur.push_back((char)c);
        else { flush(); }
    }
    flush();

    // De-duplicate identical clip symbols.
    std::sort(inv.clips.begin(), inv.clips.end(),
              [](const AnimClip& a, const AnimClip& b) { return a.name < b.name; });
    inv.clips.erase(std::unique(inv.clips.begin(), inv.clips.end(),
                    [](const AnimClip& a, const AnimClip& b) { return a.name == b.name; }),
                    inv.clips.end());

    // Group by bound object.
    std::unordered_map<std::string, size_t> idx;
    for (const auto& c : inv.clips) {
        const std::string key = Lower(c.boundObject);
        auto it = idx.find(key);
        if (it == idx.end()) {
            idx[key] = inv.objects.size();
            AnimatedObject ao;
            ao.object = c.boundObject;
            inv.objects.push_back(ao);
            it = idx.find(key);
        }
        AnimatedObject& ao = inv.objects[it->second];
        ++ao.clipCount;
        switch (c.channel) {
            case AnimChannel::Rotation:    ao.hasRotation    = true; break;
            case AnimChannel::Translation: ao.hasTranslation = true; break;
            case AnimChannel::Scale:       ao.hasScale       = true; break;
            default: break;
        }
    }

    std::sort(inv.objects.begin(), inv.objects.end(),
              [](const AnimatedObject& a, const AnimatedObject& b) { return a.object < b.object; });
    return inv;
}

Result<AnimBankInventory> AnimationBankParser::ScanFile(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return Result<AnimBankInventory>::Err("Cannot open " + path.string());
    std::vector<uint8_t> raw(static_cast<size_t>(f.tellg()));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(raw.size()));

    // Whole-file JDLZ container? decompress and scan the plain bytes.
    if (raw.size() >= 4 && std::memcmp(raw.data(), "JDLZ", 4) == 0) {
        auto dr = LZCDecompressor::Decompress(raw);
        if (dr) {
            auto inv = Scan(dr.value);
            LOG_INFO("AnimationBankParser: {} clips / {} objects in {} (JDLZ)",
                     inv.clips.size(), inv.objects.size(), path.filename().string());
            return Result<AnimBankInventory>::Ok(std::move(inv));
        }
    }
    auto inv = Scan(raw);
    LOG_INFO("AnimationBankParser: {} clips / {} objects in {}",
             inv.clips.size(), inv.objects.size(), path.filename().string());
    return Result<AnimBankInventory>::Ok(std::move(inv));
}

} // namespace nfsmw
