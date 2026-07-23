#include "formats/NisEventSequence.h"
#include "core/StringHash.h"
#include "core/ChunkReader.h"
#include "Common.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <unordered_set>

namespace nfsmw {

namespace {

// ── ENIS verb table (ground-truth hashes from the encyclopedia RE data, so
//    label resolution does not depend on our own Lookup2Hash being bit-exact).
struct VerbHash { uint32_t hash; const char* name; };
constexpr std::array<VerbHash, 33> kEnisVerbs = {{
    {0x8da05b3du, "ENISAeroDynamics"},   {0xa01a259bu, "ENISBrakelock"},
    {0x6a5265b2u, "ENISBurnout"},        {0x991dd250u, "ENISCarDamageReset"},
    {0xa18e1896u, "ENISCarPitch"},       {0x3081837du, "ENISCarRoll"},
    {0xfb3dad79u, "ENISCarShake"},       {0x22aa36ceu, "ENISConstraint"},
    {0x3f56a04eu, "ENISCopCarDoors"},    {0x83e731c7u, "ENISCopLights"},
    {0x257a10a8u, "ENISDetach"},         {0x74e96511u, "ENISDetail"},
    {0x49d34ff1u, "ENISFakeFar"},        {0x7e045d1fu, "ENISFreeze"},
    {0x534bee88u, "ENISHideCharacter"},  {0x5e565165u, "ENISLights"},
    {0xfc382d92u, "ENISMotionBlur"},     {0x00eb198bu, "ENISNeutralRev"},
    {0xeb348f62u, "ENISNitro"},          {0xa87e176du, "ENISNukeSmack"},
    {0x37a3ee72u, "ENISOverlayMessage"}, {0x86d368cbu, "ENISPixelate"},
    {0x58aae7a8u, "ENISPlayEffect"},     {0x6a465054u, "ENISRain"},
    {0xcf01b27eu, "ENISReattach"},       {0xb9a02fa8u, "ENISRoadNoise"},
    {0x9f79a4edu, "ENISScreenFlash"},    {0x7282975du, "ENISSteering"},
    {0xc0582354u, "ENISStopEffects"},    {0xf699bf04u, "ENISTimeOfDay"},
    {0x41d652bbu, "ENISVisualLook"},     {0x509649b1u, "ENISWolrdGeometry"},
    {0x62f10875u, "ENISWorldAnimTrigger"},
}};

// Generic engine events that also appear in NIS timelines (hashes computed from
// the names via Lookup2Hash at load — a self-check of that implementation).
constexpr std::array<const char*, 6> kGenericVerbs = {{
    "EPlayObjectEffect", "ECameraPhotoFinish", "ESpawnExplosion",
    "EDDaySpeech", "ECellCall", "EStopObjectEffect",
}};

bool RdU32(std::span<const uint8_t> b, size_t off, uint32_t& v) {
    if (off + 4 > b.size()) return false;
    std::memcpy(&v, b.data() + off, 4);
    return true;
}

float AsF32(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

std::string HexId(uint32_t h) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%08x", h);
    return std::string(buf);
}

size_t Align4(size_t v) { return (v + 3) & ~size_t(3); }

// Recursively find the first EventSequenceChunk payload in a chunk tree.
bool FindFirstES(std::span<const uint8_t> data, std::span<const uint8_t>& out) {
    size_t pos = 0;
    while (pos + 8 <= data.size()) {
        uint32_t id = 0, sz = 0;
        std::memcpy(&id, data.data() + pos, 4);
        std::memcpy(&sz, data.data() + pos + 4, 4);
        if (id == 0 && sz == 0) break;
        const size_t payOff = pos + 8;
        if (payOff + sz > data.size()) break;
        if (id == ChunkID::EventSequenceChunk) {
            out = data.subspan(payOff, sz);
            return true;
        }
        if (ChunkReader::IsContainer(id) && sz >= 8) {
            if (FindFirstES(data.subspan(payOff, sz), out)) return true;
        }
        pos = payOff + sz;
    }
    return false;
}

} // namespace

NisSceneTimeline NisEventSequenceParser::Parse(std::span<const uint8_t> p) {
    NisSceneTimeline tl;
    const size_t N = p.size();
    // Header: registry base 0x28; row 0 is the string-table descriptor.
    if (N < 0x38) return tl;

    uint32_t numEntries = 0;
    if (!RdU32(p, 0x20, numEntries)) return tl;
    if (numEntries == 0 || numEntries > 4096) return tl;
    if (0x28 + size_t(numEntries) * 16 > N) return tl;

    uint32_t strOff = 0;
    RdU32(p, 0x34, strOff);

    // ── String table: greedy run of null-terminated printable C strings. ──────
    std::vector<std::string> names;
    size_t strTableEnd = 0x28 + strOff;
    {
        size_t sp = 0x28 + strOff;
        while (sp < N && p[sp] >= 0x20 && p[sp] < 0x7F) {
            size_t e = sp;
            while (e < N && p[e] != 0) ++e;
            names.emplace_back(reinterpret_cast<const char*>(p.data() + sp), e - sp);
            sp = e + 1;
            if (names.size() > 8192) break; // pathological guard
        }
        strTableEnd = sp;
    }

    // hash → name resolver.
    std::unordered_map<uint32_t, std::string> h2n;
    for (const auto& v : kEnisVerbs) h2n.emplace(v.hash, v.name);
    for (const auto* g : kGenericVerbs) h2n.emplace(Lookup2Hash(g), g);
    for (const auto& s : names) if (!s.empty()) h2n.emplace(Lookup2Hash(s), s);

    // ── Registry rows (16 bytes each): {u16 seq, char[2] tag, u8 3, u8 sz,
    //    u16 0, u32 nameHash, u32 arenaOff}. ────────────────────────────────────
    std::unordered_set<uint32_t> aeHashes;
    std::vector<uint16_t>        nonemptyLe; // le seqs with runtimeSize > 0x10
    for (uint32_t i = 1; i < numEntries; ++i) {
        const size_t o = 0x28 + size_t(i) * 16;
        uint16_t seq = 0;
        std::memcpy(&seq, p.data() + o, 2);
        const char t0 = static_cast<char>(p[o + 2]);
        const char t1 = static_cast<char>(p[o + 3]);
        const uint8_t sz = p[o + 5];
        uint32_t h = 0;
        std::memcpy(&h, p.data() + o + 8, 4);

        if (t0 == 'A' && t1 == 'e') {
            aeHashes.insert(h);
        } else if (t0 == 'l' && t1 == 'e') {
            if (sz > 0x10) nonemptyLe.push_back(seq);
        } else if (t0 == 'N' && t1 == 'i' && tl.sceneName.empty()) {
            auto it = h2n.find(h);
            if (it != h2n.end()) tl.sceneName = it->second;
        }
    }
    std::sort(nonemptyLe.begin(), nonemptyLe.end());

    // Data region begins right after the string table, 4-byte aligned.
    const size_t dataStart = Align4(strTableEnd);

    // ── Pass 1: timeline items (le payloads) → event-hash lists, keyed by seq. ─
    // Self-syncing on the {count,0,0,0} descriptor header + a payload self-hash
    // check, so it never desyncs on unrelated records. Best-effort: used only to
    // *label* markers; failure here leaves markers unlabeled, never invalid.
    std::unordered_map<int, std::vector<uint32_t>> itemEvents; // seq → event hashes
    if (!nonemptyLe.empty()) {
        size_t off = dataStart;
        size_t itemIndex = 0;
        int guard = 0;
        while (off + 16 <= N && guard < 200000) {
            ++guard;
            uint32_t cnt = 0, z1 = 0, z2 = 0, z3 = 0;
            RdU32(p, off, cnt);
            RdU32(p, off + 4, z1);
            RdU32(p, off + 8, z2);
            RdU32(p, off + 12, z3);
            if (cnt >= 1 && cnt <= 32 && z1 == 0 && z2 == 0 && z3 == 0) {
                struct Desc { size_t base; uint32_t hh, szk, offk; };
                std::vector<Desc> descs;
                descs.reserve(cnt);
                size_t d = off + 16;
                bool ok = true;
                for (uint32_t j = 0; j < cnt; ++j) {
                    if (d + 16 > N) { ok = false; break; }
                    uint32_t hh = 0, szk = 0, offk = 0, zz = 0;
                    RdU32(p, d, hh);
                    RdU32(p, d + 4, szk);
                    RdU32(p, d + 8, offk);
                    RdU32(p, d + 12, zz);
                    if (szk > 0x200 || zz != 0 || offk > 0x4000) { ok = false; break; }
                    descs.push_back({d, hh, szk, offk});
                    d += 16;
                }
                if (ok && !descs.empty()) {
                    size_t pend = d;
                    std::vector<uint32_t> evs;
                    for (const auto& ds : descs) {
                        const size_t pp = ds.base + ds.offk;
                        uint32_t first = 0;
                        if (pp + ds.szk > N || !RdU32(p, pp, first) || first != ds.hh) {
                            ok = false;
                            break;
                        }
                        evs.push_back(ds.hh);
                        pend = std::max(pend, pp + ds.szk);
                    }
                    if (ok) {
                        if (itemIndex < nonemptyLe.size())
                            itemEvents[static_cast<int>(nonemptyLe[itemIndex])] = std::move(evs);
                        ++itemIndex;
                        off = Align4(pend);
                        continue;
                    }
                }
            }
            off += 4;
        }
    }

    // ── Pass 2: action schedules — sync on registered 'Ae' self-hashes. ───────
    // Layout: {self, k, n, f32 dur, keys[k], times[n], refs[k+n]}. The timed
    // half (refs[k..]) maps times[i] → an "seq.le" timeline item.
    float maxDur = 0.f;
    {
        size_t off = dataStart;
        int guard = 0;
        while (off + 16 <= N && guard < 400000) {
            ++guard;
            uint32_t self = 0;
            RdU32(p, off, self);
            if (aeHashes.find(self) == aeHashes.end()) { off += 4; continue; }

            uint32_t k = 0, nn = 0, durU = 0;
            RdU32(p, off + 4, k);
            RdU32(p, off + 8, nn);
            RdU32(p, off + 12, durU);
            if (k > 256 || nn > 2048) { off += 4; continue; }
            const size_t need = off + 16 + 8 * size_t(k) + 8 * size_t(nn);
            if (need > N) { off += 4; continue; }
            const float dur = AsF32(durU);
            if (!(dur >= 0.f && dur < 100000.f)) { off += 4; continue; }

            std::string aname;
            if (auto it = h2n.find(self); it != h2n.end()) aname = it->second;
            std::string actor = aname;
            if (actor.rfind("Action", 0) == 0) actor = actor.substr(6);

            NisActionLane lane;
            lane.name     = aname.empty() ? HexId(self) : aname;
            lane.actor    = actor;
            lane.duration = dur;

            const size_t timesBase = off + 16 + 4 * size_t(k);
            const size_t refsBase  = off + 16 + 4 * size_t(k) + 4 * size_t(nn);
            for (uint32_t idx = 0; idx < nn; ++idx) {
                uint32_t tu = 0;
                RdU32(p, timesBase + 4 * size_t(idx), tu);
                const float tt = AsF32(tu);
                if (!(tt >= 0.f && tt <= dur + 0.001f)) continue;

                uint32_t ref = 0;
                RdU32(p, refsBase + 4 * size_t(k + idx), ref);

                std::string label;
                const uint16_t rseq = static_cast<uint16_t>(ref & 0xFFFFu);
                const char rc0 = static_cast<char>((ref >> 16) & 0xFFu);
                const char rc1 = static_cast<char>((ref >> 24) & 0xFFu);
                if (rc0 == 'l' && rc1 == 'e') {
                    auto iv = itemEvents.find(static_cast<int>(rseq));
                    if (iv != itemEvents.end()) {
                        for (uint32_t hh : iv->second) {
                            auto hn = h2n.find(hh);
                            const std::string nm = (hn != h2n.end()) ? hn->second : HexId(hh);
                            if (!label.empty()) label += "+";
                            label += nm;
                        }
                    }
                }

                NisTimelineEvent ev;
                ev.time  = tt;
                ev.actor = actor;
                ev.label = std::move(label);
                tl.events.push_back(std::move(ev));
                ++lane.eventCount;
            }

            maxDur = std::max(maxDur, dur);
            tl.lanes.push_back(std::move(lane));
            off = need;
        }
    }

    std::stable_sort(tl.events.begin(), tl.events.end(),
                     [](const NisTimelineEvent& a, const NisTimelineEvent& b) {
                         return a.time < b.time;
                     });
    tl.actionCount = static_cast<int>(tl.lanes.size());
    tl.itemCount   = static_cast<int>(itemEvents.size());
    tl.duration    = maxDur;
    tl.valid       = (!tl.lanes.empty() && maxDur > 0.001f);
    return tl;
}

NisSceneTimeline NisEventSequenceParser::ParseBundle(std::span<const uint8_t> bundleBytes) {
    std::span<const uint8_t> payload;
    if (FindFirstES(bundleBytes, payload))
        return Parse(payload);
    return NisSceneTimeline{};
}

} // namespace nfsmw
