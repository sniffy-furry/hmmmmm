#pragma once
// ─── ui/RecentFiles.h ─────────────────────────────────────────────────────────
// Phase 5 — Recent-files list.
//
// Tracks the most recently opened BUN/BIN/vault files (per "kind") and
// persists them to a small JSON-ish file next to the app's log
// (nfsmwtoolkit_recent.txt — kept deliberately simple, no JSON dependency
// required) so the list survives restarts.
//
// Usage:
//   RecentFiles recents;
//   recents.Load();
//   recents.Add(RecentKind::Texture, path);
//   for (auto& e : recents.Entries(RecentKind::Texture)) { ... }
//   recents.Save();
// ─────────────────────────────────────────────────────────────────────────────
#include <deque>
#include <filesystem>
#include <string>
#include <vector>

namespace nfsmw {

enum class RecentKind {
    Texture,  ///< Opened via "Open BUN/BIN..." (TexturePackPanel)
    Export,   ///< Opened via "Open BUN/BIN for Object Export..."
    Audio,    ///< Opened via "Open Audio (SNR)..."
    ABK,      ///< Opened via "Open Sound Bank (ABK)..."
    GIN,      ///< Opened via "Open Engine Audio (GIN)..."
    MPF,      ///< Opened via "Open Music (MPF+MUS)..."
    Video,    ///< Opened via "Open Video (VP6)..."
    UIHud,    ///< Opened via "Open UI/HUD..." (FRONTEND/GLOBALHUD BUN, GLOBALB.LZC)
    Minimap,  ///< Opened via "Open Minimap (MINI_MAP.BIN)..."
};

class RecentFiles {
public:
    static constexpr size_t kMaxEntries = 10;

    /// Add (or move-to-front) a path for the given kind. De-duplicates.
    void Add(RecentKind kind, const std::filesystem::path& path);

    /// Most-recent-first list of paths for the given kind.
    const std::deque<std::filesystem::path>& Entries(RecentKind kind) const;

    /// Remove entries that no longer exist on disk.
    void PruneMissing();

    /// Remove all entries for the given kind.
    void Clear(RecentKind kind);

    /// Load from disk (call once at startup). Missing file is not an error.
    void Load();

    /// Persist to disk. Safe to call frequently (small file).
    void Save() const;

private:
    std::deque<std::filesystem::path> texture_;
    std::deque<std::filesystem::path> export_;
    std::deque<std::filesystem::path> audio_;
    std::deque<std::filesystem::path> abk_;
    std::deque<std::filesystem::path> gin_;
    std::deque<std::filesystem::path> mpf_;
    std::deque<std::filesystem::path> video_;
    std::deque<std::filesystem::path> uihud_;
    std::deque<std::filesystem::path> minimap_;

    std::deque<std::filesystem::path>& ListFor(RecentKind kind);
    const std::deque<std::filesystem::path>& ListFor(RecentKind kind) const;

    static std::filesystem::path StorePath();
};

} // namespace nfsmw
