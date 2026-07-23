#pragma once
// ─── editor/MeshImporter.h ────────────────────────────────────────────────────
// Import OBJ and binary glTF (.glb) / glTF (.gltf) meshes into an ImportModel
// for object rebuild (see patch/SolidWriter.h). One sub-mesh per material group.
//
// Coordinate handling mirrors the exporters:
//   • OBJ  — written in game space (Z-up) with V flipped; import flips V back.
//   • glTF — written Y-up; import converts Y-up → game Z-up via (x, -z, y).
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include "patch/SolidWriter.h"   // ImportModel / ImportMesh
#include <filesystem>

namespace nfsmw {

class MeshImporter {
public:
    /// Dispatch by extension (.obj / .glb / .gltf).
    static Result<ImportModel> Import(const std::filesystem::path& path);

    static Result<ImportModel> ImportOBJ(const std::filesystem::path& path);
    static Result<ImportModel> ImportGLTF(const std::filesystem::path& path);
};

} // namespace nfsmw
