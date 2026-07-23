#include "core/KnownNames.h"
#include "core/StringHash.h"
#include <array>
#include <cstddef>
#include <string_view>

namespace nfsmw {

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Source: GLOBAL/attributes.bin, the master VPAK attribute vault's 'ErtS'
// string table (confirmed via RIVALS_AND_VAULTS_Audit.txt / att_dump.txt —
// a verified hex-editor string scan of the retail file, not a guess). The
// 'VPAK' and 'ErtS' chunk-format magic strings themselves are excluded; only
// the gameplay/asset names that follow them are listed here.
// ─────────────────────────────────────────────────────────────────────────────

// Physics / surface / material classes — referenced by terrain, collision,
// and tire-sound systems (carsurface / terraindriving lookups) (20 names)
constexpr std::array<std::string_view, 20> kSurfaceClasses = {
    "carsurface", "concrete", "debris", "default", "destruction", "dust", "environmental",
    "fire", "gameplay", "glass", "metal", "plastic", "smoke", "solid", "sparks",
    "terraindriving", "water", "wood", "xeci_car", "xecs_solid_wall",
};

// Particle / VFX system names — fxcar_*/fxenv_*/fxtd_*/fxnis_*/fxgame_*/
// fxmis_*/fxsmk_*/fxsprk_*/fxex_*/fxwtr_*/fxdust_*/fxfire_* effect emitters (109 names)
constexpr std::array<std::string_view, 109> kParticleEffects = {
    "fxcar_cop_damage1", "fxcar_cop_death1", "fxcar_coplightblue", "fxcar_coplightred",
    "fxcar_coplightwhite", "fxcar_dusttrail1", "fxcar_engineblow1", "fxcar_exhaust_bmw",
    "fxcar_exhaust_bmw2", "fxcar_exhaust_drip", "fxcar_impactdebrisl", "fxcar_impactl",
    "fxcar_impactpavement", "fxcar_nos", "fxcar_tireblow", "fxcs_sc_metal",
    "fxdust_lg_billow1", "fxdust_lg_fall", "fxdust_med_billow1", "fxdust_med_exup",
    "fxdust_med_fall", "fxenv_bird", "fxenv_birdblack01", "fxenv_blackbird02",
    "fxenv_chimney1", "fxenv_dustmotes1", "fxenv_fog1", "fxenv_fog1thick", "fxenv_fog2",
    "fxenv_fog_fe1", "fxenv_fog_fe2", "fxenv_fountain1", "fxenv_fountain2", "fxenv_fountain3",
    "fxenv_leaffall_hvy", "fxenv_leaves1", "fxenv_ripple1", "fxenv_ripple2",
    "fxenv_small_steam1", "fxenv_small_steam2", "fxenv_small_steam3", "fxenv_smokestack",
    "fxenv_smokestack_blk", "fxenv_smokestack_brn", "fxenv_smokestack_long",
    "fxex_carexplode_sm1", "fxex_gasstation", "fxex_large1", "fxex_large2", "fxfire_lg_area1",
    "fxfire_sm1", "fxfire_trail1", "fxgame_flare_green", "fxgame_flare_red",
    "fxgame_icongrp_chop", "fxgame_icongrp_circuit", "fxgame_icongrp_drag",
    "fxgame_icongrp_hidecar", "fxgame_icongrp_lapk", "fxgame_icongrp_lot",
    "fxgame_icongrp_pursuit", "fxgame_icongrp_rivalr", "fxgame_icongrp_safe",
    "fxgame_icongrp_speedt", "fxgame_icongrp_sprint", "fxgame_icongrp_tollb", "fxmis_coins1",
    "fxmis_dustpuff", "fxmis_glass1", "fxmis_grasshit", "fxmis_hitdust1", "fxmis_leaffall1",
    "fxmis_leafhit", "fxmis_paper1", "fxmis_wooddust1", "fxnis_extradust1", "fxnis_leafblast1",
    "fxnis_leafblast2", "fxnis_leafblast3", "fxnis_leafblast4", "fxnis_steamjet",
    "fxsmk_md_trail", "fxsmk_md_trail02", "fxsprk_lg_dir", "fxsprk_md_dir", "fxsprk_md_trail",
    "fxtd_dr_asphalt_leaves", "fxtd_dr_asphalt_noleaves", "fxtd_dr_asphalt_wet",
    "fxtd_dr_cobble", "fxtd_dr_cobble_wet", "fxtd_dr_dirt", "fxtd_dr_grass",
    "fxtd_dr_grass_wet", "fxtd_dr_sand", "fxtd_dr_sand_wet", "fxtd_fly_asphalt",
    "fxtd_fly_dirt", "fxtd_hit_grass", "fxtd_hit_sand", "fxtd_sk_asphalt",
    "fxtd_sk_asphalt_no_leaves", "fxtd_sk_cobble", "fxtd_sk_sand", "fxtd_sl_asphalt",
    "fxtd_sl_grass", "fxwtr_fountain1", "fxwtr_waterbarrel_L", "fxwtr_waterbarrel_sm",
};

// AI / gameplay system node names — top-level systems wired into the
// per-event vault chunk tree (gameplay.bin 'NtaD' records) (24 names)
constexpr std::array<std::string_view, 24> kAIGameplayNodes = {
    "AICopManager", "AIElementController", "AIParkedCarSpawner", "AIPursuit", "AIRoadSpawn",
    "AITrafficManager", "AIVehicle", "AIVehicleController", "AIZoneController",
    "AvoidableManager", "CameraAI", "Comment", "GameplayActivity", "Physics", "RigidBody",
    "SceneryModel", "SimEnd", "SimStart", "Speech", "VehicleSystem", "WRoadNetwork", "World",
    "WorldUpdate", "drive",
};

// Pursuit AI strategy / dispatch event names — cop dialogue/strategy state
// machine event IDs (setup_*, anytimeevents_*, staticroadblock_*, backup_*,
// extracops_*, helispecific_*, cross_*, rollingstrategy_*, interrupts_*,
// outcome_*, arrest_*) (132 names)
constexpr std::array<std::string_view, 132> kPursuitEvents = {
    "acknowledge", "anytimeevents_bailout", "anytimeevents_bailoutdeny",
    "anytimeevents_callforev", "anytimeevents_collisionworld", "anytimeevents_collworld_air",
    "anytimeevents_collworld_civi", "anytimeevents_collworld_flip",
    "anytimeevents_collworld_spin", "anytimeevents_directionhigh",
    "anytimeevents_disp911report", "anytimeevents_dispbreakaway", "anytimeevents_dispevreply",
    "anytimeevents_dispjurisshift", "anytimeevents_disppursescgen",
    "anytimeevents_disppursuitescalation", "anytimeevents_disppursuitupdate",
    "anytimeevents_disptimeexpired", "anytimeevents_driverhistory",
    "anytimeevents_focuschange", "anytimeevents_heatjump", "anytimeevents_intenttoram",
    "anytimeevents_lostsuspect", "anytimeevents_lostvisual", "anytimeevents_offroadmoment",
    "anytimeevents_pursuitupdaterep", "anytimeevents_regainvisual",
    "anytimeevents_regainvisualinterrupt", "anytimeevents_spotted",
    "anytimeevents_suspectbehaviour", "anytimeevents_suspectbrake",
    "anytimeevents_suspectoutrun", "anytimeevents_suspectuturn", "anytimeevents_unit911reply",
    "anytimeevents_unitdisabled", "anytimeevents_weatherreport", "arrest_arrest",
    "arrest_bullhornarrest", "arrest_disparrestreply", "backup_buarrives", "backup_bureminder",
    "backup_callforbu", "backup_callforswarming", "backup_dispbackupreply",
    "backup_dispbackupupdate", "backup_dispbueta", "backup_disphelibueta",
    "backup_negativebureply", "backup_unitbureply", "cellcall", "cross_crossbailoutdeny_sub",
    "cross_crossbureply", "cross_crossfailreply", "cross_crossmultistrategy",
    "cross_crosspursuitesc", "cross_crossrbfailreply", "cross_crossselfstrategy", "d_day",
    "dispintrorace", "extracops_extrarbaverted", "extracops_extrarbengage",
    "extracops_otherlead", "extracops_possiblesuspect", "extracops_quadrentforming",
    "extracops_quadrentmoving", "extracops_rbposition", "extracops_rbwarning",
    "extracops_superpursuitreply", "extracops_suspectpossiblygone", "extracops_swarmingreply",
    "extracops_swarmingreplyfollow", "extracops_wrongsuspect", "helispecific_helibailout",
    "helispecific_helibullhornarrest", "helispecific_helihazardalert",
    "helispecific_heliintenttobail", "helispecific_helilostvisual",
    "helispecific_heliquadrent", "helispecific_heliquadrentmoving",
    "helispecific_heliselfstrategy", "helispecific_helispotter", "helispecific_heliswarming",
    "interrupts_interrupt", "interrupts_interruptram", "interrupts_interruptram_ho",
    "interrupts_interruptram_re", "interrupts_interruptram_ss", "interrupts_interruptram_tb",
    "interrupts_interruptramhigh", "interrupts_staticinterrupt", "outcome_anticipatefail",
    "outcome_anticipatesuccess", "outcome_outcomefail", "outcome_strategyreset", "outcomes",
    "rollingstrategy_calltoposition", "rollingstrategy_calltopositionrem",
    "rollingstrategy_initstrategy", "rollingstrategy_strategyexecute", "setup_attmptvehstp",
    "setup_bullhorn", "setup_bullhornprefix", "setup_dispcustpaint", "setup_dispgoahead",
    "setup_dispnovehdescrip", "setup_dispvehdescrip", "setup_dispvehdescripvinyls",
    "setup_initialcallforbu", "setup_initialcallforbu_ms", "setup_initpursuit",
    "setup_locationreport", "setup_moredetails", "setup_primaryengage", "setup_reinitpursuit",
    "setup_selfstrategy", "setup_spotter", "setup_spotterreply", "setup_spotterwanted",
    "setup_suspectconfirmed", "setup_vehiclereport", "setup_vehiclereporttag",
    "staticroadblock_callforrb", "staticroadblock_callforrb_sub",
    "staticroadblock_disprbreply", "staticroadblock_disprbupdate", "staticroadblock_dispsubrb",
    "staticroadblock_negativerbreply", "staticroadblock_pursuitapproaching",
    "staticroadblock_rbapproach", "staticroadblock_rbaverted", "staticroadblock_rbengage",
    "staticroadblock_rbreminder",
};

// Smackable object physics flags — collision-response classification used
// by destructible scenery records (8 names)
constexpr std::array<std::string_view, 8> kSmackableFlags = {
    "BOTTOMOUT", "EVENT", "FRONT", "ROLLOVER", "SIDE", "SMOKABLE", "TWO_CAR", "WALL",
};

// Destructible scenery object classes — street-furniture and breakable
// prop names (signs, barrels, fences, roadside structures, etc.) (205 names)
constexpr std::array<std::string_view, 205> kSceneryObjects = {
    "FxCS_Sc_Stone", "FxCS_Sc_Wood", "SpikeStrip", "aluminumroof", "aluminumrooffrag",
    "amphitheater", "amphitheater_base", "bench", "bench_mtl_bar", "bicycle",
    "bigrollingtorus", "boothroof", "bumper", "bus_mtl_peices", "busstoproof", "busstopsmack",
    "campuskiosk", "campuskiosk_seq", "cardboard_box", "carparts", "chair", "chlink_fnc",
    "cone", "coplights", "cranesupport", "cranesupport_seq", "crate", "crsh_barrel",
    "crsh_barrel_frag", "donut_seq", "donutshoproof_frag", "donutshoproof_seq",
    "donutshoproofsupport_seq", "door", "drivein", "drivein_seq", "dumpster", "explosion_seq",
    "explosions", "fallingobject", "fallingobject_seq", "firehydrant", "firehydrant_seq",
    "fishmarketsign", "fmarketsignspawnsubfrag_seq", "foundation", "garbg_bag", "garbg_can",
    "gaspumpexplosion", "gaspumpexplosion_seq", "gassstation_parts", "gasstationroof",
    "gasstationroof_frag", "gasstationroof_seq", "gasstationsupport", "gasstationsupport_seq",
    "gazebo", "gazebo_roof", "gazebo_seq", "gazeboroofsupport", "gazeboroofsupport_seq",
    "genericsmackable", "genericsmackable_metal", "genericwood_frag", "glass_frag",
    "glassshatter1", "glasswindow_frag_seq", "glasswindow_seq", "gls_dr", "gls_toll",
    "gls_toll_no_sound", "golf_flag", "golfcart", "greenhouse", "greenhouse_seq", "haybale",
    "heavyconcrete_seq", "heavyconcreteobject", "hinkleybase", "hinkleybase_seq", "hood",
    "hotdog_stnd", "hugemetalbreakable", "hugemetalbreakable_seq", "hugemetalfrag",
    "hugemetalobject", "hugemetalobject_seq", "ice_box", "kiosk", "kiosk_seq", "largeglass",
    "largemetalobject", "largemetalobjectfrag", "largemetalpost", "largemetalsupport",
    "largesign", "largewoodenobject", "largewoodenobject_seq", "largewoodenpost",
    "largewoodimobile", "lightfixture", "lightmetalobject", "lightplasticobject",
    "lightwoodenobject", "lightwoodenpost", "lounge_chr", "lwn_chr", "mast",
    "mediumconcreteobject", "mediummetalobject", "mediummetalpost", "mediumplasticobject",
    "mediumwoodenobject", "mediumwoodenpost", "metal_gate", "metalexplosion", "metalpost_seq",
    "misc", "moments", "mtl_box", "mtl_bus_roof", "news_box", "oil_drum", "panel",
    "parking_mtr", "parkingarmpost", "payphone", "pic_table", "picket_fence", "placeables",
    "plank", "pop_mach", "potty", "propanetank", "pylon", "rack", "radiotower",
    "radiotower_seq", "radiotowersupport_seq", "recyc_bin", "rm_glass", "rm_largemetalobject",
    "rm_tollbooth", "roadblock", "sailboat", "sailboat_seq", "sailboatmast_seq", "sawhorse",
    "scaffolding", "scaffoldsupport_seq", "seafoodpatio", "seafoodpatio_seq", "sm_wd_roof",
    "smallwoodroof", "smallwoodroof_seq", "smallwoodroofsupport", "smallwoodroofsupport_seq",
    "spawnheavy_seq", "spawnsubfrag_seq", "speed_trap_camera", "spikestrip", "stand",
    "storefront", "stump", "subfrag", "surfboard", "tennisnet", "test", "testsmack",
    "testsplode", "testtrigger", "tire", "tix_mach", "tollbooth", "tollbooth_seq",
    "traffic_cable", "trafficcable_seq", "trafficcablelight", "trafficcablelight_seq",
    "trafficlight", "trafficpole_seq", "trailing_fx", "trailing_fx_med", "trailing_fx_seq",
    "trailing_fx_slow", "tree", "umb_table", "umbrella_table_seq", "vend_mach", "watertower",
    "watertower_seq", "watertowercontainer", "wood_slat_fence", "woodcarparts",
    "woodenscaffold_seq", "woodenscaffolding", "woodgate", "woodroof", "woodroof_frag",
    "woodroofsupport", "woodsimplesmack", "woodsupport", "woodsupport_seq", "wreck",
    "wtr_barrel_seq",
};

// Total: 498 confirmed names across 6 categories

} // namespace

size_t RegisterKnownNames() {
    auto& resolver = HashResolver::Instance();
    size_t before = resolver.Count();

    for (auto name : kSurfaceClasses)   resolver.Register(name);
    for (auto name : kParticleEffects)  resolver.Register(name);
    for (auto name : kAIGameplayNodes)  resolver.Register(name);
    for (auto name : kPursuitEvents)    resolver.Register(name);
    for (auto name : kSmackableFlags)   resolver.Register(name);
    for (auto name : kSceneryObjects)   resolver.Register(name);

    return resolver.Count() - before;
}

} // namespace nfsmw
