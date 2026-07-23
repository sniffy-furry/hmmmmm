# ImGui — Production-Grade Tool UI

## What It Is
Dear ImGui is an immediate-mode GUI library for C++. You rebuild the UI from scratch
every frame by calling draw functions — no retained widget tree. Ideal for developer
tools, overlays, and in-process inspectors wired directly to live runtime state.

---

## "Production Grade" Means
- Custom visual theme (no default grey ImGui chrome in shipped products)
- Consistent font rendering with icon glyphs (FontAwesome or custom atlas)
- Proper DPI handling
- Input focus and keyboard navigation working correctly
- No per-frame allocations in the render path

---

## Backend Selection

| Use Case | Backend | Notes |
|---|---|---|
| DirectX 11 in-process overlay | `imgui_impl_dx11` | Most common in Windows tooling |
| DirectX 12 | `imgui_impl_dx12` | More setup; use only if host uses DX12 |
| OpenGL (cross-platform tool) | `imgui_impl_opengl3` + `imgui_impl_glfw` | Good for standalone tools |
| Vulkan | `imgui_impl_vulkan` | Highest setup cost |
| Software-only (no GPU) | `imgui_impl_win32` + custom renderer | Useful for accessibility tools |

For in-process overlays, hook the host's present call (e.g., `IDXGISwapChain::Present`)
and call `ImGui_ImplDX11_NewFrame()` / `ImGui::NewFrame()` there each frame.

---

## Full Custom Theme

```cpp
void ApplyGadgetTheme() {
    ImGuiStyle& s = ImGui::GetStyle();

    // Geometry
    s.WindowRounding    = 8.0f;   s.ChildRounding  = 6.0f;
    s.FrameRounding     = 4.0f;   s.PopupRounding  = 6.0f;
    s.ScrollbarRounding = 4.0f;   s.GrabRounding   = 4.0f;
    s.TabRounding       = 4.0f;   s.WindowBorderSize = 1.0f;
    s.ItemSpacing       = { 8, 6 };
    s.FramePadding      = { 10, 5 };
    s.WindowPadding     = { 14, 12 };
    s.IndentSpacing     = 20.0f;
    s.ScrollbarSize     = 10.0f;

    // Palette
    auto* c = s.Colors;
    const ImVec4 bg0      = { 0.09f, 0.09f, 0.10f, 1.00f };
    const ImVec4 bg1      = { 0.12f, 0.12f, 0.14f, 1.00f };
    const ImVec4 bg2      = { 0.18f, 0.18f, 0.20f, 1.00f };
    const ImVec4 accent   = { 0.26f, 0.59f, 0.98f, 1.00f };
    const ImVec4 accentHv = { 0.36f, 0.68f, 1.00f, 1.00f };
    const ImVec4 accentAc = { 0.16f, 0.49f, 0.88f, 1.00f };
    const ImVec4 text     = { 0.90f, 0.90f, 0.92f, 1.00f };
    const ImVec4 textDim  = { 0.55f, 0.55f, 0.60f, 1.00f };
    const ImVec4 border   = { 0.28f, 0.28f, 0.32f, 1.00f };

    c[ImGuiCol_WindowBg]             = bg0;
    c[ImGuiCol_ChildBg]              = bg1;
    c[ImGuiCol_PopupBg]              = bg1;
    c[ImGuiCol_Border]               = border;
    c[ImGuiCol_FrameBg]              = bg2;
    c[ImGuiCol_FrameBgHovered]       = { bg2.x+0.04f, bg2.y+0.04f, bg2.z+0.04f, 1.f };
    c[ImGuiCol_FrameBgActive]        = accent;
    c[ImGuiCol_TitleBg]              = bg0;
    c[ImGuiCol_TitleBgActive]        = bg1;
    c[ImGuiCol_Button]               = bg2;
    c[ImGuiCol_ButtonHovered]        = accentHv;
    c[ImGuiCol_ButtonActive]         = accentAc;
    c[ImGuiCol_CheckMark]            = accent;
    c[ImGuiCol_SliderGrab]           = accent;
    c[ImGuiCol_SliderGrabActive]     = accentAc;
    c[ImGuiCol_Tab]                  = bg2;
    c[ImGuiCol_TabHovered]           = accentHv;
    c[ImGuiCol_TabActive]            = accent;
    c[ImGuiCol_TabUnfocused]         = bg1;
    c[ImGuiCol_TabUnfocusedActive]   = bg2;
    c[ImGuiCol_Separator]            = border;
    c[ImGuiCol_SeparatorHovered]     = accentHv;
    c[ImGuiCol_SeparatorActive]      = accent;
    c[ImGuiCol_Text]                 = text;
    c[ImGuiCol_TextDisabled]         = textDim;
    c[ImGuiCol_PlotLines]            = accent;
    c[ImGuiCol_PlotHistogram]        = accent;
}
```

---

## Font Atlas with Icon Glyphs

```cpp
void SetupFonts(ImGuiIO& io) {
    // Base proportional font
    ImFontConfig cfg;
    cfg.OversampleH = 3;  cfg.OversampleV = 2;
    io.Fonts->AddFontFromFileTTF("fonts/Inter-Regular.ttf", 15.0f, &cfg);

    // Merge FontAwesome 6 icon glyphs
    static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icon_cfg;
    icon_cfg.MergeMode   = true;
    icon_cfg.GlyphOffset = { 0, 2.0f };  // vertical alignment tweak
    icon_cfg.PixelSnapH  = true;
    io.Fonts->AddFontFromFileTTF("fonts/FontAwesome6-Solid.otf",
                                  14.0f, &icon_cfg, icon_ranges);
    io.Fonts->Build();

    // Monospace for code/hex displays
    io.Fonts->AddFontFromFileTTF("fonts/JetBrainsMono-Regular.ttf", 13.0f);
}

// Usage:
// ImGui::Text(ICON_FA_MAGNIFYING_GLASS " Search");
```

---

## DPI Awareness

```cpp
float GetDpiScale() {
    HDC hdc = GetDC(nullptr);
    float dpi = static_cast<float>(GetDeviceCaps(hdc, LOGPIXELSX));
    ReleaseDC(nullptr, hdc);
    return dpi / 96.0f;  // 96 DPI = 1.0 scale
}

void ScaleStyle(float scale) {
    ImGui::GetStyle().ScaleAllSizes(scale);
    // Rebuild font atlas at new pixel size
}
```

---

## Overlay Frame Pattern

```cpp
void RenderFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Full-viewport transparent overlay container
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin("##overlay", nullptr,
        ImGuiWindowFlags_NoTitleBar    |
        ImGuiWindowFlags_NoResize      |
        ImGuiWindowFlags_NoScrollbar   |
        ImGuiWindowFlags_NoInputs      |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus);

    // ... draw world-space annotations, lines, text ...

    ImGui::End();

    // Separate tool window that accepts input
    if (g_show_tool_panel)
        DrawToolPanel();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
```

---

## Performance Rules

- Pre-format strings with `snprintf` before `ImGui::Text` — the format path allocates per-frame
- Always pair `PushStyleColor` / `PopStyleColor` — never push without pop
- Use `ImGui::BeginDisabled(condition)` instead of conditional rendering for inactive widgets
- Profile with `ImGui::ShowMetricsWindow()` during development — watch DrawCalls and Vertices
