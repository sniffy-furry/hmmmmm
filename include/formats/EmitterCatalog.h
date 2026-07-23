#pragma once
#include <string>
#include <string_view>
#include <vector>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// EmitterCatalog — the game's ambient/effect emitters, grouped for authoring.
//
// NFSMW effects are `fx…` classes assembled from `em…` emitters, filed into FX
// banks by family (environment, car, terrain, gameplay, collision …). This is a
// curated catalogue of the ones you attach to world objects — birds, leaves,
// fog, fountains, steam & smoke, dust, cop lights, sparks, fire, NOS, and more —
// recovered from the retail ErtS table (see MWEncyclopedia C19). GL/VanGUI-free.
// ─────────────────────────────────────────────────────────────────────────────

enum class FxFamily : uint8_t {
    Birds, Leaves, Fog, Water, SmokeSteam, Dust,
    Lights, Sparks, Fire, Car, Terrain, Explosion, Other,
};

const char* FxFamilyLabel(FxFamily f);

/// One catalogue entry: an effect (`fx…`) and the emitter (`em…`) that drives it.
struct EmitterDef {
    const char* effect;   ///< fx class name (e.g. "fxenv_bird")
    const char* emitter;  ///< emitter name  (e.g. "emenv_bird01") — may be ""
    FxFamily    family;
    const char* desc;     ///< human description
};

/// The whole catalogue, in family order.
const std::vector<EmitterDef>& EmitterCatalog();

/// All families that have at least one entry, in display order.
const std::vector<FxFamily>& EmitterFamilies();

/// Entries in one family.
std::vector<const EmitterDef*> EmittersInFamily(FxFamily f);

/// Look up a catalogue entry by effect name (nullptr if not catalogued).
const EmitterDef* FindEmitter(std::string_view effect);

} // namespace nfsmw
