#include "renderer/ShaderProgram.h"
#include "core/Logger.h"

#include "renderer/GLCompat.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>

namespace nfsmw {

#if defined(__ANDROID__)
namespace {
std::string ToGles300(std::string_view src) {
    std::string out(src);
    const std::string desktopVersion = "#version 330 core";
    const std::string esVersion = "#version 300 es\nprecision mediump float;";
    size_t pos = out.find(desktopVersion);
    if (pos != std::string::npos)
        out.replace(pos, desktopVersion.size(), esVersion);
    return out;
}
} // namespace
#endif

ShaderProgram::~ShaderProgram() {
    Delete();
}

void ShaderProgram::Delete() {
    if (id_) { glDeleteProgram(id_); id_ = 0; }
}

Result<uint32_t> ShaderProgram::CompileStage(uint32_t type, std::string_view src) {
    uint32_t shader = glCreateShader(type);
    const char* cstr = src.data();
    const int   len  = static_cast<int>(src.size());
    glShaderSource(shader, 1, &cstr, &len);
    glCompileShader(shader);

    int ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        return Result<uint32_t>::Err(std::string(log));
    }
    return Result<uint32_t>::Ok(shader);
}

Result<void> ShaderProgram::Build(std::string_view vertSrc, std::string_view fragSrc) {
#if defined(__ANDROID__)
    const std::string vertGles = ToGles300(vertSrc);
    const std::string fragGles = ToGles300(fragSrc);
    vertSrc = vertGles;
    fragSrc = fragGles;
#endif

    auto vs = CompileStage(GL_VERTEX_SHADER,   vertSrc);
    if (!vs) return Result<void>::Err("Vertex shader: " + vs.error);

    auto fs = CompileStage(GL_FRAGMENT_SHADER, fragSrc);
    if (!fs) { glDeleteShader(vs.value); return Result<void>::Err("Fragment shader: " + fs.error); }

    uint32_t prog = glCreateProgram();
    glAttachShader(prog, vs.value);
    glAttachShader(prog, fs.value);
    glLinkProgram(prog);
    glDeleteShader(vs.value);
    glDeleteShader(fs.value);

    int ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        glDeleteProgram(prog);
        return Result<void>::Err(std::string(log));
    }

    Delete();
    id_ = prog;
    return Result<void>::Ok();
}

Result<void> ShaderProgram::BuildFromFiles(const std::filesystem::path& vert,
                                            const std::filesystem::path& frag) {
    auto readFile = [](const std::filesystem::path& p) -> Result<std::string> {
        std::ifstream f(p);
        if (!f) return Result<std::string>::Err("Cannot open: " + p.string());
        std::ostringstream ss;
        ss << f.rdbuf();
        return Result<std::string>::Ok(ss.str());
    };
    auto vs = readFile(vert); if (!vs) return Result<void>::Err(vs.error);
    auto fs = readFile(frag); if (!fs) return Result<void>::Err(fs.error);
    return Build(vs.value, fs.value);
}

void ShaderProgram::Bind()   const { glUseProgram(id_); }
void ShaderProgram::Unbind() const { glUseProgram(0);   }

int ShaderProgram::GetUniformLocation(std::string_view name) const {
    // glGetUniformLocation needs a null-terminated string.
    char buf[64];
    const size_t n = std::min(name.size(), sizeof(buf) - 1);
    std::memcpy(buf, name.data(), n);
    buf[n] = '\0';
    return glGetUniformLocation(id_, buf);
}

void ShaderProgram::SetBool (std::string_view n, bool v)              const { glUniform1i (GetUniformLocation(n), (int)v); }
void ShaderProgram::SetInt  (std::string_view n, int v)               const { glUniform1i (GetUniformLocation(n), v); }
void ShaderProgram::SetFloat(std::string_view n, float v)             const { glUniform1f (GetUniformLocation(n), v); }
void ShaderProgram::SetVec2 (std::string_view n, const glm::vec2& v)  const { glUniform2fv(GetUniformLocation(n), 1, glm::value_ptr(v)); }
void ShaderProgram::SetVec3 (std::string_view n, const glm::vec3& v)  const { glUniform3fv(GetUniformLocation(n), 1, glm::value_ptr(v)); }
void ShaderProgram::SetVec4 (std::string_view n, const glm::vec4& v)  const { glUniform4fv(GetUniformLocation(n), 1, glm::value_ptr(v)); }
void ShaderProgram::SetMat4 (std::string_view n, const glm::mat4& v)  const { glUniformMatrix4fv(GetUniformLocation(n), 1, GL_FALSE, glm::value_ptr(v)); }
void ShaderProgram::SetMat3 (std::string_view n, const glm::mat3& v)  const { glUniformMatrix3fv(GetUniformLocation(n), 1, GL_FALSE, glm::value_ptr(v)); }

} // namespace nfsmw
