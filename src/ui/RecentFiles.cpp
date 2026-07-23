#include "ui/RecentFiles.h"
#include "core/Logger.h"

#include <fstream>
#include <algorithm>

namespace nfsmw {

std::deque<std::filesystem::path>& RecentFiles::ListFor(RecentKind kind) {
    switch (kind) {
        case RecentKind::Texture: return texture_;
        case RecentKind::Export:  return export_;
        case RecentKind::Audio:   return audio_;
        case RecentKind::ABK:     return abk_;
        case RecentKind::GIN:     return gin_;
        case RecentKind::MPF:     return mpf_;
        case RecentKind::Video:   return video_;
        case RecentKind::UIHud:   return uihud_;
        case RecentKind::Minimap: return minimap_;
    }
    return texture_; // unreachable
}

const std::deque<std::filesystem::path>& RecentFiles::ListFor(RecentKind kind) const {
    switch (kind) {
        case RecentKind::Texture: return texture_;
        case RecentKind::Export:  return export_;
        case RecentKind::Audio:   return audio_;
        case RecentKind::ABK:     return abk_;
        case RecentKind::GIN:     return gin_;
        case RecentKind::MPF:     return mpf_;
        case RecentKind::Video:   return video_;
        case RecentKind::UIHud:   return uihud_;
        case RecentKind::Minimap: return minimap_;
    }
    return texture_; // unreachable
}

void RecentFiles::Add(RecentKind kind, const std::filesystem::path& path) {
    auto& list = ListFor(kind);

    const auto abs = std::filesystem::weakly_canonical(path);

    list.erase(std::remove_if(list.begin(), list.end(), [&](const std::filesystem::path& p) {
        return std::filesystem::weakly_canonical(p) == abs;
    }), list.end());

    list.push_front(abs);

    while (list.size() > kMaxEntries)
        list.pop_back();
}

const std::deque<std::filesystem::path>& RecentFiles::Entries(RecentKind kind) const {
    return ListFor(kind);
}

void RecentFiles::PruneMissing() {
    for (auto* list : { &texture_, &export_, &audio_, &abk_, &gin_, &mpf_, &video_, &uihud_, &minimap_ }) {
        list->erase(std::remove_if(list->begin(), list->end(), [](const std::filesystem::path& p) {
            return !std::filesystem::exists(p);
        }), list->end());
    }
}

void RecentFiles::Clear(RecentKind kind) {
    ListFor(kind).clear();
}

std::filesystem::path RecentFiles::StorePath() {
    // Sit next to the log file in the working directory — consistent with
    // Logger::Init("nfsmwtoolkit.log").
    return std::filesystem::path("nfsmwtoolkit_recent.txt");
}

// Simple line-based format:
//   [Texture] <absolute path>
//   [Export]  <absolute path>
//   [Audio]   <absolute path>
// One entry per line, most-recent-first within each kind.
void RecentFiles::Load() {
    std::ifstream in(StorePath());
    if (!in)
        return;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty())
            continue;

        RecentKind kind;
        std::string rest;
        if (line.rfind("[Texture] ", 0) == 0) {
            kind = RecentKind::Texture;
            rest = line.substr(10);
        } else if (line.rfind("[Export] ", 0) == 0) {
            kind = RecentKind::Export;
            rest = line.substr(9);
        } else if (line.rfind("[Audio] ", 0) == 0) {
            kind = RecentKind::Audio;
            rest = line.substr(8);
        } else if (line.rfind("[ABK] ", 0) == 0) {
            kind = RecentKind::ABK;
            rest = line.substr(6);
        } else if (line.rfind("[GIN] ", 0) == 0) {
            kind = RecentKind::GIN;
            rest = line.substr(6);
        } else if (line.rfind("[MPF] ", 0) == 0) {
            kind = RecentKind::MPF;
            rest = line.substr(6);
        } else if (line.rfind("[Video] ", 0) == 0) {
            kind = RecentKind::Video;
            rest = line.substr(8);
        } else if (line.rfind("[UIHud] ", 0) == 0) {
            kind = RecentKind::UIHud;
            rest = line.substr(8);
        } else if (line.rfind("[Minimap] ", 0) == 0) {
            kind = RecentKind::Minimap;
            rest = line.substr(10);
        } else {
            LOG_WARN("RecentFiles::Load: unrecognised line (format drift?): '{}'", line);
            continue;
        }

        if (!rest.empty())
            ListFor(kind).push_back(std::filesystem::path(rest));
    }

    for (auto* list : { &texture_, &export_, &audio_, &abk_, &gin_, &mpf_, &video_, &uihud_, &minimap_ }) {
        while (list->size() > kMaxEntries)
            list->pop_back();
    }
}

void RecentFiles::Save() const {
    std::ofstream out(StorePath(), std::ios::trunc);
    if (!out)
        return;

    for (const auto& p : texture_) out << "[Texture] " << p.string() << "\n";
    for (const auto& p : export_)  out << "[Export] "  << p.string() << "\n";
    for (const auto& p : audio_)   out << "[Audio] "   << p.string() << "\n";
    for (const auto& p : abk_)     out << "[ABK] "     << p.string() << "\n";
    for (const auto& p : gin_)     out << "[GIN] "     << p.string() << "\n";
    for (const auto& p : mpf_)     out << "[MPF] "     << p.string() << "\n";
    for (const auto& p : video_)   out << "[Video] "   << p.string() << "\n";
    for (const auto& p : uihud_)   out << "[UIHud] "   << p.string() << "\n";
    for (const auto& p : minimap_) out << "[Minimap] " << p.string() << "\n";
}

} // namespace nfsmw
