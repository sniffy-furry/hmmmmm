#pragma once
// ─── formats/MPFFile.h ────────────────────────────────────────────────────────
// Parser for EA PathFinder v5 (.MPF) interactive-music routing graphs.
//
// MPF maps in-game EventIDs → NodeIDs → MUS section indices.
// Format reverse-engineered by xan1242 (MPFmaster, MIT).
//
// Binary layout (little-endian):
//   [0x00] char[4]   "MPFF"
//   [0x04] uint32    version (5)
//   [0x08] uint32    nodeCount
//   [0x0C] uint32    eventCount
//   [0x10] uint32    nodeTableOffset
//   [0x14] uint32    eventTableOffset
//   [0x18] uint32    stringPoolOffset
//   [0x1C] uint32    stringPoolSize
//
//   Node entry (16 bytes):
//     uint32 nodeID
//     uint32 sectionIndex   — index into paired MUSFile::sections
//     uint32 nameOffset     — from stringPoolOffset
//     uint32 flags
//
//   Event entry (12 bytes):
//     uint32 eventID
//     uint32 nodeID
//     uint32 nameOffset
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "formats/MUSFile.h"
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
struct MPFNode {
    uint32_t    nodeID       = 0;
    uint32_t    sectionIndex = 0;  ///< index into MUSFile::sections
    std::string name;
    uint32_t    flags        = 0;
};

struct MPFEvent {
    uint32_t    eventID = 0;
    uint32_t    nodeID  = 0;
    std::string name;
};

struct MPFFile {
    std::string                              name;
    std::vector<MPFNode>                     nodes;
    std::vector<MPFEvent>                    events;
    std::filesystem::path                    path;

    /// Resolved after calling MPFParser::Resolve():
    /// eventID → MUSSection index (into the paired MUSFile::sections)
    std::unordered_map<uint32_t, uint32_t>   eventToSection;

    /// Lookup helpers (post-resolve)
    const MPFNode*  FindNode(uint32_t nodeID)   const;
    const MPFEvent* FindEvent(uint32_t eventID) const;
};

// ─────────────────────────────────────────────────────────────────────────────
class MPFParser {
public:
    /// Parse MPF from disk.
    static Result<MPFFile> Load(const std::filesystem::path& path);

    /// Populate mpf.eventToSection using the node table and the paired MUS.
    static void Resolve(MPFFile& mpf, const MUSFile& mus);
};

} // namespace nfsmw
