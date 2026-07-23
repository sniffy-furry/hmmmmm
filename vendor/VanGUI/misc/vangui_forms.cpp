// vangui_forms.cpp — see vangui_forms.h. Empty TU unless enabled.
#include "vangui_forms.h"

#ifdef VANGUI_ENABLE_FORMS

namespace VanGui {

namespace {
struct FormState { float label_w; };
constexpr int kMaxForms = 8;
static FormState s_stack[kMaxForms];
static int       s_depth = 0;
inline float CurLabelW() { return s_depth > 0 ? s_stack[s_depth - 1].label_w : 120.0f; }
}

void BeginForm(const char* id, float label_width)
{
    PushID(id);
    if (label_width <= 0.0f) label_width = GetContentRegionAvail().x * 0.35f;
    if (s_depth < kMaxForms) s_stack[s_depth++] = { label_width };
}

void FormRow(const char* label)
{
    AlignTextToFramePadding();
    TextUnformatted(label);
    SameLine(CurLabelW());
    SetNextItemWidth(-1.0f);
}

void EndForm()
{
    if (s_depth > 0) --s_depth;
    PopID();
}

void FieldError(const char* msg)
{
    const float indent = CurLabelW();
    Dummy(VanVec2(indent, 0));
    SameLine(indent);
    TextColored(VanVec4(0.92f, 0.34f, 0.30f, 1.0f), "%s", msg);
}

void FieldHint(const char* msg)
{
    const float indent = CurLabelW();
    Dummy(VanVec2(indent, 0));
    SameLine(indent);
    TextDisabled("%s", msg);
}

void PushInvalid()
{
    const VanVec4 red(0.55f, 0.18f, 0.18f, 0.55f);
    const VanVec4 redB(0.92f, 0.34f, 0.30f, 1.0f);
    PushStyleColor(VanGuiCol_FrameBg, red);
    PushStyleColor(VanGuiCol_FrameBgHovered, red);
    PushStyleColor(VanGuiCol_Border, redB);
}
void PopInvalid() { PopStyleColor(3); }

} // namespace VanGui

#endif // VANGUI_ENABLE_FORMS
