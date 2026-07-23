// ─── tests/microtest.h ────────────────────────────────────────────────────────
// A tiny, dependency-free unit-test harness (stb-style single header).
//
// Usage:
//   // in exactly ONE translation unit:
//   #define MICROTEST_MAIN
//   #include "microtest.h"
//
//   // in any test file:
//   #include "microtest.h"
//   TEST(joaat_known_vectors) {
//       CHECK_EQ(nfsmw::JoaatHash("car"), 0x69697274u);
//       CHECK(some_condition);
//   }
//
// Build all test .cpp into one executable; running it returns the number of
// failed checks (0 = success), so CTest treats non-zero as failure.
#pragma once
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace microtest {

struct Case { const char* name; std::function<void()> fn; };

inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
inline int&  fail_count()  { static int n = 0; return n; }
inline int&  check_count() { static int n = 0; return n; }
inline const char*& current() { static const char* c = "<none>"; return c; }

struct Registrar { Registrar(const char* n, std::function<void()> f) { registry().push_back({n, std::move(f)}); } };

inline void report_fail(const char* expr, const char* file, int line) {
    ++fail_count();
    std::printf("  [FAIL] %s\n      %s:%d:  %s\n", current(), file, line, expr);
}
template <class A, class B>
inline void report_fail_eq(const char* ea, const char* eb, A a, B b, const char* file, int line) {
    ++fail_count();
    std::printf("  [FAIL] %s\n      %s:%d:  %s == %s\n      lhs = %lld (0x%llX)\n      rhs = %lld (0x%llX)\n",
                current(), file, line, ea, eb,
                (long long)a, (unsigned long long)a, (long long)b, (unsigned long long)b);
}

inline int run_all() {
    std::printf("microtest: %zu test case(s)\n", registry().size());
    for (auto& c : registry()) {
        current() = c.name;
        int before = fail_count();
        c.fn();
        if (fail_count() == before) std::printf("  [ ok ] %s\n", c.name);
    }
    std::printf("microtest: %d check(s), %d failure(s)\n", check_count(), fail_count());
    return fail_count();
}

} // namespace microtest

#define MT_CAT2(a,b) a##b
#define MT_CAT(a,b)  MT_CAT2(a,b)
#define TEST(name)                                                            \
    static void MT_CAT(mt_fn_, name)();                                       \
    static ::microtest::Registrar MT_CAT(mt_reg_, name)(#name, MT_CAT(mt_fn_, name)); \
    static void MT_CAT(mt_fn_, name)()

#define CHECK(expr) do { ++::microtest::check_count();                        \
    if (!(expr)) ::microtest::report_fail(#expr, __FILE__, __LINE__); } while (0)

#define CHECK_EQ(a, b) do { ++::microtest::check_count();                     \
    auto _va = (a); auto _vb = (b);                                           \
    if (!(_va == _vb)) ::microtest::report_fail_eq(#a, #b, _va, _vb, __FILE__, __LINE__); } while (0)

#ifdef MICROTEST_MAIN
int main() { return ::microtest::run_all(); }
#endif
