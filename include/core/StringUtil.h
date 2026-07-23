#pragma once

// StringUtil.h — shared string helpers used across panels.
//
// History:
//   • ContainsCI()      — consolidated from 3× duplicate call sites (issue #20).
//   • ToLowerAscii()    — extracted from 5+ inline std::transform blocks
//                         in TexturePackPanel, VaultPanel, AppShell (audit 4.1).
//   • ContainsCIStr()   — string_view overload used by FilterBox::Matches
//                         and PassesFilter replacements (audit 4.1).

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>

namespace nfsmw {

/// Return a copy of `s` with every ASCII letter lowercased.
inline std::string ToLowerAscii(std::string s) {
    for (auto& c : s)
        c = (char)std::tolower((unsigned char)c);
    return s;
}

/// Case-insensitive substring search (string_view overload).
/// An empty `needle` matches anything.
inline bool ContainsCIStr(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) return true;
    auto it = std::search(haystack.begin(), haystack.end(),
                          needle.begin(),   needle.end(),
                          [](char a, char b) {
                              return std::tolower((unsigned char)a) ==
                                     std::tolower((unsigned char)b);
                          });
    return it != haystack.end();
}

/// Case-insensitive substring search (legacy const char* overload — kept for
/// existing callers; prefer ContainsCIStr for new code).
/// An empty/null `needle` matches anything.
inline bool ContainsCI(const std::string& haystack, const char* needle) {
    if (!needle || !needle[0]) return true;
    return ContainsCIStr(haystack, needle);
}

} // namespace nfsmw
