// ─── ui/CarMeshPanel.cpp ──────────────────────────────────────────────────────
#include "ui/CarMeshPanel.h"
#include "ui/CarPanel.h"
#include "editor/Exporter.h"
#include "editor/MeshImporter.h"
#include "patch/SolidWriter.h"
#include <vangui.h>

namespace nfsmw {

void CarMeshPanel::Draw(CarContext& ctx, TaskQueue& tasks) {
    if (!ctx.geometryReady) {
        VanGui::TextDisabled("Geometry not loaded.");
        return;
    }

    const auto& sl = ctx.bodySection.solidLists.empty()
                     ? SolidList{} : ctx.bodySection.solidLists[0];
    const bool hasParts = !sl.objects.empty();

    // ── Part selector ─────────────────────────────────────────────────────
    VanGui::AlignTextToFramePadding();
    VanGui::TextUnformatted("Part:");
    VanGui::SameLine();
    VanGui::SetNextItemWidth(220.f);
    if (hasParts) {
        const char* preview = sl.objects[selectedPart_].name.c_str();
        if (VanGui::BeginCombo("##meshpart", preview)) {
            for (int i = 0; i < (int)sl.objects.size(); ++i) {
                bool sel = (i == selectedPart_);
                if (VanGui::Selectable(sl.objects[i].name.c_str(), sel))
                    selectedPart_ = i;
                if (sel) VanGui::SetItemDefaultFocus();
            }
            VanGui::EndCombo();
        }
    } else {
        VanGui::TextDisabled("(no parts)");
    }
    VanGui::Separator();

    // ── Per-part export ───────────────────────────────────────────────────
    if (hasParts) {
        if (VanGui::Button("Export part as OBJ..."))
            DoExportPart(ctx, tasks);
        VanGui::SameLine();
        if (VanGui::Button("Export part as GLB..."))
            DoExportPart(ctx, tasks); // format chosen via dialog filter

        VanGui::Spacing();

        if (importInProgress_) {
            VanGui::TextUnformatted("Importing...");
        } else {
            if (VanGui::Button("Import replacement (OBJ / GLB / GLTF)..."))
                DoImportReplace(ctx, {}, tasks);
        }
    }

    VanGui::Separator();

    // ── Full-section export ───────────────────────────────────────────────
    if (VanGui::Button("Export whole body (OBJ)..."))
        DoExportSection(ctx, /*wheels=*/false, tasks);
    VanGui::SameLine();
    if (VanGui::Button("Export wheels (OBJ)..."))
        DoExportSection(ctx, /*wheels=*/true, tasks);

    // ── Status ────────────────────────────────────────────────────────────
    if (!lastStatus_.empty()) {
        VanGui::Spacing();
        VanGui::TextWrapped("%s", lastStatus_.c_str());
    }
}

void CarMeshPanel::DoExportPart(const CarContext& ctx, TaskQueue& tasks) {
    if (ctx.bodySection.solidLists.empty()) return;
    const auto& sl  = ctx.bodySection.solidLists[0];
    if (selectedPart_ >= (int)sl.objects.size()) return;
    const SolidObject& obj = sl.objects[selectedPart_];

    exportDialog_.Show("Export Part", FileDialog::Mode::Save, {".obj", ".glb"},
        [this, &ctx, &obj, &tasks](const std::filesystem::path& dest) {
            tasks.Submit("Exporting part", [&ctx, &obj, dest](ProgressState&) {
                Exporter::ExportOptions opts;
                if (dest.extension() == ".glb")
                    Exporter::ExportObjectGLTF(obj, ctx.bodySection, dest, opts);
                else
                    Exporter::ExportObjectOBJ(obj, ctx.bodySection, dest, opts);
            }, [this, dest]() {
                lastStatus_ = "Exported to " + dest.string();
            });
        });
}

void CarMeshPanel::DoExportSection(const CarContext& ctx, bool wheels,
                                    TaskQueue& tasks) {
    const StreamSection& sect = wheels ? ctx.wheelSection : ctx.bodySection;
    exportDialog_.Show("Export Section", FileDialog::Mode::Save, {".obj", ".glb"},
        [this, &sect, &tasks](const std::filesystem::path& dest) {
            tasks.Submit("Exporting section", [&sect, dest](ProgressState&) {
                Exporter::ExportOptions opts;
                if (dest.extension() == ".glb")
                    Exporter::ExportSectionGLTF(sect, dest, opts);
                else
                    Exporter::ExportSectionOBJ(sect, dest, opts);
            }, [this, dest]() {
                lastStatus_ = "Exported to " + dest.string();
            });
        });
}

void CarMeshPanel::DoImportReplace(CarContext& ctx,
                                    const std::filesystem::path& /*src*/,
                                    TaskQueue& tasks) {
    if (ctx.bodySection.solidLists.empty()) return;
    const auto& sl = ctx.bodySection.solidLists[0];
    if (selectedPart_ >= (int)sl.objects.size()) return;

    // Snapshot everything the worker thread needs BEFORE the lambda is
    // created. `ctx` (and `selectedPart_`, which can change if the user
    // re-selects a part while the import is running) must not be touched
    // from the background task.
    const std::filesystem::path bodyBUN = ctx.carDir / (ctx.id + ".BUN");
    const uint32_t targetHash = sl.objects[selectedPart_].nameHash;

    importDialog_.Show("Import Replacement Mesh", FileDialog::Mode::Open,
        {".obj", ".glb", ".gltf"},
        [this, bodyBUN, targetHash, &tasks](const std::filesystem::path& src) {
            importInProgress_ = true;
            tasks.Submit("Importing mesh",
                [this, bodyBUN, targetHash, src](ProgressState&) {
                    backup_.EnsureFileBak(bodyBUN);

                    auto importResult = MeshImporter::Import(src);
                    if (!importResult) {
                        lastStatus_ = "Import failed: " + importResult.error;
                        return;
                    }
                    auto writeResult = SolidWriter::ReplaceObject(
                        bodyBUN, targetHash, importResult.value, backup_);
                    if (!writeResult)
                        lastStatus_ = "Replace failed: " + writeResult.error;
                    else
                        lastStatus_ = "Import successful.";
                }, [this]() {
                    importInProgress_ = false;
                });
        });
}

} // namespace nfsmw
