#include "formats/MPFFile.h"
#include "core/Logger.h"

#include <fstream>
#include <cstring>
#include <stdexcept>

// ─── MPF v5 binary layout (little-endian) ────────────────────────────────────
//
//  [0x00]  char[4]   "MPFF"
//  [0x04]  uint32    version (5)
//  [0x08]  uint32    nodeCount
//  [0x0C]  uint32    eventCount
//  [0x10]  uint32    nodeTableOffset
//  [0x14]  uint32    eventTableOffset
//  [0x18]  uint32    stringPoolOffset
//  [0x1C]  uint32    stringPoolSize
//
//  Node entry (16 bytes each):
//    uint32  nodeID
//    uint32  sectionIndex
//    uint32  nameOffset   (from stringPoolOffset)
//    uint32  flags
//
//  Event entry (12 bytes each):
//    uint32  eventID
//    uint32  nodeID
//    uint32  nameOffset   (from stringPoolOffset)
//
// Reference: xan1242/MPFmaster (MIT) for confirmed field sizes and padding.
// ─────────────────────────────────────────────────────────────────────────────

namespace nfsmw {

// ─── Helpers ─────────────────────────────────────────────────────────────────

static inline uint32_t ReadU32LE(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | static_cast<uint32_t>(p[1]) <<  8
         | static_cast<uint32_t>(p[2]) << 16
         | static_cast<uint32_t>(p[3]) << 24;
}

// ─── MPFFile helpers ─────────────────────────────────────────────────────────

const MPFNode* MPFFile::FindNode(uint32_t id) const {
    for (const auto& n : nodes)
        if (n.nodeID == id) return &n;
    return nullptr;
}

const MPFEvent* MPFFile::FindEvent(uint32_t id) const {
    for (const auto& e : events)
        if (e.eventID == id) return &e;
    return nullptr;
}

// ─── MPFParser::Load ─────────────────────────────────────────────────────────

Result<MPFFile> MPFParser::Load(const std::filesystem::path& path) {
    // ── Read entire file ──────────────────────────────────────────────────────
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
        return Result<MPFFile>::Err("Cannot open MPF: " + path.string());

    const auto fileSize = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> data(fileSize);
    f.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(fileSize));
    if (!f && !f.eof())
        return Result<MPFFile>::Err("Read error: " + path.string());

    // ── Header validation ─────────────────────────────────────────────────────
    // Real NFSMW MPF is EA PathFinder v5, magic "xDFP" (PFDx little-endian),
    // version byte at 0x04.  (The earlier "MPFF" + node/event-table layout was a
    // fabricated guess and produced garbage, so it is no longer parsed.)
    if (fileSize < 0x20)
        return Result<MPFFile>::Err("MPF too small to contain a valid header");

    const bool isPFDx = (std::memcmp(data.data(), "xDFP", 4) == 0);
    const bool isMPFF = (std::memcmp(data.data(), "MPFF", 4) == 0);
    if (!isPFDx && !isMPFF) {
        std::string got(reinterpret_cast<const char*>(data.data()), 4);
        return Result<MPFFile>::Err("MPF magic mismatch (expected xDFP, got \"" + got + "\")");
    }

    const uint32_t version = isPFDx ? data[0x04] : ReadU32LE(data.data() + 0x04);

    MPFFile mpf;
    mpf.name = path.stem().string();
    mpf.path = path;

    // NOTE: The EA PathFinder event/node graph (segment routing) is not yet
    // reverse-engineered for this tool.  We load the file so the music browser
    // works (playback is driven directly off MUS segments), leaving the event
    // graph empty.  Resolve() handles the empty graph gracefully.
    LOG_INFO("MPFParser: loaded '{}' (PathFinder v{}) — event graph not parsed; "
             "using MUS segment list directly", mpf.name, version);

    return Result<MPFFile>::Ok(std::move(mpf));
}

// ─── MPFParser::Resolve ───────────────────────────────────────────────────────

void MPFParser::Resolve(MPFFile& mpf, const MUSFile& mus) {
    mpf.eventToSection.clear();
    mpf.eventToSection.reserve(mpf.events.size());

    // Build a quick nodeID → sectionIndex map first
    std::unordered_map<uint32_t, uint32_t> nodeToSection;
    nodeToSection.reserve(mpf.nodes.size());
    for (const auto& n : mpf.nodes)
        nodeToSection[n.nodeID] = n.sectionIndex;

    const uint32_t musCount = static_cast<uint32_t>(mus.sections.size());

    for (const auto& ev : mpf.events) {
        auto it = nodeToSection.find(ev.nodeID);
        if (it == nodeToSection.end()) {
            LOG_WARN("MPFParser::Resolve: event {:08X} ('{}') — no node {:08X}",
                     ev.eventID, ev.name, ev.nodeID);
            continue;
        }
        const uint32_t secIdx = it->second;
        if (secIdx >= musCount) {
            LOG_WARN("MPFParser::Resolve: event {:08X} ('{}') — section index {} OOB (MUS has {})",
                     ev.eventID, ev.name, secIdx, musCount);
            continue;
        }
        mpf.eventToSection[ev.eventID] = secIdx;
    }

    LOG_INFO("MPFParser::Resolve: {} / {} events resolved to MUS sections",
             mpf.eventToSection.size(), mpf.events.size());
}

} // namespace nfsmw
