#pragma once
#include "Common.h"
#include <span>
#include <functional>
#include <unordered_map>

namespace nfsmw {

/// Universal EAGL chunk header (little-endian, 8 bytes).
#pragma pack(push, 1)
struct ChunkHeader {
    uint32_t id;    ///< Chunk type identifier
    uint32_t size;  ///< Byte length of payload (excluding the 8-byte header)
};
#pragma pack(pop)

static_assert(sizeof(ChunkHeader) == 8, "ChunkHeader must be 8 bytes");

/// Strip the leading 0x11 alignment padding many payloads carry.
inline std::span<const uint8_t> StripAlignPad(std::span<const uint8_t> p) {
    size_t i = 0;
    while (i < p.size() && p[i] == kChunkPadByte) ++i;
    return p.subspan(i);
}

// ─────────────────────────────────────────────────────────────────────────────
/// Dispatch-table based chunk-tree parser.
///
/// Usage:
///   ChunkReader reader;
///   reader.RegisterHandler(ChunkID::GeometryContainer, [](auto id, auto payload){ ... });
///   reader.Parse(fileBytes);
// ─────────────────────────────────────────────────────────────────────────────
class ChunkReader {
public:
    using Handler = std::function<void(uint32_t id, std::span<const uint8_t> payload)>;
    /// Handler that also receives the absolute payload offset within the parsed root buffer.
    using OffsetHandler = std::function<void(uint32_t id, std::span<const uint8_t> payload, size_t absOffset)>;

    /// Register a handler for a specific chunk ID.
    void RegisterHandler(uint32_t chunkId, Handler handler) {
        handlers_[chunkId] = [h = std::move(handler)](uint32_t id, std::span<const uint8_t> p, size_t) { h(id, p); };
    }
    void RegisterHandler(uint32_t chunkId, OffsetHandler handler) {
        handlers_[chunkId] = std::move(handler);
    }

    /// Register a catch-all handler invoked for every unregistered chunk ID.
    void SetUnknownHandler(OffsetHandler handler) {
        unknownHandler_ = std::move(handler);
    }

    /// Parse a flat buffer as a sequence of chunks.
    /// `baseOffset` is added to all reported offsets (use the absolute file
    /// offset of `data` so handlers can record patch locations).
    void Parse(std::span<const uint8_t> data, size_t baseOffset = 0) const;

    /// Recursively walk a nested chunk tree. Registered handlers win;
    /// unhandled container chunks (bit 31 set) are descended into.
    void ParseRecursive(std::span<const uint8_t> data, size_t baseOffset = 0, int depth = 0) const;

    /// Dump mode: prints every chunk ID + size to the logger (no handlers).
    static void Dump(std::span<const uint8_t> data, int depth = 0);

    /// Is this a container chunk (holds nested chunks)?
    static bool IsContainer(uint32_t id) { return (id & 0x80000000u) != 0; }

private:
    std::unordered_map<uint32_t, OffsetHandler> handlers_;
    OffsetHandler                               unknownHandler_;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Binary buffer reader — safe, bounds-checked sequential read helper.
// ─────────────────────────────────────────────────────────────────────────────
class BinaryReader {
public:
    explicit BinaryReader(std::span<const uint8_t> data)
        : data_(data), cursor_(0) {}

    bool Ok()        const { return cursor_ <= data_.size(); }
    size_t Pos()     const { return cursor_; }
    size_t Remaining() const { return cursor_ < data_.size() ? data_.size() - cursor_ : 0; }

    template<typename T>
    T Read() {
        static_assert(std::is_trivially_copyable_v<T>);
        T val{};
        if (cursor_ + sizeof(T) <= data_.size()) {
            std::memcpy(&val, data_.data() + cursor_, sizeof(T));
            cursor_ += sizeof(T);
        }
        return val;
    }

    template<typename T>
    T Peek() const {
        static_assert(std::is_trivially_copyable_v<T>);
        T val{};
        if (cursor_ + sizeof(T) <= data_.size())
            std::memcpy(&val, data_.data() + cursor_, sizeof(T));
        return val;
    }

    void Skip(size_t n) { cursor_ = std::min(cursor_ + n, data_.size()); }
    void Seek(size_t pos) { cursor_ = std::min(pos, data_.size()); }

    /// Skip the 0x11 alignment padding.
    void SkipAlignPad() {
        while (cursor_ < data_.size() && data_[cursor_] == kChunkPadByte) ++cursor_;
    }

    std::string ReadString(size_t maxLen) {
        std::string s;
        s.reserve(maxLen);
        size_t end = std::min(cursor_ + maxLen, data_.size());
        size_t i = cursor_;
        for (; i < end; ++i) {
            char c = static_cast<char>(data_[i]);
            if (c == '\0') break;
            s += c;
        }
        cursor_ = std::min(cursor_ + maxLen, data_.size()); // fixed-size field
        return s;
    }

    /// Read a null-terminated string, consuming exactly up to and including the null.
    std::string ReadCString(size_t maxLen = 256) {
        std::string s;
        for (size_t i = 0; i < maxLen && cursor_ < data_.size(); ++i) {
            char c = static_cast<char>(data_[cursor_++]);
            if (c == '\0') break;
            s += c;
        }
        return s;
    }

    std::span<const uint8_t> ReadBytes(size_t n) {
        if (cursor_ + n > data_.size()) n = data_.size() - cursor_;
        auto span = data_.subspan(cursor_, n);
        cursor_ += n;
        return span;
    }

    std::span<const uint8_t> RemainingBytes() const {
        return data_.subspan(std::min(cursor_, data_.size()));
    }

    float ReadFloat()    { return Read<float>(); }
    uint8_t  ReadU8()   { return Read<uint8_t>(); }
    uint16_t ReadU16()  { return Read<uint16_t>(); }
    uint32_t ReadU32()  { return Read<uint32_t>(); }
    int16_t  ReadI16()  { return Read<int16_t>(); }
    int32_t  ReadI32()  { return Read<int32_t>(); }

    glm::vec2 ReadVec2() { float x=ReadFloat(), y=ReadFloat(); return {x,y}; }
    glm::vec3 ReadVec3() { float x=ReadFloat(), y=ReadFloat(), z=ReadFloat(); return {x,y,z}; }
    glm::vec4 ReadVec4() { float x=ReadFloat(), y=ReadFloat(), z=ReadFloat(), w=ReadFloat(); return {x,y,z,w}; }

    /// File matrices are D3D row-major with row vectors (translation in row 3,
    /// floats 12..14). That memory layout is IDENTICAL to GLM's column-major
    /// column-vector convention, so a straight sequential read is correct.
    glm::mat4 ReadMat4() {
        glm::mat4 m;
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
                m[col][row] = ReadFloat();
        return m;
    }

private:
    std::span<const uint8_t> data_;
    size_t cursor_;
};

} // namespace nfsmw
