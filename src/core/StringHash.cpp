#include "core/StringHash.h"
#include <cstdio>

namespace nfsmw {

HashResolver& HashResolver::Instance() {
    static HashResolver inst;
    return inst;
}

void HashResolver::Register(std::string_view name) {
    if (name.empty()) return;
    table_.emplace(JoaatHash(name), std::string(name));
}

void HashResolver::RegisterHash(uint32_t hash, std::string_view name) {
    if (name.empty()) return;
    table_.emplace(hash, std::string(name));
}

const std::string& HashResolver::Resolve(uint32_t hash) const {
    if (const std::string* s = TryResolve(hash)) return *s;
    // Issue #34: avoid heap-allocating/copying a std::string on every call
    // for unregistered hashes; format into a reusable thread-local buffer.
    static thread_local std::string buf;
    char tmp[16];
    std::snprintf(tmp, sizeof(tmp), "0x%08X", hash);
    buf = tmp;
    return buf;
}

const std::string* HashResolver::TryResolve(uint32_t hash) const {
    auto it = table_.find(hash);
    return it != table_.end() ? &it->second : nullptr;
}

} // namespace nfsmw
