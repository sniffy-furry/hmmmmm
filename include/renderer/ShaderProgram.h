#pragma once
#include "Common.h"
#include <string>
#include <string_view>
#include <filesystem>

namespace nfsmw {

/// RAII wrapper around an OpenGL shader program.
class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();

    // Non-copyable, movable
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&& o) noexcept : id_(o.id_) { o.id_ = 0; }
    ShaderProgram& operator=(ShaderProgram&& o) noexcept {
        if (this != &o) { Delete(); id_ = o.id_; o.id_ = 0; }
        return *this;
    }

    /// Build from GLSL source strings.
    Result<void> Build(std::string_view vertSrc, std::string_view fragSrc);

    /// Build from shader files on disk.
    Result<void> BuildFromFiles(const std::filesystem::path& vert,
                                 const std::filesystem::path& frag);

    void Bind()   const;
    void Unbind() const;
    bool Valid()  const { return id_ != 0; }

    // ─── Uniform setters ──────────────────────────────────────────────────
    void SetBool  (std::string_view name, bool v)         const;
    void SetInt   (std::string_view name, int v)          const;
    void SetFloat (std::string_view name, float v)        const;
    void SetVec2  (std::string_view name, const glm::vec2& v) const;
    void SetVec3  (std::string_view name, const glm::vec3& v) const;
    void SetVec4  (std::string_view name, const glm::vec4& v) const;
    void SetMat4  (std::string_view name, const glm::mat4& v) const;
    void SetMat3  (std::string_view name, const glm::mat3& v) const;

    uint32_t ID() const { return id_; }

private:
    uint32_t id_ = 0;

    static Result<uint32_t> CompileStage(uint32_t type, std::string_view src);
    void Delete();
    int  GetUniformLocation(std::string_view name) const;
};

} // namespace nfsmw
