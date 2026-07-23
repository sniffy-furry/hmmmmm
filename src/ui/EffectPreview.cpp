#include "ui/EffectPreview.h"
#include <cmath>
#include <cctype>
#include <cstdint>

namespace nfsmw {

namespace {

// Deterministic per-particle pseudo-random in [0,1).
float PHash(int i, int salt) {
    uint32_t h = (uint32_t)i * 374761393u + (uint32_t)salt * 668265263u + 1u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 0xFFFFFFu) / (float)0x1000000;
}

} // namespace

void EmitParticles(VanDrawList* dl, float sx, float sy, FxFamily fam,
                   const std::string& effect, float t, float sc) {
    // Lights: a pulsing coloured glow (blue/red for cop bars), no motion.
    if (fam == FxFamily::Lights) {
        int r = 255, g = 200, b = 60;
        std::string e = effect;
        for (char& c : e) c = (char)std::tolower((unsigned char)c);
        if (e.find("blue") != std::string::npos) { r = 60; g = 120; b = 255; }
        else if (e.find("red") != std::string::npos) { r = 255; g = 50; b = 50; }
        const float pulse = 0.5f + 0.5f * std::sin(t * 6.0f);
        for (int k = 3; k >= 1; --k)
            dl->AddCircleFilled(VanVec2(sx, sy), (float)(3 + k * 3) * sc,
                                VAN_COL32(r, g, b, (int)(pulse * 55.f * (float)k / 3.f)));
        return;
    }

    struct P { int count; float life, vx, vy, spread, grav, s0, s1;
               int r0,g0,b0,a0, r1,g1,b1,a1; };
    P p;
    switch (fam) {
        case FxFamily::SmokeSteam: p = {10, 2.6f, 0,-22, 10,  0, 3,10, 205,205,210,150, 120,120,125,0}; break;
        case FxFamily::Fire:       p = {14, 0.9f, 0,-34, 14,  0, 6, 1, 255,190,50,225, 210,40,0,0};    break;
        case FxFamily::Sparks:     p = {18, 0.5f, 0,-10,360, 90, 2, 1, 255,235,130,255, 255,110,0,0};  break;
        case FxFamily::Fog:        p = { 6, 5.0f, 0, -3, 22,  0,14,22, 185,190,200,55,  185,190,200,0}; break;
        case FxFamily::Water:      p = {16, 1.2f, 0,-42, 26, 80, 3, 2, 150,200,255,200, 150,200,255,0}; break;
        case FxFamily::Dust:       p = { 8, 3.0f, 0, -6, 20,  0, 4, 9, 205,185,145,120, 205,185,145,0}; break;
        case FxFamily::Leaves:     p = { 8, 3.4f, 0, 15, 32,  0, 3, 3, 130,165,75,210,  150,110,50,0};  break;
        case FxFamily::Birds:      p = { 6, 4.0f,34, -8, 10,  0, 2, 2, 40,40,45,225,    40,40,45,120};  break;
        case FxFamily::Terrain:    p = { 8, 1.0f, 0, -5, 40,  0, 3, 7, 200,180,140,150, 200,180,140,0}; break;
        case FxFamily::Explosion:  p = {14, 0.7f, 0,  0,360,  0, 4, 2, 255,170,60,240,  120,30,0,0};    break;
        case FxFamily::Car:        p = { 8, 0.6f, 0,  8, 18,  0, 4, 2, 255,150,40,220,  200,40,0,0};    break;
        default:                   p = { 8, 1.5f, 0,-12, 20,  0, 3, 5, 200,200,200,150, 200,200,200,0}; break;
    }

    if (fam == FxFamily::Explosion) {   // expanding shock ring under the sparks
        const float ph = std::fmod(t, p.life) / p.life;
        dl->AddCircle(VanVec2(sx, sy), ph * 46.f * sc,
                      VAN_COL32(255, 150, 50, (int)((1.f - ph) * 210.f)), 0, 2.5f);
    }

    const bool radial = (p.spread >= 360.f);
    const bool sway   = (fam == FxFamily::Leaves || fam == FxFamily::Birds);
    for (int i = 0; i < p.count; ++i) {
        const float seed  = PHash(i, 1), seed2 = PHash(i, 2);
        const float ph    = std::fmod(t / p.life + seed, 1.0f);
        const float age   = ph * p.life;
        // All spatial quantities scale by `sc` (world→screen) so the whole
        // effect tracks camera zoom and reads at its true size.
        float px, py;
        if (radial) {
            const float ang = seed2 * 6.2832f, spd = 40.f + seed * 40.f;
            px = sx + sc * (std::cos(ang) * spd * age);
            py = sy + sc * (std::sin(ang) * spd * age + 0.5f * p.grav * age * age);
        } else {
            px = sx + sc * ((seed2 - 0.5f) * p.spread * (age / p.life) + p.vx * age
                 + (sway ? std::sin(t * 2.f + seed * 6.2832f) * 12.f : 0.f));
            py = sy + sc * (p.vy * age + 0.5f * p.grav * age * age);
        }
        const float f    = radial ? (1.f - ph) : 1.f;
        const float size = (p.s0 + (p.s1 - p.s0) * ph) * sc;
        const int r = (int)(p.r0 + (p.r1 - p.r0) * ph);
        const int g = (int)(p.g0 + (p.g1 - p.g0) * ph);
        const int b = (int)(p.b0 + (p.b1 - p.b0) * ph);
        const int a = (int)((p.a0 + (p.a1 - p.a0) * ph) * f);
        dl->AddCircleFilled(VanVec2(px, py), size > 0.5f ? size : 0.5f, VAN_COL32(r, g, b, a));
    }
}

} // namespace nfsmw
