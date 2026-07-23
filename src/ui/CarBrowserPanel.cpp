// ─── ui/CarBrowserPanel.cpp ───────────────────────────────────────────────────
#include "ui/CarBrowserPanel.h"
#include <vangui.h>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace nfsmw {

// ─── Display-name table ────────────────────────────────────────────────────
// Friendly names for known car/cop directory IDs. Not exhaustive — any ID
// not listed here is shown as-is in the browser list.
const std::unordered_map<std::string, std::string> CarBrowserPanel::kCarNames = {
    // Playable cars
    {"350Z",        "Nissan 350Z"},
    {"911GT2",      "Porsche 911 GT2"},
    {"911TURBO",    "Porsche 911 Turbo"},
    {"997S",        "Porsche 997 S"},
    {"A3",          "Audi A3"},
    {"A4",          "Audi A4"},
    {"BMWM3",       "BMW M3 E46"},
    {"BMWM3GTR",    "BMW M3 GTR"},
    {"BMWM3GTRE46", "BMW M3 GTR (E46 Street)"},
    {"CAMARO",      "Chevrolet Camaro SS"},
    {"CARRERAGT",   "Porsche Carrera GT"},
    {"CAYMANS",     "Porsche Cayman S"},
    {"CLK500",      "Mercedes-Benz CLK500"},
    {"COBALTSS",    "Chevrolet Cobalt SS"},
    {"CORVETTE",    "Chevrolet Corvette C5"},
    {"CORVETTEC6R", "Chevrolet Corvette C6.R"},
    {"CTS",         "Cadillac CTS-V"},
    {"DB9",         "Aston Martin DB9"},
    {"ECLIPSEGT",   "Mitsubishi Eclipse GT"},
    {"ELISE",       "Lotus Elise"},
    {"FORDGT",      "Ford GT"},
    {"GALLARDO",    "Lamborghini Gallardo"},
    {"GTI",         "Volkswagen Golf GTI"},
    {"GTO",         "Pontiac GTO"},
    {"IMPREZAWRX",  "Subaru Impreza WRX STI"},
    {"IS300",       "Lexus IS300"},
    {"LANCEREVO8",  "Mitsubishi Lancer Evolution VIII"},
    {"MURCIELAGO",  "Lamborghini Murciélago"},
    {"MUSTANGGT",   "Ford Mustang GT"},
    {"RX7",         "Mazda RX-7"},
    {"RX8",         "Mazda RX-8"},
    {"RX8SPEEDT",   "Mazda RX-8 (Speed Test)"},
    {"SL65",        "Mercedes-Benz SL65 AMG"},
    {"SL500",       "Mercedes-Benz SL500"},
    {"SLR",         "Mercedes-Benz SLR McLaren"},
    {"SUPRA",       "Toyota Supra"},
    {"TT",          "Audi TT"},
    {"VIPER",       "Dodge Viper SRT-10"},
    // Cop cars
    {"COPGHOST",      "[COP] Camaro SS"},
    {"COPGTO",        "[COP] Pontiac GTO"},
    {"COPGTOGHOST",   "[COP] Pontiac GTO (Ghost)"},
    {"COPHELI",       "[COP] Helicopter"},
    {"COPMIDSIZE",    "[COP] Midsize Sedan"},
    {"COPMIDSIZEINT", "[COP] Midsize Sedan (Interceptor)"},
    {"COPSPORT",      "[COP] Sport Coupe"},
    {"COPSPORTGHOST", "[COP] Sport Coupe (Ghost)"},
    {"COPSPORTHENCH", "[COP] Sport Coupe (Henchman)"},
    {"COPSUV",        "[COP] SUV"},
    {"COPSUVL",       "[COP] SUV (Lieutenant)"},
};

void CarBrowserPanel::Scan(const std::filesystem::path& carsRoot, TaskQueue& tasks) {
    scanning_     = true;
    pendingReady_ = false;
    carIds_.clear();
    selectedId_.clear();

    tasks.Submit("Scanning CARS/", [this, carsRoot](ProgressState&) {
        std::vector<std::string> found;
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(carsRoot, ec)) {
            if (!entry.is_directory()) continue;
            const std::string name = entry.path().filename().string();
            // Car directory names are alphanumeric (+ underscore), e.g. "M3G",
            // "350Z", "BMWM3GTR", "COBALTSS". Accept any case so extracted
            // installs that lower-cased folder names still show up.
            bool valid = !name.empty();
            for (char c : name) {
                if (!std::isalnum((unsigned char)c) && c != '_') { valid = false; break; }
            }
            if (!valid) continue;

            // A folder counts as a car if it carries any recognised car asset.
            // Two real-world layouts exist:
            //   • packed   — "<id>.BUN" (a single bundle)
            //   • extracted/retail — loose GEOMETRY.BIN / TEXTURES.BIN /
            //     VINYLS.BIN / PREVINYL.BIN  (no <id>.BUN at all)
            // The old check required <id>.BUN only, so extracted CARS folders
            // showed "No cars found". (Filesystem is case-insensitive on
            // Windows, so the uppercase names also match lower-cased files.)
            const std::filesystem::path dir = entry.path();
            const char* kMarkers[] = {
                nullptr,            // slot for "<id>.BUN", filled below
                "GEOMETRY.BIN", "TEXTURES.BIN", "VINYLS.BIN", "PREVINYL.BIN",
                "CARP.BIN", "CAR.BIN",
            };
            const std::string bun = name + ".BUN";
            kMarkers[0] = bun.c_str();
            bool isCar = false;
            for (const char* m : kMarkers) {
                if (std::filesystem::exists(dir / m)) { isCar = true; break; }
            }
            if (!isCar) continue;
            found.push_back(name);
        }
        std::sort(found.begin(), found.end());
        pendingIds_   = std::move(found);
        pendingReady_ = true;
    }, /*onDone=*/[this]() {
        // Drained in Draw() via PumpPending()
    });
}

void CarBrowserPanel::PumpPending() {
    if (!pendingReady_) return;
    carIds_       = std::move(pendingIds_);
    scanning_     = false;
    pendingReady_ = false;
}

void CarBrowserPanel::Draw() {
    PumpPending();

    if (scanning_) {
        VanGui::TextUnformatted("Scanning CARS/ ...");
        return;
    }

    if (carIds_.empty()) {
        VanGui::TextDisabled("No cars found.");
        VanGui::TextDisabled("Open a CARS/ directory first.");
        return;
    }

    // Filter box
    VanGui::SetNextItemWidth(-1.f);
    char filterBuf[128] = {};
    strncpy_s(filterBuf, sizeof(filterBuf), filter_.c_str(), _TRUNCATE);
    if (VanGui::InputTextWithHint("##carfilter", "Filter...", filterBuf, sizeof(filterBuf)))
        filter_ = filterBuf;

    VanGui::Separator();

    (void)VanGui::GetTextLineHeightWithSpacing(); // rowH reserved for future use
    VanVec2 listSize(-1.f, -1.f);
    (void)VanGui::BeginChild("##carlist", listSize, false);

    for (const auto& id : carIds_) {
        if (!filter_.empty()) {
            // Case-insensitive substring match
            std::string idL = id, fL = filter_;
            std::transform(idL.begin(), idL.end(), idL.begin(),
                           [](unsigned char c){ return (char)std::tolower(c); });
            std::transform(fL.begin(),  fL.end(),  fL.begin(),
                           [](unsigned char c){ return (char)std::tolower(c); });
            if (idL.find(fL) == std::string::npos) continue;
        }

        bool selected = (id == selectedId_);
        const auto it = kCarNames.find(id);
        const std::string label = (it != kCarNames.end())
                                  ? (id + "  —  " + it->second)
                                  : id;
        if (VanGui::Selectable(label.c_str(), selected,
                              VanGuiSelectableFlags_SpanAllColumns)) {
            selectedId_ = id;
            if (onSelect_) onSelect_(id);
        }
    }

    VanGui::EndChild();
}

} // namespace nfsmw
