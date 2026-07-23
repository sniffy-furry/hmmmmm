// ─── ui/CarPanel.cpp ──────────────────────────────────────────────────────────
#include "ui/CarPanel.h"
#include "formats/StreamBundle.h"
#include "formats/CarInfo.h"
#include "formats/CarSlot.h"
#include "formats/GINFile.h"
#include <vangui.h>
#include <algorithm>
#include <cctype>
#include <system_error>

namespace nfsmw {

namespace {

// Reduce a string to uppercase alphanumerics so "GIN_BMW_M3.gin" and "BMWM3"
// can be compared. Drops separators, case, and extension noise.
std::string NormAlnum(std::string_view s) {
    std::string o;
    o.reserve(s.size());
    for (char c : s)
        if (std::isalnum((unsigned char)c)) o += (char)std::toupper((unsigned char)c);
    return o;
}

// Engine GIN names are "GIN_<car>[_<variant>].gin". Strip the leading "GIN"
// token and the extension, then normalise — leaving the car-identifying core
// (plus any variant suffix) for matching.
std::string GinCore(const std::string& filename) {
    std::string stem = filename;
    auto dot = stem.find_last_of('.');
    if (dot != std::string::npos) stem = stem.substr(0, dot);
    std::string n = NormAlnum(stem);
    if (n.rfind("GIN", 0) == 0) n = n.substr(3);
    return n;
}

size_t CommonPrefix(const std::string& a, const std::string& b) {
    size_t n = std::min(a.size(), b.size()), i = 0;
    while (i < n && a[i] == b[i]) ++i;
    return i;
}

// Score how well a GIN filename matches a car id (higher = better; 0 = no
// usable match). GIN names often carry a manufacturer prefix the car-folder id
// lacks (e.g. car "CARRERAGT" vs "GIN_POR_Carrera_GT", "CLK500" vs
// "GIN_MBZ_CLK500"), so substring containment matters as much as prefix.
// De-prioritises the companion deceleration banks (*_DCL / *_Decel).
int ScoreGin(const std::string& carId, const std::string& ginFile) {
    const std::string car  = NormAlnum(carId);
    const std::string core = GinCore(ginFile);
    if (car.empty() || core.empty()) return 0;

    int score = 0;
    if (core == car) {
        score = 1000;
    } else if (core.rfind(car, 0) == 0) {            // core starts with car
        score = 800 - (int)(core.size() - car.size());
    } else if (car.rfind(core, 0) == 0) {            // car starts with core
        score = 700 - (int)(car.size() - core.size());
    } else if (car.size() >= 3) {
        // Manufacturer-prefixed names: car id appears inside the GIN core.
        auto pos = core.find(car);
        if (pos != std::string::npos) {
            score = 600 - (int)(core.size() - car.size()) - (int)pos * 5;
        } else if (core.size() >= 3 && car.find(core) != std::string::npos) {
            score = 500 - (int)(car.size() - core.size());
        } else {
            size_t cp = CommonPrefix(car, core);
            if (cp < 3) return 0;                    // too weak to be meaningful
            score = 100 + (int)cp * 20;
        }
    } else {
        size_t cp = CommonPrefix(car, core);
        if (cp < 2) return 0;
        score = 100 + (int)cp * 20;
    }

    std::string up = NormAlnum(ginFile);
    if (up.find("DCL") != std::string::npos || up.find("DECEL") != std::string::npos)
        score -= 120;                                // decel bank is secondary
    score -= (int)core.size();                       // mild preference for the base variant
    return score;
}

} // namespace

CarPanel::CarPanel() = default;

Result<void> CarPanel::Init() {
    if (auto r = viewer_.Init(); !r) return r;
    // Other sub-panels have no GL resources to initialise
    return Result<void>::Ok();
}

void CarPanel::Shutdown() {
    viewer_.Shutdown();
    texPanel_.Shutdown();
    vinylPanel_.Shutdown();
}

void CarPanel::Open(const std::filesystem::path& carsRoot, TaskQueue& tasks) {
    carsRoot_ = carsRoot;
    activeTab_ = 0;

    // If a previous LoadCar() task is still running on a worker thread, do
    // NOT reset ctx_ here — that would race with the in-flight write. The
    // root change still takes effect (carsRoot_ above); the stale load will
    // finish harmlessly and the user can reselect a car afterwards.
    if (!loading_.load(std::memory_order_acquire)) {
        ctx_ = CarContext{};     // reset
    }

    browser_.SetOnSelect([this, &tasks](const std::string& id) {
        LoadCar(id, tasks);
    });
    browser_.Scan(carsRoot, tasks);
}

bool CarPanel::HasCars() const { return browser_.HasCars(); }

// ─────────────────────────────────────────────────────────────────────────────
// LoadCar — async load of all car data into CarContext
// ─────────────────────────────────────────────────────────────────────────────
void CarPanel::LoadCar(const std::string& carId, TaskQueue& tasks) {
    if (loading_.load(std::memory_order_acquire)) {
        // A load is already running on a worker thread and writing into
        // ctx_. Don't start a second one against the same CarContext —
        // remember the most recent request and pick it up once the current
        // load's onDone callback (UI thread) fires below.
        pendingCarId_ = carId;
        hasPending_   = true;
        return;
    }

    loading_    = true;
    ctx_        = CarContext{};
    ctx_.id     = carId;
    ctx_.carDir = carsRoot_ / carId;

    tasks.Submit("Loading " + carId, [this, carId](ProgressState&) {
        const auto& dir = ctx_.carDir;

        // ── Phase A: geometry ──────────────────────────────────────────────
        // Two on-disk layouts are supported:
        //   • packed     — "<id>.BUN" (+ "<id>_WHEEL.BUN") bundles
        //   • extracted  — loose GEOMETRY.BIN (mesh) + TEXTURES.BIN (TPK),
        //                  the layout retail/extracted CARS folders ship.
        {
            auto bodyResult = StreamBundleLoader::Load(dir / (carId + ".BUN"));

            // Fall back to the extracted layout if the packed bundle is absent
            // or carried no geometry.
            if (!bodyResult || bodyResult.value.solidLists.empty()) {
                StreamSection merged;
                if (bodyResult) merged = std::move(bodyResult.value);

                if (auto geo = StreamBundleLoader::Load(dir / "GEOMETRY.BIN")) {
                    for (auto& sl : geo.value.solidLists)
                        merged.solidLists.push_back(std::move(sl));
                    for (auto& tp : geo.value.texturePacks)
                        merged.texturePacks.push_back(std::move(tp));
                }
                if (auto tex = StreamBundleLoader::Load(dir / "TEXTURES.BIN")) {
                    for (auto& tp : tex.value.texturePacks)
                        merged.texturePacks.push_back(std::move(tp));
                }
                bodyResult = Result<StreamSection>::Ok(std::move(merged));
            }

            if (bodyResult && (!bodyResult.value.solidLists.empty() ||
                               !bodyResult.value.texturePacks.empty())) {
                ctx_.bodySection  = std::move(bodyResult.value);
                // Collect TPK blocks for the texture panel
                ctx_.bodyTPKs = ctx_.bodySection.texturePacks;
                ctx_.geometryReady = !ctx_.bodySection.solidLists.empty();
            }

            // Wheels: packed cars keep them in a separate bundle; extracted
            // cars bundle wheel meshes inside GEOMETRY.BIN, so this is optional.
            if (auto wheelResult = StreamBundleLoader::Load(dir / (carId + "_WHEEL.BUN")))
                ctx_.wheelSection = std::move(wheelResult.value);
        }

        // ── Phase B: CARSKINS.BUN (shared vinyl atlas) ─────────────────────
        {
            auto skinsPath = carsRoot_.parent_path() / "GLOBAL" / "CARSKINS.BUN";
            auto result    = StreamBundleLoader::Load(skinsPath);
            if (result && !result.value.texturePacks.empty())
                ctx_.skinsTPK = std::move(result.value.texturePacks[0]);
            ctx_.texturesReady = true;
        }

        // ── Phase B2: per-car vinyl decal textures (VINYLS.BIN) ────────────
        // VINYLS.BIN is a real TPK of the car's vinyl/decal textures (every
        // car ships one). This is the actual vinyl data — not the fictional
        // CARSLOT layer stack the old parser expected.
        {
            auto result = StreamBundleLoader::Load(dir / "VINYLS.BIN");
            if (result && !result.value.texturePacks.empty()) {
                ctx_.vinylTPKs = std::move(result.value.texturePacks);
                ctx_.vinylReady = true;
            }
        }

        // ── Phase C: performance data (stubbed) ───────────────────────────
        {
            auto perfPath = dir / "CARINFO.BIN";
            auto result   = CarInfoParser::Load(perfPath, carId);
            if (result) {
                ctx_.perfData  = std::move(result.value);
                ctx_.perfReady = true;
            }
            // If stubbed, perfReady stays false; panels show stub notice.
        }

        // ── Phase D: pursuit data (stubbed) ───────────────────────────────
        {
            auto result = CarPursuitParser::Load(ctx_.carDir / "CARINFO.BIN", carId);
            if (result) {
                ctx_.pursuitData  = std::move(result.value);
                ctx_.pursuitReady = true;
            }
        }

        // ── Phase E: engine audio ──────────────────────────────────────────
        // Engine GINs live in <MW>/SOUND/ENGINE/ as GIN_<car>[_<variant>].gin,
        // NOT in the car folder. Names don't map 1:1 to car ids, so we list
        // every GIN, auto-pick the best match, and let the Engine Audio tab
        // offer a dropdown to override.
        {
            ctx_.engineDir = carsRoot_.parent_path() / "SOUND" / "ENGINE";
            std::error_code ec;
            if (std::filesystem::is_directory(ctx_.engineDir, ec)) {
                for (const auto& e :
                     std::filesystem::directory_iterator(ctx_.engineDir, ec)) {
                    if (!e.is_regular_file()) continue;
                    auto ext = e.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(),
                                   [](unsigned char c){ return (char)std::tolower(c); });
                    if (ext == ".gin")
                        ctx_.engineGINs.push_back(e.path().filename().string());
                }
                std::sort(ctx_.engineGINs.begin(), ctx_.engineGINs.end());
            }

            // Best fuzzy match for this car.
            int best = 0;
            for (const auto& g : ctx_.engineGINs) {
                int s = ScoreGin(carId, g);
                if (s > best) { best = s; ctx_.engineGINName = g; }
            }

            if (!ctx_.engineGINName.empty()) {
                auto result = GINParser::Load(ctx_.engineDir / ctx_.engineGINName);
                if (result) {
                    ctx_.engineGIN  = std::move(result.value);
                    ctx_.audioReady = true;
                }
            }
        }

        ctx_.loaded = true;

    }, /*onDone=*/[this, carId, &tasks]() {
        loading_ = false;

        // Notify sub-panels (UI thread)
        if (ctx_.geometryReady)
            viewer_.OnCarLoaded(ctx_);
        texPanel_.OnCarLoaded(ctx_);
        vinylPanel_.OnCarLoaded(ctx_);
        perfPanel_.OnCarLoaded(ctx_);
        pursuitPanel_.OnCarLoaded(ctx_);
        // Always notify the engine-audio panel: even with no auto-matched GIN,
        // it shows the SOUND/ENGINE dropdown so the user can pick one.
        audioPanel_.OnCarLoaded(ctx_, tasks);

        // If the user selected another car while this one was loading,
        // start that load now that ctx_ is no longer being written to.
        if (hasPending_) {
            hasPending_ = false;
            const std::string next = std::move(pendingCarId_);
            LoadCar(next, tasks);
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw
// ─────────────────────────────────────────────────────────────────────────────
void CarPanel::Draw(TaskQueue& tasks) {
    const float totalW = VanGui::GetContentRegionAvail().x;
    const float totalH = VanGui::GetContentRegionAvail().y;

    // Keep the browser pane from collapsing to zero/negative width if the
    // whole panel got narrower than the minimum since last frame.
    browserWidth_ = std::min(browserWidth_, std::max(kBrowserWidthMin, totalW - kBrowserWidthMin));

    // ── Left: car browser (drag the right border to resize) ────────────────
    (void)VanGui::BeginChild("##carbrowser", VanVec2(browserWidth_, totalH),
                       VanGuiChildFlags_ResizeX | VanGuiChildFlags_Borders);
    browser_.Draw();
    // Capture any user drag from this frame so next frame's BeginChild call
    // (which sets the size explicitly) doesn't snap back to the old width.
    browserWidth_ = std::max(kBrowserWidthMin, VanGui::GetWindowWidth());
    VanGui::EndChild();

    VanGui::SameLine();

    // ── Right: detail tabs ────────────────────────────────────────────────
    const float detailW = totalW - browserWidth_ - VanGui::GetStyle().ItemSpacing.x;

    (void)VanGui::BeginChild("##cardetail", VanVec2(detailW, totalH), false);

    if (loading_.load(std::memory_order_acquire) && !browser_.SelectedId().empty()) {
        VanGui::TextDisabled("Loading %s ...", browser_.SelectedId().c_str());
    } else if (browser_.SelectedId().empty()) {
        VanGui::TextDisabled("Select a car from the list.");
    } else {
        DrawDetailTabs(tasks, detailW, totalH);
    }

    VanGui::EndChild();
}

void CarPanel::DrawDetailTabs(TaskQueue& tasks, float /*w*/, float /*h*/) {
    // Header showing selected car
    VanGui::Text("Car: %s", ctx_.id.c_str());
    VanGui::Separator();

    if (VanGui::BeginTabBar("##cartabs")) {
        if (VanGui::BeginTabItem("Viewer")) {
            viewer_.Draw(ctx_, tasks);
            VanGui::EndTabItem();
        }
        if (VanGui::BeginTabItem("Mesh")) {
            meshPanel_.Draw(ctx_, tasks);
            VanGui::EndTabItem();
        }
        if (VanGui::BeginTabItem("Textures")) {
            texPanel_.Draw(ctx_, tasks);
            VanGui::EndTabItem();
        }
        if (VanGui::BeginTabItem("Vinyls")) {
            vinylPanel_.Draw(ctx_, tasks);
            VanGui::EndTabItem();
        }
        if (VanGui::BeginTabItem("Performance")) {
            perfPanel_.Draw(ctx_, tasks);
            VanGui::EndTabItem();
        }
        if (VanGui::BeginTabItem("Pursuit")) {
            pursuitPanel_.Draw(ctx_, tasks);
            VanGui::EndTabItem();
        }
        if (VanGui::BeginTabItem("Engine Audio")) {
            audioPanel_.Draw(ctx_, tasks);
            VanGui::EndTabItem();
        }
        VanGui::EndTabBar();
    }
}

} // namespace nfsmw
