#pragma once
// ─── ui/CarMeshPanel.h ────────────────────────────────────────────────────────
// Geometry export / import for car body and wheel BUN files.
// Calls existing Exporter and MeshImporter/SolidWriter APIs directly.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "patch/BackupManager.h"
#include "editor/Exporter.h"
#include "editor/MeshImporter.h"
#include "ui/FileDialog.h"
#include "async/TaskQueue.h"
#include <string>

namespace nfsmw {

struct CarContext;

class CarMeshPanel {
public:
    void Draw(CarContext& ctx, TaskQueue& tasks);

private:
    int          selectedPart_  = 0;
    BackupManager backup_;
    FileDialog   exportDialog_;
    FileDialog   importDialog_;
    std::string  lastStatus_;

    bool importInProgress_ = false;

    void DoExportPart(const CarContext& ctx, TaskQueue& tasks);
    void DoExportSection(const CarContext& ctx, bool wheels, TaskQueue& tasks);
    void DoImportReplace(CarContext& ctx, const std::filesystem::path& src,
                         TaskQueue& tasks);
};

} // namespace nfsmw
