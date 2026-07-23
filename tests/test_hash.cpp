// Hash algorithm regression vectors (Joaat / Joaat-CI / Bin).
// Values computed from the reference algorithms; if these change, a hashing
// regression has been introduced and every name-resolution path is at risk.
#include "microtest.h"
#include "core/StringHash.h"

using namespace nfsmw;

TEST(joaat_known_vectors) {
    CHECK_EQ(JoaatHash("car"),          0x69697274u);
    CHECK_EQ(JoaatHash("carsurface"),   0xBE47B9ECu);
    CHECK_EQ(JoaatHash("EngineRacer"),  0x3620343Eu);
    CHECK_EQ(JoaatHash("fxcar_nos"),    0x515F0730u);
    CHECK_EQ(JoaatHash("AICopManager"), 0x4AC5F3E4u);
    CHECK_EQ(JoaatHash(""),             0x00000000u);
}

TEST(joaat_ci_lowercases) {
    // Already-lowercase keys hash identically under CI; mixed-case differ from
    // the case-sensitive form but match their lowercased counterpart.
    CHECK_EQ(JoaatHashCI("car"),          JoaatHash("car"));
    CHECK_EQ(JoaatHashCI("EngineRacer"),  0xE7ED059Du);
    CHECK_EQ(JoaatHashCI("AICopManager"), 0xB22E846Eu);
    CHECK_EQ(JoaatHashCI("EngineRacer"),  JoaatHash("engineracer"));
}

TEST(bin_hash_known_vectors) {
    CHECK_EQ(BinHash("car"),          0x000125B5u);
    CHECK_EQ(BinHash("carsurface"),   0x10CE663Eu);
    CHECK_EQ(BinHash("EngineRacer"),  0x47C78022u);
    CHECK_EQ(BinHash(""),             0xFFFFFFFFu);
}

TEST(hash_resolver_roundtrip) {
    auto& r = HashResolver::Instance();
    r.Register("EngineRacer");
    // A registered name resolves back to its text; an unknown hash yields a hex placeholder.
    CHECK(r.Resolve(JoaatHash("EngineRacer")) == "EngineRacer");
    CHECK(r.Resolve(0x12345678u).rfind("0x", 0) == 0); // starts with "0x"
}
