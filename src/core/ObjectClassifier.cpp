#include "core/ObjectClassifier.h"
#include <array>
#include <cctype>

namespace nfsmw {

namespace {

std::string Upper(const std::string& s) {
    std::string u;
    u.reserve(s.size());
    for (char c : s) u.push_back(static_cast<char>(std::toupper((unsigned char)c)));
    return u;
}

bool Contains(const std::string& hay, const char* needle) {
    return hay.find(needle) != std::string::npos;
}

bool EndsWith(const std::string& s, const char* suffix) {
    const std::string suf = suffix;
    return s.size() >= suf.size() &&
           s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

// Foliage classes that bend in the wind (ported from NFSMapEditor
// MainWindow::IsSwayableName — the PLAY-mode sway list).
bool IsSwayable(const std::string& u) {
    return u.rfind("XT_", 0) == 0 ||                 // tree/foliage class prefix
           Contains(u, "TREE")  || Contains(u, "BUSH")   ||
           Contains(u, "FOLIAGE")|| Contains(u, "LEAF")  ||
           Contains(u, "HEDGE") || Contains(u, "PALM")   ||
           Contains(u, "FERN")  || Contains(u, "SHRUB")  ||
           Contains(u, "PLANT") || Contains(u, "BRANCH");
}

// Far-distance panorama / LOD stand-in meshes (ported from SceneObject.h).
bool IsPanoramaLOD(const std::string& u) {
    return u.rfind("PAN_", 0) == 0 || Contains(u, "_LOD");
}

// Reflection / baked-shadow proxy geometry the game does not draw in the main
// view (ported from SceneObject.h::IsProxyGeometryName).
bool IsProxy(const std::string& u) {
    return u.rfind("RFL_", 0) == 0 || u.rfind("SHADOW", 0) == 0;
}

// Race / cutscene-only props (ported from SceneObject.h::IsEventOnlyModelName).
bool IsEventOnly(const std::string& u) {
    return Contains(u, "CHEVRON") || Contains(u, "TRACKBARRIER") ||
           u.rfind("NIS_", 0) == 0 || Contains(u, "_NIS_") || Contains(u, "XO_NIS");
}

// Roots that mark a destructible/breakable prop even without a _seq/_frag suffix
// (from the retail ErtS/scenery catalogue, MWEncyclopedia C25.5).
constexpr std::array<const char*, 10> kSmackRoots = {
    "SMACK", "SUPPORT", "SUBFRAG", "_FRAG", "BREAKABLE",
    "SPLODE", "SHATTER", "FRAGMENT", "DESTRUCT", "WRECK",
};

// Pursuit-breaker set-pieces: substring → display name (MWEncyclopedia C25.5).
struct SetPiece { const char* key; const char* name; };
constexpr std::array<SetPiece, 12> kSetPieces = {{
    { "DONUT",      "Donut shop"   },
    { "WATERTOWER", "Water tower"  },
    { "GASSTATION", "Gas station"  },
    { "GASPUMP",    "Gas station"  },
    { "RADIOTOWER", "Radio tower"  },
    { "SCAFFOLD",   "Scaffolding"  },
    { "SAILBOAT",   "Sailboat"     },
    { "GAZEBO",     "Gazebo"       },
    { "BILLBOARD",  "Billboard"    },
    { "CRANESUPPORT","Crane"       },
    { "GREENHOUSE", "Greenhouse"   },
    { "DRIVEIN",    "Drive-in"     },
}};

} // namespace

const char* SmackableRoleName(SmackableRole r) {
    switch (r) {
        case SmackableRole::None:     return "none";
        case SmackableRole::Generic:  return "generic breakable";
        case SmackableRole::Collapse: return "collapse sequence (_seq)";
        case SmackableRole::Support:  return "support structure";
        case SmackableRole::Fragment: return "debris fragment";
    }
    return "none";
}

ObjectClassification ClassifyObject(const std::string& name) {
    ObjectClassification c;
    if (name.empty()) return c;
    const std::string u = Upper(name);

    c.swayable      = IsSwayable(u);
    c.panoramaLOD   = IsPanoramaLOD(u);
    c.proxyGeometry = IsProxy(u);
    c.eventOnly     = IsEventOnly(u);

    // Smackable detection: any destructible root token in the name.
    for (const char* root : kSmackRoots)
        if (Contains(u, root)) { c.smackable = true; break; }

    // Collapse/fragment naming also implies a smackable even without a root token.
    const bool isSeq  = EndsWith(u, "_SEQ") || Contains(u, "_SEQ");
    const bool isFrag = Contains(u, "SUBFRAG") || Contains(u, "_FRAG") ||
                        EndsWith(u, "FRAG");
    const bool isSupp = Contains(u, "SUPPORT");
    if (isSeq || isFrag || isSupp) c.smackable = true;

    if (c.smackable) {
        if (isSupp)       c.role = SmackableRole::Support;
        else if (isFrag)  c.role = SmackableRole::Fragment;
        else if (isSeq)   c.role = SmackableRole::Collapse;
        else              c.role = SmackableRole::Generic;
    }

    // Pursuit-breaker set-piece recognition (only meaningful for smackables).
    for (const auto& sp : kSetPieces)
        if (Contains(u, sp.key)) {
            c.pursuitBreaker = true;
            c.setPiece       = sp.name;
            c.smackable      = true;
            if (c.role == SmackableRole::None) c.role = SmackableRole::Generic;
            break;
        }

    return c;
}

const std::vector<SmackableParamRef>& SmackableParamsReference() {
    // Schema order per MWEncyclopedia C25.6 / C7-Attribute-Vaults. Values live in
    // the attribute vault (attributes.bin), not the geometry BUN.
    static const std::vector<SmackableParamRef> kRef = {
        { "break_threshold",
          "Impulse (mass x closing-speed) needed to break the prop. Low = breaks "
          "at walking pace (cone/fence); high = needs real speed (set-piece)." },
        { "fragment_count",
          "How many debris pieces the prop sheds when it breaks." },
        { "impulse_transfer",
          "How much of the hit's impulse is passed into the debris / bodies in "
          "range (the shove the collapse gives nearby cars)." },
    };
    return kRef;
}

} // namespace nfsmw
