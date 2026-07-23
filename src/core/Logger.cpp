#include "core/Logger.h"

namespace nfsmw {

std::shared_ptr<spdlog::logger> Logger::s_Logger;

void Logger::Init(const std::string& logFile) {
    if (s_Logger) return;

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    try {
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true));
    } catch (const spdlog::spdlog_ex&) {
        // Read-only working directory: console-only logging is fine.
    }

    s_Logger = std::make_shared<spdlog::logger>("NFSMW", sinks.begin(), sinks.end());
    s_Logger->set_level(spdlog::level::debug);
    s_Logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::register_logger(s_Logger);
}

std::shared_ptr<spdlog::logger>& Logger::Get() {
    if (!s_Logger) {
        s_Logger = spdlog::stdout_color_mt("NFSMW");
        s_Logger->set_level(spdlog::level::info);
        s_Logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    }
    return s_Logger;
}

void Logger::Shutdown() {
    s_Logger.reset();
    spdlog::shutdown();
}

} // namespace nfsmw
