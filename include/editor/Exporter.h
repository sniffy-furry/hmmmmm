#pragma once
#include "Common.h"
#include "formats/SolidList.h"
#include "formats/StreamBundle.h"
#include <filesystem>
#include <vector>

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// Object-export writer. Adapted from NFSMapEditor's editor/Exporter.h::ExportOBJ
// but reworked to operate directly on a StreamSection/SolidObject instead of
// the full editor Scene (this app has no scene graph).
//
// Two output formats ship and are both wired into ObjectExportPanel:
//   • OBJ+MTL — implemented here (Exporter.cpp), no external dependencies.
//   • glTF/.glb — fully implemented in src/editor/GLTFExporter.cpp (both
//     ExportObjectGLTF and ExportSectionGLTF): Z-up→Y-up conversion, embedded
//     PNG textures, and SimSurface collision metadata in glTF `extras`.
// ─────────────────────────────────────────────────────────────────────────────
class Exporter {
public:
    struct ExportOptions {
        bool applyWorldTransform = true;  ///< bake SolidObject::transform into verts
        bool exportTextures      = true;  ///< write referenced textures as .dds
        /// Subdirectory (relative to the output path) for dumped textures.
        /// Empty = write textures next to the model file.
        std::string textureSubdir = "textures";
    };

    struct ExportReport {
        size_t objectsExported  = 0;
        size_t verticesWritten  = 0;
        size_t trianglesWritten = 0;
        size_t materialsWritten = 0;
        size_t texturesWritten  = 0;
        std::vector<std::string> warnings;
    };

    /// Export a single SolidObject to Wavefront OBJ + a sibling .mtl file.
    /// `section` supplies the TPKBlocks used to resolve `textureHashes` to
    /// pixel data for the texture dump. `extraTPKs` (optional) are searched
    /// as a fallback (e.g. GLOBAL texture packs not in this section).
    static Result<ExportReport> ExportObjectOBJ(const SolidObject& obj,
                                                  const StreamSection& section,
                                                  const std::filesystem::path& objPath,
                                                  const ExportOptions& options,
                                                  const std::vector<TPKBlock>* extraTPKs = nullptr);

    /// Export every object in `section` to one OBJ+MTL (optionally only
    /// objects from `lists`, empty = all solid lists in the section).
    static Result<ExportReport> ExportSectionOBJ(const StreamSection& section,
                                                   const std::filesystem::path& objPath,
                                                   const ExportOptions& options,
                                                   const std::vector<TPKBlock>* extraTPKs = nullptr);

    /// Export a single SolidObject to a self-contained binary glTF (.glb):
    /// geometry + PBR materials + embedded PNG textures (DXT is decoded to RGBA
    /// and re-encoded as PNG). Each material carries its SimSurface collision
    /// surface type in its name and in glTF `extras.simSurface`. Coordinates are
    /// converted from the game's Z-up to glTF's Y-up so the model lands upright
    /// in Blender / 3ds Max. Imports directly via "File > Import > glTF 2.0".
    static Result<ExportReport> ExportObjectGLTF(const SolidObject& obj,
                                                  const StreamSection& section,
                                                  const std::filesystem::path& glbPath,
                                                  const ExportOptions& options,
                                                  const std::vector<TPKBlock>* extraTPKs = nullptr);

    /// Export every object in `section` to one self-contained .glb.
    static Result<ExportReport> ExportSectionGLTF(const StreamSection& section,
                                                   const std::filesystem::path& glbPath,
                                                   const ExportOptions& options,
                                                   const std::vector<TPKBlock>* extraTPKs = nullptr);
};

} // namespace nfsmw
