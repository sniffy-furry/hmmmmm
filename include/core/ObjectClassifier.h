#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// ObjectClassifier — name-based semantics for world SolidObjects.
//
// NFSMW does not store a "this is a tree / this is a breakable" bit that we have
// fully decoded (see FlagReference — most geometry flag bits are still raw). The
// engine instead keys behaviour off the object's MODEL NAME, resolved through one
// intern function at world-load (MWEncyclopedia C15/C25, confirmed live by
// MWDebugger 2026-07-02). The naming *convention* is therefore the authoritative,
// editable knowledge layer:
//
//   • foliage  — XT_*, *TREE*, *BUSH*, *PALM*, *FERN*, *HEDGE*, … sway in wind
//   • smackable/destructible props end their names with a sequence/fragment role:
//        …_seq          collapse sequence (the scripted "coming down" motion)
//        …support_seq   / …support_frag   the support structure that gives way
//        …_frag / …subfrag / …spawnsubfrag the debris pieces that scatter
//        …smack / testsmack / genericsmackable the generic breakables
//   • pursuit-breaker set-pieces are just very large smackables whose break is
//     tuned by a SmackableParams vault row (break threshold / fragment count /
//     impulse transfer) — donut shop, water tower, gas station, radio tower,
//     scaffold, sailboat, gazebo, billboard, crane, greenhouse, drive-in.
//
// This module is GL/VanGUI-free (nfsmw_core, C++20) so both the CLI and the GUI
// object viewer can classify an object from its name and present the smackable /
// sway / wind / set-piece semantics the map editor and encyclopedia describe.
// ─────────────────────────────────────────────────────────────────────────────

/// Where a destructible prop sits in the collapse anatomy, inferred from the
/// name suffix (see C25.5 Pursuit Breakers).
enum class SmackableRole : uint8_t {
    None,      ///< not a destructible prop
    Generic,   ///< a breakable, but no sequence/fragment role in the name
    Collapse,  ///< …_seq — the scripted collapse sequence
    Support,   ///< …support_seq / …support_frag — the support structure
    Fragment,  ///< …_frag / …subfrag / …spawnsubfrag — the debris pieces
};

const char* SmackableRoleName(SmackableRole r);

struct ObjectClassification {
    bool          swayable       = false;  ///< foliage that bends in the wind
    bool          smackable      = false;  ///< destructible / breakable prop
    SmackableRole role           = SmackableRole::None;
    bool          pursuitBreaker = false;  ///< recognised set-piece breaker
    const char*   setPiece       = nullptr;///< e.g. "Water tower" (nullptr if none)
    bool          panoramaLOD    = false;  ///< PAN_* / *_LOD* far stand-in
    bool          proxyGeometry  = false;  ///< RFL_* reflection / SHADOW_* decal
    bool          eventOnly      = false;  ///< CHEVRON / TRACKBARRIER / NIS prop

    /// True when the object carries no special world semantics (plain geometry).
    bool IsPlain() const {
        return !swayable && !smackable && !pursuitBreaker &&
               !panoramaLOD && !proxyGeometry && !eventOnly;
    }
};

/// Classify a SolidObject purely from its model name (case-insensitive).
ObjectClassification ClassifyObject(const std::string& name);

// ── SmackableParams vault reference ──────────────────────────────────────────
// The per-prop break behaviour is NOT in the geometry file — it is a
// SmackableParams row in the attribute vault (attributes.bin / VaultFile). We
// surface the schema here as reference so the object viewer can point the user at
// what is tunable and where. Field semantics are from MWEncyclopedia C25.6 /
// C7 (verified class reference; exact byte layout inferred).
struct SmackableParamRef { const char* field; const char* meaning; };

/// The documented SmackableParams fields, in schema order.
const std::vector<SmackableParamRef>& SmackableParamsReference();

} // namespace nfsmw
