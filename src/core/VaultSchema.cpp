#include "core/VaultSchema.h"
#include <array>

// ─────────────────────────────────────────────────────────────────────────────
// Data source: MWEncyclopedia RE-Data-And-Discoveries/data/vault_class_layouts.json
// (inverted from attributes_field_census.json + attributes_class_table.json).
// Every (field-hash → name) below is an EXACT lookup2/0xABCDEF00 hash match
// against strings mined from speed.exe + attributes.bin + gameplay.bin — not a
// guess. Only fields whose names resolved are listed; class field COUNTS may be
// larger (some names remain compile-stripped), but nothing here is speculative.
// ─────────────────────────────────────────────────────────────────────────────

namespace nfsmw {

const std::vector<VaultClassSchema>& VaultSchemaCatalog() {
    static const std::vector<VaultClassSchema> kCatalog = {
        {"Performance", "engine", 0xF1F5FBC7, {
            {0x82C55940, "ENGINE_BRAKING", "Float"},
            {0xCCBB0245, "FLYWHEEL_MASS", "Float"},
            {0x9319476A, "IDLE", "Float"},
            {0xC5561812, "MAX_RPM", "Float"},
            {0xE00B01BF, "RED_LINE", "Float"},
            {0xA2D9ECB4, "SPEED_LIMITER", "Float"},
            {0xAFCD67BC, "TORQUE", "Float"},
        }},
        {"Performance", "transmission", 0x07A7A3E5, {
            {0xCF5DD4E9, "CLUTCH_SLIP", "Float"},
            {0x30B5E627, "DIFFERENTIAL", "Float"},
            {0xDDB32B64, "FINAL_GEAR", "Float"},
            {0x5250EB92, "GEAR_EFFICIENCY", "Float"},
            {0x17144A84, "GEAR_RATIO", "Float"},
            {0x8DC16371, "OPTIMAL_SHIFT", "Float"},
            {0xDD806309, "SHIFT_SPEED", "Float"},
            {0x53C717EC, "TORQUE_CONVERTER", "Float"},
            {0x7381EE38, "TORQUE_SPLIT", "Float"},
        }},
        {"Performance", "brakes", 0x36350867, {
            {0x3A377301, "BRAKES", "AxlePair"},
            {0xF5BEC23D, "BRAKE_LOCK", "AxlePair"},
            {0xDC965A0C, "EBRAKE", "Float"},
        }},
        {"Performance", "chassis", 0xAFA210F0, {
            {0xA073DE78, "AERO_CG", "Float"},
            {0xC29B0F17, "AERO_COEFFICIENT", "Float"},
            {0x3C860745, "DRAG_COEFFICIENT", "Float"},
            {0xD8CBCC48, "FRONT_AXLE", "Float"},
            {0xE56DEE24, "FRONT_WEIGHT_BIAS", "Float"},
            {0xB3D3E7D5, "RENDER_MOTION", "Float"},
            {0x46C189B0, "RIDE_HEIGHT", "AxlePair"},
            {0x6702CFF9, "ROLL_CENTER", "Float"},
            {0x54536D38, "SHOCK_BLOWOUT", "Float"},
            {0xAA13AE7B, "SHOCK_DIGRESSION", "AxlePair"},
            {0x54680830, "SHOCK_EXT_STIFFNESS", "AxlePair"},
            {0x4BC16211, "SHOCK_STIFFNESS", "AxlePair"},
            {0x42C42B3F, "SHOCK_VALVING", "AxlePair"},
            {0xA0B49FC2, "SPRING_PROGRESSION", "AxlePair"},
            {0x802E05DF, "SPRING_STIFFNESS", "AxlePair"},
            {0x61CA75F2, "SWAYBAR_STIFFNESS", "AxlePair"},
            {0xA738AEDF, "TRACK_WIDTH", "AxlePair"},
            {0x7E01B96D, "TRAVEL", "AxlePair"},
            {0x2D1375A1, "WHEEL_BASE", "Float"},
        }},
        {"Performance", "tires", 0xBD38D1CA, {
            {0xDEFC0B82, "ASPECT_RATIO", "AxlePair"},
            {0xAAFC01B3, "DYNAMIC_GRIP", "AxlePair"},
            {0xF177BE1B, "GRIP_SCALE", "AxlePair"},
            {0xB698BE9F, "RIM_SIZE", "AxlePair"},
            {0x5161D2EE, "SECTION_WIDTH", "AxlePair"},
            {0xEC096166, "STATIC_GRIP", "AxlePair"},
            {0xFEF5CC35, "STEERING", "Float"},
            {0x64C43C4B, "YAW_CONTROL", "Float"},
            {0xC2094707, "YAW_SPEED", "Float"},
        }},
        {"Performance", "induction", 0xC92A0142, {
            {0x7168EF58, "HIGH_BOOST", "Float"},
            {0x7A3181C0, "LOW_BOOST", "Float"},
            {0xBA0A7D04, "PSI", "Float"},
            {0xCF11AF87, "SPOOL", "Float"},
            {0xEE2C326F, "VACUUM", "Float"},
        }},
        {"Performance", "nos", 0xB1669F64, {
            {0x3F3B1976, "FLOW_RATE", "Float"},
            {0xAA032196, "NOS_CAPACITY", "Float"},
            {0xAC733DE3, "NOS_DISENGAGE", "Float"},
            {0x12A14452, "RECHARGE_MAX", "Float"},
            {0x5799043B, "RECHARGE_MAX_SPEED", "Float"},
            {0x0A03C0EB, "RECHARGE_MIN", "Float"},
            {0x97A0D2DF, "RECHARGE_MIN_SPEED", "Float"},
            {0x8DE27C3C, "TORQUE_BOOST", "Float"},
        }},
        {"Pursuit", "pursuitlevels", 0x551E22B3, {
            {0xAC6E1EC7, "BackupCallTimer", "Float"},
            {0x769E8D9E, "BustSpeed", "Float"},
            {0xA00DE933, "CTSFor911", "Int32"},
            {0x594E1492, "CollapseAggression", "Float"},
            {0x1E0AF662, "CollapseInnerRadius", "Int32"},
            {0x947542F2, "CollapseOuterRadius", "Float"},
            {0xDB66950C, "CollapseSpeed", "Float"},
            {0x73FEA6DB, "FullEngagementCopCount", "Int32"},
            {0x01CEC2B4, "FullEngagementRadius", "Float"},
            {0xE766EB78, "HeliFuelTime", "Float"},
            {0x6E590F57, "NumCiviHitsFor911", "Int8"},
            {0xC467015C, "NumCopsToTriggerBackup", "Int32"},
            {0x3FA01F3D, "OverwritingLength", "Float"},
            {0x3F11FBFC, "RefspecTollgates", "Float"},
            {0x0F575B64, "SpeedReactionTime", "Float"},
            {0x7648C884, "StaggerFormationTime", "Float"},
            {0x9BF0F433, "TimeBetweenFirstFourSpawn", "Float"},
            {0xFF761484, "TuneCop2", "Float"},
            {0xA109BCCE, "evadetimeout", "Float"},
            {0xC146FC03, "harriers_cops", "Float"},
        }},
        {"Pursuit", "pursuitescalation", 0xD6D4330B, {
            {0xD4B0CC11, "heattable", "Attrib::RefSpec"},
            {0x2283ECAF, "racetable", "Attrib::RefSpec"},
            {0xE5332008, "supportracetable", "Attrib::RefSpec"},
            {0xF3918F68, "supporttable", "Attrib::RefSpec"},
        }},
        {"Pursuit", "aivehicle", 0x22515733, {
            {0xCC320329, "AccelerationMultiplier", "Float"},
            {0xEC57E16B, "TopSpeedMultiplier", "Float"},
        }},
        {"Pursuit", "damagespecs", 0xC1F0B434, {
            {0x3698B384, "FORCE", "Float"},
            {0x97F059B7, "HIT_POINTS", "Float"},
            {0x499CB7BD, "HP_THRESHOLD", "Float"},
            {0x907930DF, "SHOCK_FORCE", "Float"},
            {0x78059FF8, "SHOCK_TIME", "Float"},
        }},
        {"Effects", "emitterdata", 0xB30B18AF, {
            {0xCCF41B18, "Color1", "UInt32"},
            {0x4A282AF1, "Color2", "UInt32"},
            {0xC103B771, "Color3", "UInt32"},
            {0x78F32C40, "Color4", "UInt32"},
            {0x4E58FA4E, "Drag", "Float"},
            {0x66052349, "FarClip", "Float"},
            {0xAC5B265E, "Gravity", "Float"},
            {0x284A8C2C, "InitialAngleRange", "Float"},
            {0x81625B35, "Life", "Float"},
            {0xEFB4BB64, "LifeVariance", "Float"},
            {0x6BCBFC06, "MotionInherit", "Float"},
            {0x4D69EF9E, "MotionLive", "Int32"},
            {0xDC943CC9, "NumParticles", "Float"},
            {0xEB86A538, "OnCycle", "Float"},
            {0x31AF20D6, "RotationVariance", "Float"},
            {0x41862FE6, "Speed", "Float"},
            {0x58FCB1C3, "SpeedVariance", "Float"},
            {0xCAC30FF2, "SpreadAngle", "Float"},
            {0xEE67AD35, "StartDelay", "Float"},
        }},
        {"Effects", "emittergroup", 0xABA86E60, {
            {0x66052349, "FarClip", "Float"},
        }},
        {"Effects", "effects", 0xEBCEE74C, {
            {0x0099CB26, "InheritVelocity", "Float"},
            {0x110882D5, "FogEnable", "Bool"},
        }},
        {"Effects", "emitteruv", 0xE4983A7D, {
            {0x841D0551, "EndU", "Float"},
            {0x0D28675B, "EndV", "Float"},
            {0x98B1DA40, "StartU", "Float"},
            {0xB1EBD0D4, "StartV", "Float"},
        }},
    };
    return kCatalog;
}

std::vector<const VaultClassSchema*> VaultClassesInGroup(std::string_view group) {
    std::vector<const VaultClassSchema*> out;
    for (const auto& c : VaultSchemaCatalog())
        if (group == c.group) out.push_back(&c);
    return out;
}

const VaultClassSchema* FindVaultClass(std::string_view className) {
    for (const auto& c : VaultSchemaCatalog())
        if (className == c.className) return &c;
    return nullptr;
}

const char* ResolveVaultFieldName(uint32_t fieldHash) {
    for (const auto& c : VaultSchemaCatalog())
        if (const VaultField* f = c.FindField(fieldHash)) return f->name;
    return nullptr;
}

const VaultField* FindVaultField(uint32_t fieldHash) {
    for (const auto& c : VaultSchemaCatalog())
        if (const VaultField* f = c.FindField(fieldHash)) return f;
    return nullptr;
}

bool IsScalarVaultType(std::string_view type) {
    static constexpr std::array<std::string_view, 8> kScalars = {
        "Float", "Int32", "UInt32", "Int8", "UInt8", "Int16", "UInt16", "Bool",
    };
    for (auto s : kScalars) if (s == type) return true;
    return false;
}

} // namespace nfsmw
