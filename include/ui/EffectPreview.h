#pragma once
#include "formats/EmitterCatalog.h"
#include <vangui.h>
#include <string>

namespace nfsmw {

// Draw a looping, animated particle burst for an emitter family at screen
// (sx,sy) on the given draw list. Screen-space (y-down); loops on `t` so it
// plays continuously. Shared by the object viewer's inline preview and the
// dedicated Effects panel.
void EmitParticles(VanDrawList* dl, float sx, float sy, FxFamily fam,
                   const std::string& effect, float t, float sizeScale);

} // namespace nfsmw
