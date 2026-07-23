#include "formats/EmitterCatalog.h"
#include <array>

// Effect/emitter names verified against the retail attributes.bin 'ErtS' table
// (MWEncyclopedia C19.6/C19.7/C19.8). These are the ambient / attachable classes
// a world object can carry. Nothing here is speculative — names are real classes.

namespace nfsmw {

const char* FxFamilyLabel(FxFamily f) {
    switch (f) {
        case FxFamily::Birds:      return "Birds";
        case FxFamily::Leaves:     return "Leaves";
        case FxFamily::Fog:        return "Fog & haze";
        case FxFamily::Water:      return "Water & fountains";
        case FxFamily::SmokeSteam: return "Smoke & steam";
        case FxFamily::Dust:       return "Dust";
        case FxFamily::Lights:     return "Lights";
        case FxFamily::Sparks:     return "Sparks";
        case FxFamily::Fire:       return "Fire";
        case FxFamily::Car:        return "Car FX";
        case FxFamily::Terrain:    return "Terrain";
        case FxFamily::Explosion:  return "Explosions";
        case FxFamily::Other:      return "Other";
    }
    return "Other";
}

const std::vector<EmitterDef>& EmitterCatalog() {
    using F = FxFamily;
    static const std::vector<EmitterDef> kCatalog = {
        // ── Birds ────────────────────────────────────────────────────────────
        { "fxenv_bird",          "emenv_bird01",      F::Birds, "Single light/grey bird taking off" },
        { "fxenv_birdblack01",   "emenv_birdblack01", F::Birds, "Black bird" },
        { "fxenv_blackbird02",   "emenv_blackbird02", F::Birds, "Black bird (variant)" },
        { "env_birdflock1",      "",                  F::Birds, "Flock of birds (multiple)" },
        { "env_birdflock2",      "",                  F::Birds, "Flock of birds (variant)" },
        { "xe_birdspraya_rb",    "",                  F::Birds, "Birds scatter when disturbed" },

        // ── Leaves ───────────────────────────────────────────────────────────
        { "fxenv_leaves1",       "emenv_leaves1",     F::Leaves, "Ambient drifting leaves" },
        { "fxenv_leaffall_hvy",  "",                  F::Leaves, "Heavy falling-leaf shower" },
        { "env_leaves1",         "",                  F::Leaves, "Leaf placement (ambient)" },
        { "env_leafblast1",      "",                  F::Leaves, "Leaf kick-up burst" },
        { "enmisc_leaves1",      "",                  F::Leaves, "Leaf burst (misc)" },

        // ── Fog & haze ───────────────────────────────────────────────────────
        { "fxenv_fog1",          "emenv_fog1",        F::Fog, "Ground fog / haze" },
        { "fxenv_fog1thick",     "",                  F::Fog, "Thick ground fog" },
        { "fxenv_fog2",          "emenv_fog2",        F::Fog, "Ground fog (variant)" },
        { "roadfog",             "",                  F::Fog, "Road-level fog volume" },
        { "fogbig",              "",                  F::Fog, "Large fog volume" },

        // ── Water & fountains ────────────────────────────────────────────────
        { "fxenv_fountain1",     "emwtr_fountaincore1", F::Water, "Fountain spray (jet + mist + top)" },
        { "fxenv_fountain2",     "",                  F::Water, "Fountain spray (variant)" },
        { "fxenv_fountain3",     "",                  F::Water, "Fountain spray (variant)" },
        { "fxenv_ripple1",       "emenv_ripple1",     F::Water, "Water-surface ripples" },
        { "fxenv_ripple2",       "emenv_ripple2",     F::Water, "Water-surface ripples (variant)" },
        { "fxwtr_fountain1",     "",                  F::Water, "Fountain spray (water bank)" },

        // ── Smoke & steam ────────────────────────────────────────────────────
        { "fxenv_smokestack",    "emsmk_env_white",   F::SmokeSteam, "Factory chimney smoke plume" },
        { "fxenv_smokestack_blk","emsmk_env_black",   F::SmokeSteam, "Black chimney smoke" },
        { "fxenv_smokestack_brn","emsmk_env_brown",   F::SmokeSteam, "Brown chimney smoke" },
        { "fxenv_smokestack_long","emsmk_env_long",   F::SmokeSteam, "Tall chimney smoke column" },
        { "fxenv_chimney1",      "emsmk_env_chimney1",F::SmokeSteam, "House chimney smoke" },
        { "fxenv_small_steam1",  "emmis_small_steam1",F::SmokeSteam, "Small steam jet (vent/grate)" },
        { "fxenv_small_steam2",  "emmis_small_steam2",F::SmokeSteam, "Small steam jet (variant)" },
        { "fxenv_small_steam3",  "emmis_small_steam3",F::SmokeSteam, "Small steam jet (variant)" },

        // ── Dust ─────────────────────────────────────────────────────────────
        { "fxenv_dustmotes1",    "emenv_dustmotes1",  F::Dust, "Floating dust motes in light shafts" },
        { "fxcar_dusttrail1",    "",                  F::Dust, "Dust trail kicked up driving" },
        { "emmis_dustpuff",      "",                  F::Dust, "Dust puff (impact)" },

        // ── Lights ───────────────────────────────────────────────────────────
        { "fxcar_coplightblue",  "",                  F::Lights, "Police light bar (blue)" },
        { "fxcar_coplightred",   "",                  F::Lights, "Police light bar (red)" },
        { "light_flares_cg",     "",                  F::Lights, "Light flare (corona)" },
        { "streetlight_scale",   "",                  F::Lights, "Street-light glow scale" },

        // ── Sparks ───────────────────────────────────────────────────────────
        { "fxsprk_md_dir",       "",                  F::Sparks, "Directional spark burst (medium)" },
        { "fxmis_coins1",        "",                  F::Sparks, "Coin/metal spark scatter" },
        { "fxmis_glass1",        "",                  F::Sparks, "Glass shatter sparkle" },

        // ── Fire ─────────────────────────────────────────────────────────────
        { "fxfire_lg_area1",     "",                  F::Fire, "Large area fire" },
        { "fxfire_sm_area1",     "",                  F::Fire, "Small area fire" },

        // ── Car FX ───────────────────────────────────────────────────────────
        { "fxcar_nos",           "",                  F::Car, "Nitrous exhaust burst" },
        { "fxcar_backfire",      "",                  F::Car, "Exhaust backfire" },

        // ── Terrain ──────────────────────────────────────────────────────────
        { "fxtd_dr_dirt",        "",                  F::Terrain, "Dirt dust when driven on" },
        { "fxtd_dr_asphalt_leaves","",                F::Terrain, "Leafy asphalt scatter" },
        { "fxtd_dr_grass",       "",                  F::Terrain, "Grass clippings" },

        // ── Explosions ───────────────────────────────────────────────────────
        { "fxex_gasstation",     "",                  F::Explosion, "Gas-station blast" },
        { "gaspumpexplosion",    "",                  F::Explosion, "Gas-pump explosion" },
        { "explosions",          "",                  F::Explosion, "Generic explosion" },
    };
    return kCatalog;
}

const std::vector<FxFamily>& EmitterFamilies() {
    static const std::vector<FxFamily> kFams = {
        FxFamily::Birds, FxFamily::Leaves, FxFamily::Fog, FxFamily::Water,
        FxFamily::SmokeSteam, FxFamily::Dust, FxFamily::Lights, FxFamily::Sparks,
        FxFamily::Fire, FxFamily::Car, FxFamily::Terrain, FxFamily::Explosion,
    };
    return kFams;
}

std::vector<const EmitterDef*> EmittersInFamily(FxFamily f) {
    std::vector<const EmitterDef*> out;
    for (const auto& e : EmitterCatalog())
        if (e.family == f) out.push_back(&e);
    return out;
}

const EmitterDef* FindEmitter(std::string_view effect) {
    for (const auto& e : EmitterCatalog())
        if (effect == e.effect) return &e;
    return nullptr;
}

} // namespace nfsmw
