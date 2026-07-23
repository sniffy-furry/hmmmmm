#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>

namespace nfsmw {

class Logger {
public:
    /// Initialise with console + file sinks. Safe to call more than once.
    static void Init(const std::string& logFile = "mapeditor.log");

    /// Returns the logger; auto-initialises with a console-only sink if
    /// Init() was never called (keeps CLI tools and tests safe).
    static std::shared_ptr<spdlog::logger>& Get();

    static void Shutdown();

private:
    static std::shared_ptr<spdlog::logger> s_Logger;
};

// ─── Convenience macros ───────────────────────────────────────────────────────
#define LOG_TRACE(...)    ::nfsmw::Logger::Get()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)    ::nfsmw::Logger::Get()->debug(__VA_ARGS__)
#define LOG_INFO(...)     ::nfsmw::Logger::Get()->info(__VA_ARGS__)
#define LOG_WARN(...)     ::nfsmw::Logger::Get()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::nfsmw::Logger::Get()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::nfsmw::Logger::Get()->critical(__VA_ARGS__)

} // namespace nfsmw
