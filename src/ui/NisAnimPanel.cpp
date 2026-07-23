#include "ui/NisAnimPanel.h"
#include "core/ChunkReader.h"
#include "core/LZCDecompressor.h"
#include <vangui.h>
#include <fstream>
#include <cstring>

namespace nfsmw {

namespace {

// Walk a chunk tree collecting every 0x00E34009 payload + its absolute offset.
void CollectNisAnimChunks(std::span<const uint8_t> data, size_t base,
                          std::vector<std::pair<size_t, std::vector<uint8_t>>>& out) {
    size_t pos = 0;
    while (pos + sizeof(ChunkHeader) <= data.size()) {
        ChunkHeader h{};
        std::memcpy(&h, data.data() + pos, sizeof(h));
        if (h.id == 0 && h.size == 0) break;
        size_t payloadOff = pos + sizeof(h);
        if (payloadOff + h.size > data.size()) break;
        auto payload = data.subspan(payloadOff, h.size);

        if (h.id == ChunkID::NisAnimation) {
            out.emplace_back(base + payloadOff,
                             std::vector<uint8_t>(payload.begin(), payload.end()));
        } else if (ChunkReader::IsContainer(h.id) && h.size >= 8) {
            CollectNisAnimChunks(payload, base + payloadOff, out);
        }
        pos = payloadOff + h.size;
    }
}

Result<std::vector<std::pair<size_t, std::vector<uint8_t>>>>
LoadAndCollect(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
        return Result<std::vector<std::pair<size_t, std::vector<uint8_t>>>>::Err(
            "Cannot open '" + path.string() + "'");
    auto sz = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> raw(sz);
    if (!f.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(sz)))
        return Result<std::vector<std::pair<size_t, std::vector<uint8_t>>>>::Err(
            "Read error: " + path.string());

    std::span<const uint8_t> data;
    std::vector<uint8_t> decompressed;
    if (LZCDecompressor::IsCompressed(raw)) {
        auto r = LZCDecompressor::Decompress(raw);
        if (!r)
            return Result<std::vector<std::pair<size_t, std::vector<uint8_t>>>>::Err(
                "JDLZ decompress failed: " + r.error);
        decompressed = std::move(r.value);
        data = decompressed;
    } else {
        data = raw;
    }

    std::vector<std::pair<size_t, std::vector<uint8_t>>> chunks;
    CollectNisAnimChunks(data, 0, chunks);
    return Result<std::vector<std::pair<size_t, std::vector<uint8_t>>>>::Ok(std::move(chunks));
}

} // namespace

void NisAnimPanel::Load(const std::filesystem::path& path, TaskQueue& tasks) {
    Close();
    state_      = State::Loading;
    sourceName_ = path.filename().string();

    using ChunkList = std::vector<std::pair<size_t, std::vector<uint8_t>>>;
    auto result = std::make_shared<Result<ChunkList>>();
    tasks.Submit("Loading NIS cutscene animation data", [path, result](ProgressState& p) {
        p.fraction = -1.f;
        *result = LoadAndCollect(path);
    }, [this, result] {
        if (!*result) {
            errorMsg_ = result->error;
            state_    = State::Error;
            return;
        }
        clips_.clear();
        for (auto& [offset, bytes] : result->value) {
            auto parsed = NisAnimParser::Parse(bytes);
            if (!parsed) continue; // skip stub/malformed chunks, don't fabricate
            clips_.push_back({offset, std::move(parsed.value)});
        }
        selectedClip_ = clips_.empty() ? -1 : 0;
        state_        = State::Ready;
    });
}

void NisAnimPanel::Close() {
    clips_.clear();
    selectedClip_ = -1;
    state_        = State::Empty;
    errorMsg_.clear();
    sourceName_.clear();
    boneFilter_[0] = '\0';
}

void NisAnimPanel::Draw(float w, float h, FileDialog& /*fd*/, TaskQueue& /*tasks*/) {
    switch (state_) {
        case State::Empty:   DrawEmptyState(w, h);   break;
        case State::Loading: DrawLoadingState(w, h); break;
        case State::Error:   DrawErrorState(w, h);   break;
        case State::Ready:   DrawReadyState(w, h);   break;
    }
}

void NisAnimPanel::DrawEmptyState(float w, float h) {
    VanGui::SetCursorPos(VanVec2(w * 0.5f - 160.0f, h * 0.5f - 20.0f));
    VanGui::TextDisabled("Open a NIS cutscene bundle to browse its animation data.");
}

void NisAnimPanel::DrawLoadingState(float /*w*/, float /*h*/) {
    VanGui::TextDisabled("Loading...");
}

void NisAnimPanel::DrawErrorState(float /*w*/, float /*h*/) {
    VanGui::TextColored(VanVec4(0.9f, 0.3f, 0.3f, 1.0f), "Error: %s", errorMsg_.c_str());
}

void NisAnimPanel::DrawReadyState(float w, float h) {
    if (clips_.empty()) {
        VanGui::TextDisabled("'%s' has no recognizable animation chunks (0x00E34009).",
                            sourceName_.c_str());
        return;
    }

    VanGui::TextWrapped(
        "Real, named skeleton + bone data recovered from this scene's embedded "
        "animation object. Keyframe rotation curves are not decoded yet (they live "
        "in an unindexed region of the data) -- this view shows the rig structure only.");
    VanGui::Dummy(VanVec2(0.0f, 4.0f));

    const float listW = 260.0f;
    VanGui::BeginChild("##clipList", VanVec2(listW, h - 50.0f), true);
    for (size_t i = 0; i < clips_.size(); ++i) {
        const Clip& c = clips_[i];
        char label[160];
        std::snprintf(label, sizeof(label), "%s  (%zu bones) @+0x%zX",
                      c.data.skeletonName.empty() ? "(unnamed)" : c.data.skeletonName.c_str(),
                      c.data.boneSymbols.size(), c.chunkOffset);
        if (VanGui::Selectable(label, selectedClip_ == static_cast<int>(i)))
            selectedClip_ = static_cast<int>(i);
    }
    VanGui::EndChild();

    VanGui::SameLine();
    VanGui::BeginChild("##boneList", VanVec2(w - listW - 8.0f, h - 50.0f), true);
    if (selectedClip_ >= 0 && selectedClip_ < static_cast<int>(clips_.size())) {
        const Clip& c = clips_[selectedClip_];
        VanGui::Text("Skeleton: %s", c.data.skeletonName.empty() ? "(unnamed)"
                                                                 : c.data.skeletonName.c_str());
        VanGui::Text("Bones: %zu", c.data.boneSymbols.size());
        VanGui::Separator();
        VanGui::SetNextItemWidth(240.0f);
        VanGui::InputTextWithHint("##boneFilter", "Filter bones...", boneFilter_,
                                 sizeof(boneFilter_));
        VanGui::Separator();

        if (VanGui::BeginTable("##bones", 1, VanGuiTableFlags_RowBg)) {
            for (size_t bi : c.data.boneSymbols) {
                const NisAnimSymbol& sym = c.data.symbols[bi];
                // Strip the "__Bone:::<skeleton>." prefix for a clean display name.
                std::string display = sym.name;
                size_t dot = display.find('.');
                if (dot != std::string::npos) display = display.substr(dot + 1);
                if (boneFilter_[0] != '\0' &&
                    display.find(boneFilter_) == std::string::npos)
                    continue;
                VanGui::TableNextRow();
                VanGui::TableNextColumn();
                VanGui::TextUnformatted(display.c_str());
            }
            VanGui::EndTable();
        }
    }
    VanGui::EndChild();
}

} // namespace nfsmw
