#include "formats/NisAnim.h"
#include "core/Logger.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstring>

namespace nfsmw {

namespace {

#pragma pack(push, 1)
struct Elf32_Ehdr {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};
struct Elf32_Shdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
};
struct Elf32_Sym {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
};
#pragma pack(pop)

constexpr uint32_t kShtSymtab = 2;

std::string ReadCString(std::span<const uint8_t> strtab, uint32_t offset) {
    if (offset >= strtab.size()) return {};
    const char* base = reinterpret_cast<const char*>(strtab.data()) + offset;
    size_t maxLen = strtab.size() - offset;
    size_t len = 0;
    while (len < maxLen && base[len] != '\0') ++len;
    return std::string(base, len);
}

} // namespace

std::vector<float> NisAnimClip::BoneFloats(size_t boneSymbolIdx) const {
    std::vector<float> out;
    if (boneSymbolIdx >= symbols.size()) return out;
    const NisAnimSymbol& sym = symbols[boneSymbolIdx];
    if (sym.sectionIndex >= sections.size()) return out;
    const NisAnimSection& sec = sections[sym.sectionIndex];
    if (sym.value + sym.size > sec.bytes.size()) return out;
    size_t count = sym.size / sizeof(float);
    out.resize(count);
    std::memcpy(out.data(), sec.bytes.data() + sym.value, count * sizeof(float));
    return out;
}

Result<NisAnimClip> NisAnimParser::Parse(std::span<const uint8_t> payload) {
    // 8-byte 0x11 sentinel precedes the embedded ELF (verified by hex
    // inspection of NIS/Scene_ArrestF02_BundleB.bun this session).
    constexpr size_t kSentinelSize = 8;
    if (payload.size() < kSentinelSize + sizeof(Elf32_Ehdr))
        return Result<NisAnimClip>::Err("NisAnim: payload too small");

    std::span<const uint8_t> elf = payload.subspan(kSentinelSize);

    Elf32_Ehdr ehdr{};
    std::memcpy(&ehdr, elf.data(), sizeof(ehdr));

    if (!(elf[0] == 0x7F && elf[1] == 'E' && elf[2] == 'L' && elf[3] == 'F'))
        return Result<NisAnimClip>::Err("NisAnim: missing ELF magic after sentinel");

    if (ehdr.e_shoff == 0 || ehdr.e_shnum == 0)
        return Result<NisAnimClip>::Err("NisAnim: no section headers");
    if (static_cast<uint64_t>(ehdr.e_shoff) +
            static_cast<uint64_t>(ehdr.e_shnum) * ehdr.e_shentsize >
        elf.size())
        return Result<NisAnimClip>::Err("NisAnim: section header table overruns payload");

    NisAnimClip clip;

    // Pass 1: raw section headers + bytes (name resolved after we know shstrndx).
    struct RawShdr { Elf32_Shdr hdr; };
    std::vector<RawShdr> raw(ehdr.e_shnum);
    for (uint16_t i = 0; i < ehdr.e_shnum; ++i) {
        size_t off = ehdr.e_shoff + size_t(i) * ehdr.e_shentsize;
        std::memcpy(&raw[i].hdr, elf.data() + off, sizeof(Elf32_Shdr));
    }

    clip.sections.resize(ehdr.e_shnum);
    for (uint16_t i = 0; i < ehdr.e_shnum; ++i) {
        const Elf32_Shdr& sh = raw[i].hdr;
        NisAnimSection& sec = clip.sections[i];
        sec.type   = sh.sh_type;
        sec.offset = sh.sh_offset;
        // SHT_NOBITS (8) sections (.bss-like) have no file bytes.
        if (sh.sh_type != 8 && sh.sh_size > 0) {
            if (static_cast<uint64_t>(sh.sh_offset) + sh.sh_size > elf.size())
                return Result<NisAnimClip>::Err("NisAnim: section data overruns payload");
            sec.bytes.assign(elf.begin() + sh.sh_offset,
                              elf.begin() + sh.sh_offset + sh.sh_size);
        }
    }

    // Resolve section names via the shstrtab section.
    if (ehdr.e_shstrndx < clip.sections.size()) {
        const auto& shstrtab = clip.sections[ehdr.e_shstrndx].bytes;
        for (uint16_t i = 0; i < ehdr.e_shnum; ++i)
            clip.sections[i].name = ReadCString(shstrtab, raw[i].hdr.sh_name);
    }

    // Find .symtab + its linked .strtab, extract data symbols.
    for (uint16_t i = 0; i < ehdr.e_shnum; ++i) {
        const Elf32_Shdr& sh = raw[i].hdr;
        if (sh.sh_type != kShtSymtab) continue;
        if (sh.sh_link >= clip.sections.size()) continue;

        const auto& symtabBytes = clip.sections[i].bytes;
        const auto& strtabBytes = clip.sections[sh.sh_link].bytes;
        size_t symCount = sh.sh_entsize > 0 ? symtabBytes.size() / sh.sh_entsize
                                             : symtabBytes.size() / sizeof(Elf32_Sym);

        for (size_t s = 0; s < symCount; ++s) {
            Elf32_Sym sym{};
            std::memcpy(&sym, symtabBytes.data() + s * sizeof(Elf32_Sym), sizeof(Elf32_Sym));
            std::string name = ReadCString(strtabBytes, sym.st_name);
            if (name.empty() || name.rfind("__", 0) != 0) continue; // only EAGL "__*" markers

            NisAnimSymbol entry;
            entry.name         = name;
            entry.sectionIndex = sym.st_shndx;
            entry.value        = sym.st_value;
            entry.size         = sym.st_size;
            clip.symbols.push_back(entry);

            const size_t idx = clip.symbols.size() - 1;
            if (name.rfind("__Bone:::", 0) == 0)
                clip.boneSymbols.push_back(idx);
            else if (name.rfind("__Skeleton:::", 0) == 0 && clip.skeletonName.empty())
                clip.skeletonName = name.substr(std::string("__Skeleton:::").size());
        }
    }

    // ── Parse skeleton bind pose ─────────────────────────────────────────────
    // Find .data section, then locate the __Skeleton::: symbol's offset into
    // it and decode the Skeleton header + per-bone records (112 bytes each).
    {
        size_t dataSectIdx = SIZE_MAX;
        for (size_t si = 0; si < clip.sections.size(); ++si) {
            if (clip.sections[si].name == ".data") { dataSectIdx = si; break; }
        }
        if (dataSectIdx != SIZE_MAX) {
            const auto& dataSec = clip.sections[dataSectIdx];
            for (const auto& sym : clip.symbols) {
                if (sym.name.rfind("__Skeleton:::", 0) != 0) continue;
                if (sym.sectionIndex != (uint32_t)dataSectIdx) continue;

                constexpr size_t kHdrSize    = 16;  // 4×uint16 + 2×uint32
                constexpr size_t kBoneStride = 112; // V3+I32+V4+V3+I32+M4x4
                if (sym.value + kHdrSize > dataSec.bytes.size()) break;

                uint32_t numBones = 0;
                std::memcpy(&numBones, dataSec.bytes.data() + sym.value + 8, 4);

                if (numBones == 0 ||
                    sym.value + kHdrSize + numBones * kBoneStride > dataSec.bytes.size())
                    break;

                NisAnimSkeleton skel;
                skel.numBones = numBones;
                skel.bones.resize(numBones);

                const uint8_t* boneBase = dataSec.bytes.data() + sym.value + kHdrSize;
                for (uint32_t b = 0; b < numBones; ++b) {
                    const uint8_t* bp = boneBase + b * kBoneStride;
                    const float*   f  = reinterpret_cast<const float*>(bp);
                    NisAnimBone& bone = skel.bones[b];
                    bone.scale = { f[0], f[1], f[2] };
                    std::memcpy(&bone.parent, bp + 12, sizeof(int32_t));
                    // VECTOR4 in file = (qx, qy, qz, qw); glm::quat ctor = (w,x,y,z)
                    bone.rotation    = glm::quat(f[7], f[4], f[5], f[6]);
                    bone.translation = { f[8], f[9], f[10] };
                }
                clip.skeleton = std::move(skel);
                break;
            }
        }
    }

    return Result<NisAnimClip>::Ok(std::move(clip));
}

bool EvaluateNisPose(const NisAnimSkeleton& skel, float t, NisPoseMode mode,
                     std::vector<glm::mat4>& outWorld) {
    const uint32_t N = skel.numBones;
    if (N == 0 || skel.bones.size() < N) {
        outWorld.clear();
        return false;
    }
    outWorld.assign(N, glm::mat4(1.f));

    for (uint32_t i = 0; i < N; ++i) {
        const NisAnimBone& b = skel.bones[i];
        glm::quat rot   = b.rotation;
        glm::vec3 trans = b.translation;
        glm::vec3 scl   = b.scale;

        // ── Keyframe hook ────────────────────────────────────────────────────
        // The authored per-bone sample for this frame would be composed with the
        // bind transform HERE (rot = sample_q * rot, trans += sample_t, …). The
        // __AnimationBank _q/_t/_s channels are 8-bit-quantised streams whose
        // dequantisation math is still open (encyclopedia C10.3), so no real
        // sample is available yet; drop it in at this point once solved.
        if (mode == NisPoseMode::SyntheticIdle) {
            // Deliberately synthetic, NON-authored motion — a tiny yaw sway,
            // phase-shifted per bone, purely to prove the posing path is live.
            const float phase = 0.6f * t + 0.35f * static_cast<float>(i);
            const float ang   = 0.05f * std::sin(phase);
            rot = rot * glm::angleAxis(ang, glm::vec3(0.f, 0.f, 1.f));
        }

        const glm::mat4 local = glm::translate(glm::mat4(1.f), trans)
                              * glm::mat4_cast(rot)
                              * glm::scale(glm::mat4(1.f), scl);
        const int32_t parent = b.parent;
        outWorld[i] = (parent >= 0 && parent < static_cast<int32_t>(i))
                          ? outWorld[static_cast<uint32_t>(parent)] * local
                          : local;
    }
    return true;
}

} // namespace nfsmw
