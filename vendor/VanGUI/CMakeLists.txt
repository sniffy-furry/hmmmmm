cmake_minimum_required(VERSION 3.28)
project(VanGUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Classic static library (header-based, broadest compiler compatibility)
# Uses the original .cpp implementation units directly — no module support required.
add_library(vangui STATIC
    vangui.cpp
    vangui_draw.cpp
    vangui_widgets.cpp
    vangui_tables.cpp
    vangui_docking.cpp
)
target_include_directories(vangui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(vangui PUBLIC cxx_std_23)

# C++23 Module target — enables: import vangui;
# Partition .ixx files are self-contained (interface + implementation merged).
# The primary vangui.ixx is a thin re-export wrapper; its implementation lives in vangui.cpp.
# Requires CMake 3.28+ and MSVC v143 (or Clang 17+ with -fmodules)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_CXX_STANDARD GREATER_EQUAL 23)
    add_library(vangui_module STATIC)
    target_sources(vangui_module
        PUBLIC
            FILE_SET CXX_MODULES FILES
                vangui.ixx              # Primary interface — thin re-export wrapper
                vangui-core.ixx         # Core types, math, context API (thin interface partition)
                vangui-draw.ixx         # Draw lists, fonts — thin interface partition
                vangui-tables.ixx       # Tables API — thin interface partition
                vangui-widgets.ixx      # Widgets API — thin interface partition
                vangui-docking.ixx      # Docking + viewport API — thin interface partition
        PRIVATE
                vangui.cpp              # Primary implementation unit (module vangui;)
                # Plain C++ TUs — NOT module units so symbols have standard external linkage.
                # The thin interface .ixx files export the names; the linker resolves them here.
                vangui-draw-impl.cpp
                vangui-tables-impl.cpp
                vangui-widgets-impl.cpp
                vangui-docking-impl.cpp
    )
    target_compile_features(vangui_module PUBLIC cxx_std_23)
    target_compile_definitions(vangui_module PRIVATE VANGUI_BUILD_AS_MODULE)
    set_target_properties(vangui_module PROPERTIES
        CXX_SCAN_FOR_MODULES ON
    )
    target_include_directories(vangui_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    # PCH for module partition files (draws vangui.h/vangui_internal.h into global
    # fragments without requiring explicit #include in each thin interface .ixx).
    target_precompile_headers(vangui_module PRIVATE vangui_module_pch.h)
    # vangui.cpp has module; at line ~1236 (not at the very start of the file).
    # The PCH would be injected before the file content, causing C7577 (global
    # module fragment not at start of TU). Opt this file out of the PCH; it has
    # its own explicit #include "vangui.h" / vangui_internal.h in its global fragment.
    set_source_files_properties(vangui.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endif()

# Test program — verifies import vangui; works end-to-end
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_CXX_STANDARD GREATER_EQUAL 23)
    add_subdirectory(Test)
endif()

# Optional: FreeType integration
option(VANGUI_FREETYPE "Build with FreeType font rasterizer" OFF)
if(VANGUI_FREETYPE)
    find_package(Freetype REQUIRED)
    target_sources(vangui PRIVATE misc/freetype/vangui_freetype.cpp)
    target_link_libraries(vangui PUBLIC Freetype::Freetype)
    target_compile_definitions(vangui PUBLIC VANGUI_ENABLE_FREETYPE)
    # Also wire FreeType into the module target if it was built
    if(TARGET vangui_module)
        target_sources(vangui_module PRIVATE misc/freetype/vangui_freetype.cpp)
        target_link_libraries(vangui_module PUBLIC Freetype::Freetype)
        target_compile_definitions(vangui_module PUBLIC VANGUI_ENABLE_FREETYPE)
    endif()
endif()

# ============================================================
# Backends
# ============================================================
# Each backend is an opt-in STATIC library. Link your application
# against vangui plus whichever backend libraries you need.
#
# Renderer backends (need a matching Platform backend):
#   vangui_backend_dx9          -DVANGUI_BACKEND_DX9=ON
#   vangui_backend_dx10         -DVANGUI_BACKEND_DX10=ON
#   vangui_backend_dx11         -DVANGUI_BACKEND_DX11=ON
#   vangui_backend_dx12         -DVANGUI_BACKEND_DX12=ON
#   vangui_backend_opengl2      -DVANGUI_BACKEND_OPENGL2=ON
#   vangui_backend_opengl3      -DVANGUI_BACKEND_OPENGL3=ON
#   vangui_backend_vulkan       -DVANGUI_BACKEND_VULKAN=ON
#   vangui_backend_sdlrenderer2 -DVANGUI_BACKEND_SDLRENDERER2=ON
#   vangui_backend_sdlrenderer3 -DVANGUI_BACKEND_SDLRENDERER3=ON
#   vangui_backend_null         -DVANGUI_BACKEND_NULL=ON          (headless/testing)
#   vangui_backend_allegro5     -DVANGUI_BACKEND_ALLEGRO5=ON      (renderer+platform)
#   vangui_backend_metal        -DVANGUI_BACKEND_METAL=ON         (Apple only)
#   vangui_backend_wgpu         -DVANGUI_BACKEND_WGPU=ON
#   vangui_backend_sdlgpu3      -DVANGUI_BACKEND_SDLGPU3=ON
#
# Platform backends (need a matching Renderer backend):
#   vangui_backend_win32        -DVANGUI_BACKEND_WIN32=ON
#   vangui_backend_glfw         -DVANGUI_BACKEND_GLFW=ON
#   vangui_backend_sdl2         -DVANGUI_BACKEND_SDL2=ON
#   vangui_backend_sdl3         -DVANGUI_BACKEND_SDL3=ON
#   vangui_backend_glut         -DVANGUI_BACKEND_GLUT=ON
#   vangui_backend_android      -DVANGUI_BACKEND_ANDROID=ON       (Android only)
#   vangui_backend_osx          -DVANGUI_BACKEND_OSX=ON           (Apple only)
# ============================================================

set(VANGUI_BACKENDS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/backends)

# Internal helper — creates a STATIC backend library that automatically
# inherits the VanGUI include paths and links the core vangui library.
function(vangui_add_backend target_name)
    add_library(${target_name} STATIC ${ARGN})
    target_include_directories(${target_name}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${VANGUI_BACKENDS_DIR}
    )
    target_link_libraries(${target_name} PUBLIC vangui)
    target_compile_features(${target_name} PUBLIC cxx_std_23)
endfunction()

# ---- DirectX / Win32 (Windows only) ------------------------
if(WIN32)
    option(VANGUI_BACKEND_DX9   "Build DirectX 9 renderer backend"   OFF)
    option(VANGUI_BACKEND_DX10  "Build DirectX 10 renderer backend"  OFF)
    option(VANGUI_BACKEND_DX11  "Build DirectX 11 renderer backend"  OFF)
    option(VANGUI_BACKEND_DX12  "Build DirectX 12 renderer backend"  OFF)
    option(VANGUI_BACKEND_WIN32 "Build Win32 platform backend"       OFF)

    if(VANGUI_BACKEND_DX9)
        vangui_add_backend(vangui_backend_dx9
            ${VANGUI_BACKENDS_DIR}/vangui_impl_dx9.cpp)
        target_link_libraries(vangui_backend_dx9 PRIVATE d3d9 d3dcompiler)
    endif()

    if(VANGUI_BACKEND_DX10)
        vangui_add_backend(vangui_backend_dx10
            ${VANGUI_BACKENDS_DIR}/vangui_impl_dx10.cpp)
        target_link_libraries(vangui_backend_dx10 PRIVATE d3d10 dxgi d3dcompiler)
    endif()

    if(VANGUI_BACKEND_DX11)
        vangui_add_backend(vangui_backend_dx11
            ${VANGUI_BACKENDS_DIR}/vangui_impl_dx11.cpp)
        target_link_libraries(vangui_backend_dx11 PRIVATE d3d11 dxgi d3dcompiler)
    endif()

    if(VANGUI_BACKEND_DX12)
        vangui_add_backend(vangui_backend_dx12
            ${VANGUI_BACKENDS_DIR}/vangui_impl_dx12.cpp)
        target_link_libraries(vangui_backend_dx12 PRIVATE d3d12 dxgi d3dcompiler)
    endif()

    if(VANGUI_BACKEND_WIN32)
        vangui_add_backend(vangui_backend_win32
            ${VANGUI_BACKENDS_DIR}/vangui_impl_win32.cpp)
        # Win32 API is implicitly available on every Windows target — no extra libs needed.
    endif()
endif()

# ---- OpenGL 2 (legacy fixed-function) ----------------------
option(VANGUI_BACKEND_OPENGL2 "Build OpenGL 2 (legacy) renderer backend" OFF)
if(VANGUI_BACKEND_OPENGL2)
    find_package(OpenGL REQUIRED)
    vangui_add_backend(vangui_backend_opengl2
        ${VANGUI_BACKENDS_DIR}/vangui_impl_opengl2.cpp)
    target_link_libraries(vangui_backend_opengl2 PUBLIC OpenGL::GL)
endif()

# ---- OpenGL 3 / ES 2+3 -------------------------------------
option(VANGUI_BACKEND_OPENGL3 "Build OpenGL 3+ / ES renderer backend" OFF)
if(VANGUI_BACKEND_OPENGL3)
    find_package(OpenGL REQUIRED)
    vangui_add_backend(vangui_backend_opengl3
        ${VANGUI_BACKENDS_DIR}/vangui_impl_opengl3.cpp)
    target_link_libraries(vangui_backend_opengl3 PUBLIC OpenGL::GL)
    # vangui_impl_opengl3_loader.h is in backends/ — already on the include path.
endif()

# ---- Vulkan ------------------------------------------------
option(VANGUI_BACKEND_VULKAN "Build Vulkan renderer backend" OFF)
if(VANGUI_BACKEND_VULKAN)
    find_package(Vulkan REQUIRED)
    vangui_add_backend(vangui_backend_vulkan
        ${VANGUI_BACKENDS_DIR}/vangui_impl_vulkan.cpp)
    target_link_libraries(vangui_backend_vulkan PUBLIC Vulkan::Vulkan)
endif()

# ---- GLFW --------------------------------------------------
option(VANGUI_BACKEND_GLFW "Build GLFW 3.x platform backend" OFF)
if(VANGUI_BACKEND_GLFW)
    find_package(glfw3 3.3 REQUIRED)
    vangui_add_backend(vangui_backend_glfw
        ${VANGUI_BACKENDS_DIR}/vangui_impl_glfw.cpp)
    target_link_libraries(vangui_backend_glfw PUBLIC glfw)
endif()

# ---- SDL2 (platform + renderer) ----------------------------
option(VANGUI_BACKEND_SDL2         "Build SDL2 platform backend"          OFF)
option(VANGUI_BACKEND_SDLRENDERER2 "Build SDL2 renderer backend"          OFF)

if(VANGUI_BACKEND_SDL2 OR VANGUI_BACKEND_SDLRENDERER2)
    find_package(SDL2 REQUIRED)
endif()

if(VANGUI_BACKEND_SDL2)
    vangui_add_backend(vangui_backend_sdl2
        ${VANGUI_BACKENDS_DIR}/vangui_impl_sdl2.cpp)
    target_link_libraries(vangui_backend_sdl2 PUBLIC SDL2::SDL2)
endif()

if(VANGUI_BACKEND_SDLRENDERER2)
    vangui_add_backend(vangui_backend_sdlrenderer2
        ${VANGUI_BACKENDS_DIR}/vangui_impl_sdlrenderer2.cpp)
    target_link_libraries(vangui_backend_sdlrenderer2 PUBLIC SDL2::SDL2)
endif()

# ---- SDL3 (platform + renderer) ----------------------------
option(VANGUI_BACKEND_SDL3         "Build SDL3 platform backend"          OFF)
option(VANGUI_BACKEND_SDLRENDERER3 "Build SDL3 renderer backend"          OFF)

if(VANGUI_BACKEND_SDL3 OR VANGUI_BACKEND_SDLRENDERER3)
    find_package(SDL3 REQUIRED)
endif()

if(VANGUI_BACKEND_SDL3)
    vangui_add_backend(vangui_backend_sdl3
        ${VANGUI_BACKENDS_DIR}/vangui_impl_sdl3.cpp)
    target_link_libraries(vangui_backend_sdl3 PUBLIC SDL3::SDL3)
endif()

if(VANGUI_BACKEND_SDLRENDERER3)
    vangui_add_backend(vangui_backend_sdlrenderer3
        ${VANGUI_BACKENDS_DIR}/vangui_impl_sdlrenderer3.cpp)
    target_link_libraries(vangui_backend_sdlrenderer3 PUBLIC SDL3::SDL3)
endif()

# ---- Null / headless renderer (testing) --------------------
option(VANGUI_BACKEND_NULL "Build null (headless) renderer backend" OFF)
if(VANGUI_BACKEND_NULL)
    vangui_add_backend(vangui_backend_null
        ${VANGUI_BACKENDS_DIR}/vangui_impl_null.cpp)
endif()

# ---- GLUT / FreeGLUT ---------------------------------------
option(VANGUI_BACKEND_GLUT "Build GLUT/FreeGLUT platform backend" OFF)
if(VANGUI_BACKEND_GLUT)
    find_package(GLUT REQUIRED)
    vangui_add_backend(vangui_backend_glut
        ${VANGUI_BACKENDS_DIR}/vangui_impl_glut.cpp)
    target_link_libraries(vangui_backend_glut PUBLIC GLUT::GLUT)
endif()

# ---- Allegro 5 ---------------------------------------------
option(VANGUI_BACKEND_ALLEGRO5 "Build Allegro5 renderer+platform backend" OFF)
if(VANGUI_BACKEND_ALLEGRO5)
    find_package(Allegro5 REQUIRED)
    vangui_add_backend(vangui_backend_allegro5
        ${VANGUI_BACKENDS_DIR}/vangui_impl_allegro5.cpp)
    target_link_libraries(vangui_backend_allegro5 PUBLIC allegro allegro_main allegro_primitives)
endif()

# ---- Android (Android platform only) -----------------------
if(ANDROID)
    option(VANGUI_BACKEND_ANDROID "Build Android platform backend" OFF)
    if(VANGUI_BACKEND_ANDROID)
        vangui_add_backend(vangui_backend_android
            ${VANGUI_BACKENDS_DIR}/vangui_impl_android.cpp)
        target_link_libraries(vangui_backend_android PRIVATE android log)
    endif()
endif()

# ---- Apple platforms (macOS AppKit + Metal) ----------------
if(APPLE)
    option(VANGUI_BACKEND_OSX   "Build macOS (AppKit) platform backend" OFF)
    option(VANGUI_BACKEND_METAL "Build Metal renderer backend"          OFF)

    if(VANGUI_BACKEND_OSX)
        vangui_add_backend(vangui_backend_osx
            ${VANGUI_BACKENDS_DIR}/vangui_impl_osx.mm)
        target_link_libraries(vangui_backend_osx PUBLIC
            "-framework AppKit"
            "-framework GameController"
            "-framework CoreHaptics")
    endif()

    if(VANGUI_BACKEND_METAL)
        vangui_add_backend(vangui_backend_metal
            ${VANGUI_BACKENDS_DIR}/vangui_impl_metal.mm)
        target_link_libraries(vangui_backend_metal PUBLIC
            "-framework Metal"
            "-framework MetalKit"
            "-framework QuartzCore")
    endif()
endif()

# ---- WebGPU (Dawn or wgpu-native) --------------------------
option(VANGUI_BACKEND_WGPU "Build WebGPU renderer backend" OFF)
if(VANGUI_BACKEND_WGPU)
    # Requires Dawn or wgpu-native headers on the include path.
    # Set WGPU_INCLUDE_DIR if headers are not in a standard location.
    vangui_add_backend(vangui_backend_wgpu
        ${VANGUI_BACKENDS_DIR}/vangui_impl_wgpu.cpp)
    if(DEFINED WGPU_INCLUDE_DIR)
        target_include_directories(vangui_backend_wgpu PRIVATE ${WGPU_INCLUDE_DIR})
    endif()
    if(DEFINED WGPU_LIB)
        target_link_libraries(vangui_backend_wgpu PRIVATE ${WGPU_LIB})
    endif()
endif()

# ---- SDL3 GPU renderer -------------------------------------
option(VANGUI_BACKEND_SDLGPU3 "Build SDL3 GPU renderer backend" OFF)
if(VANGUI_BACKEND_SDLGPU3)
    if(NOT VANGUI_BACKEND_SDL3 AND NOT VANGUI_BACKEND_SDLRENDERER3)
        find_package(SDL3 REQUIRED)
    endif()
    vangui_add_backend(vangui_backend_sdlgpu3
        ${VANGUI_BACKENDS_DIR}/vangui_impl_sdlgpu3.cpp)
    target_link_libraries(vangui_backend_sdlgpu3 PUBLIC SDL3::SDL3)
endif()

# ============================================================
# Misc extensions
# ============================================================
# Self-contained opt-in modules that add features beyond ImGui.
# None of these touch the audited core source files.
#
# Header-only (just include the header — no target needed):
#   misc/cpp/vangui_stdlib.h    native std::string InputText overloads
#
# Compiled modules (link the target to get the implementation):
#   vangui_anim                 -DVANGUI_MISC_ANIM=ON   (Pillar 1 substrate)
#   vangui_loading              -DVANGUI_MISC_LOADING=ON         (implies ANIM)
#   vangui_notify               -DVANGUI_MISC_NOTIFY=ON
#   vangui_command_palette      -DVANGUI_MISC_COMMAND_PALETTE=ON
#   vangui_themes               -DVANGUI_MISC_THEMES=ON
#   vangui_node_graph           -DVANGUI_MISC_NODE_GRAPH=ON
#   vangui_layout               -DVANGUI_MISC_LAYOUT=ON
#   vangui_dialogs              -DVANGUI_MISC_DIALOGS=ON          (implies ANIM)
#   vangui_style_sheet          -DVANGUI_MISC_STYLESHEET=ON      (implies theme engine)
#   vangui_signals              -DVANGUI_MISC_SIGNALS=ON          (header-only)
#   vangui_views                -DVANGUI_MISC_VIEWS=ON
#
# Extended widgets & app modules (van.h exposes wrappers when linked):
#   vangui_widgets_ext          -DVANGUI_MISC_WIDGETS_EXT=ON
#   vangui_charts               -DVANGUI_MISC_CHARTS=ON
#   vangui_feedback             -DVANGUI_MISC_FEEDBACK=ON
#   vangui_forms                -DVANGUI_MISC_FORMS=ON
#   vangui_panels               -DVANGUI_MISC_PANELS=ON
#   vangui_shortcuts            -DVANGUI_MISC_SHORTCUTS=ON
#   vangui_vui                  -DVANGUI_MISC_VUI=ON
#
# Theme Engine (Phase B):
#   vangui_theme_engine         -DVANGUI_MISC_THEME_ENGINE=ON
#   vangui_theme_editor         -DVANGUI_MISC_THEME_EDITOR=ON
#     (theme_editor depends on theme_engine and vangui_themes)
# ============================================================

set(VANGUI_MISC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/misc)

# Internal helper — same as vangui_add_backend but for misc modules.
function(vangui_add_misc target_name)
    add_library(${target_name} STATIC ${ARGN})
    target_include_directories(${target_name}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${VANGUI_MISC_DIR}
    )
    target_link_libraries(${target_name} PUBLIC vangui)
    target_compile_features(${target_name} PUBLIC cxx_std_23)
endfunction()

# ---- Animation & transition substrate (Pillar 1) -----------
# The shared easing/tween/spring substrate. Unlike the other misc modules,
# this one installs a single guarded hook in the core (VanGui::NewFrame), so
# enabling it defines VANGUI_ENABLE_ANIM on the core 'vangui' target as well.
# With the option OFF, the core is byte-identical to upstream and the module
# compiles to an empty translation unit.
option(VANGUI_MISC_ANIM "Build animation/transition substrate (Pillar 1)" OFF)
if(VANGUI_MISC_ANIM)
    vangui_add_misc(vangui_anim
        ${VANGUI_MISC_DIR}/vangui_anim.cpp)
    # Activate the substrate everywhere: the module's own TU, the core hook,
    # and every consumer that links the core.
    target_compile_definitions(vangui_anim PUBLIC VANGUI_ENABLE_ANIM)
    target_compile_definitions(vangui     PUBLIC VANGUI_ENABLE_ANIM)
endif()

# ---- Loading effects (Pillar 2) ----------------------------
# Spinners, skeletons, progress ring, busy overlay. Consumes Pillar 1.
option(VANGUI_MISC_LOADING "Build loading/busy-state effects (Pillar 2)" OFF)
if(VANGUI_MISC_LOADING)
    if(NOT VANGUI_MISC_ANIM)
        message(FATAL_ERROR "VANGUI_MISC_LOADING requires VANGUI_MISC_ANIM (Pillar 2 depends on Pillar 1).")
    endif()
    vangui_add_misc(vangui_loading
        ${VANGUI_MISC_DIR}/vangui_loading.cpp)
    target_compile_definitions(vangui_loading PUBLIC VANGUI_ENABLE_LOADING)
endif()

# ---- Toast / notification overlay --------------------------
option(VANGUI_MISC_NOTIFY "Build toast notification system" OFF)
if(VANGUI_MISC_NOTIFY)
    vangui_add_misc(vangui_notify
        ${VANGUI_MISC_DIR}/vangui_notify.cpp)
    target_compile_definitions(vangui_notify PUBLIC VANGUI_ENABLE_NOTIFY)
endif()

# ---- Command palette (Ctrl+K) ------------------------------
option(VANGUI_MISC_COMMAND_PALETTE "Build Ctrl+K command palette widget" OFF)
if(VANGUI_MISC_COMMAND_PALETTE)
    vangui_add_misc(vangui_command_palette
        ${VANGUI_MISC_DIR}/vangui_command_palette.cpp)
    target_compile_definitions(vangui_command_palette PUBLIC VANGUI_ENABLE_COMMAND_PALETTE)
endif()

# ---- Named theme presets + serialization -------------------
option(VANGUI_MISC_THEMES "Build named theme presets and serialization" OFF)
if(VANGUI_MISC_THEMES)
    vangui_add_misc(vangui_themes
        ${VANGUI_MISC_DIR}/vangui_themes.cpp)
    target_compile_definitions(vangui_themes PUBLIC VANGUI_ENABLE_THEMES)
endif()

# ---- Node graph widget -------------------------------------
option(VANGUI_MISC_NODE_GRAPH "Build node graph / node editor widget" OFF)
if(VANGUI_MISC_NODE_GRAPH)
    vangui_add_misc(vangui_node_graph
        ${VANGUI_MISC_DIR}/vangui_node_graph.cpp)
    target_compile_definitions(vangui_node_graph PUBLIC VANGUI_ENABLE_NODE_GRAPH)
endif()

# ---- Layout helpers (Pillar 3) -----------------------------
# HBox/VBox/Grid/Stretch. Pure cursor math, no core dependency.
option(VANGUI_MISC_LAYOUT "Build layout helpers (Pillar 3)" OFF)
if(VANGUI_MISC_LAYOUT)
    vangui_add_misc(vangui_layout ${VANGUI_MISC_DIR}/vangui_layout.cpp)
    target_compile_definitions(vangui_layout PUBLIC VANGUI_ENABLE_LAYOUT)
endif()

# ---- Standard dialogs (Pillar 3) ---------------------------
# MessageBox + file pickers, animated via Pillar 1, std::expected results.
option(VANGUI_MISC_DIALOGS "Build standard dialogs (Pillar 3)" OFF)
if(VANGUI_MISC_DIALOGS)
    if(NOT VANGUI_MISC_ANIM)
        message(FATAL_ERROR "VANGUI_MISC_DIALOGS requires VANGUI_MISC_ANIM.")
    endif()
    vangui_add_misc(vangui_dialogs ${VANGUI_MISC_DIR}/vangui_dialogs.cpp)
    target_compile_definitions(vangui_dialogs PUBLIC VANGUI_ENABLE_DIALOGS)
endif()

# ---- Stylesheet (.vss) front-end (Pillar 3) ----------------
# Declarative theming over the theme engine. Implies the theme engine.
option(VANGUI_MISC_STYLESHEET "Build .vss stylesheet front-end (Pillar 3)" OFF)
if(VANGUI_MISC_STYLESHEET)
    if(NOT VANGUI_MISC_THEME_ENGINE)
        message(WARNING "VANGUI_MISC_STYLESHEET depends on VANGUI_MISC_THEME_ENGINE. Enabling it.")
        set(VANGUI_MISC_THEME_ENGINE ON CACHE BOOL "" FORCE)
    endif()
    vangui_add_misc(vangui_style_sheet ${VANGUI_MISC_DIR}/vangui_style_sheet.cpp)
    target_compile_definitions(vangui_style_sheet PUBLIC VANGUI_ENABLE_STYLESHEET)
    if(TARGET vangui_theme_engine)
        target_link_libraries(vangui_style_sheet PUBLIC vangui_theme_engine)
    endif()
endif()

# ---- Signals / slots (Pillar 3, header-only) ---------------
option(VANGUI_MISC_SIGNALS "Enable header-only signals/slots (Pillar 3)" OFF)
if(VANGUI_MISC_SIGNALS)
    add_library(vangui_signals INTERFACE)
    target_include_directories(vangui_signals INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${VANGUI_MISC_DIR})
    target_compile_features(vangui_signals INTERFACE cxx_std_23)
    target_compile_definitions(vangui_signals INTERFACE VANGUI_ENABLE_SIGNALS)
endif()

# ---- Model/View adapters (Pillar 3) ------------------------
option(VANGUI_MISC_VIEWS "Build model/view list & tree adapters (Pillar 3)" OFF)
if(VANGUI_MISC_VIEWS)
    vangui_add_misc(vangui_views ${VANGUI_MISC_DIR}/vangui_views.cpp)
    target_compile_definitions(vangui_views PUBLIC VANGUI_ENABLE_VIEWS)
endif()

# ===========================================================================
# Extended widget & application modules (the van.h facade exposes wrappers for
# these when their VANGUI_ENABLE_* macro is defined — i.e. when you link them).
# The ones that animate use the anim substrate; when VANGUI_MISC_ANIM is OFF the
# anim header shims snap instantly, so ANIM is an optimization, not a hard dep.
# ===========================================================================

# ---- Extended widgets (toggle/segmented/chip/badge/rating/...) -
option(VANGUI_MISC_WIDGETS_EXT "Build extended widgets (toggle, segmented, chip, rating, search, ...)" OFF)
if(VANGUI_MISC_WIDGETS_EXT)
    vangui_add_misc(vangui_widgets_ext ${VANGUI_MISC_DIR}/vangui_widgets_ext.cpp)
    target_compile_definitions(vangui_widgets_ext PUBLIC VANGUI_ENABLE_WIDGETS_EXT)
    if(TARGET vangui_anim)
        target_link_libraries(vangui_widgets_ext PUBLIC vangui_anim)
    endif()
endif()

# ---- Charts (sparkline / bar chart / gauge) ----------------
option(VANGUI_MISC_CHARTS "Build lightweight charts (sparkline, bar chart, gauge)" OFF)
if(VANGUI_MISC_CHARTS)
    vangui_add_misc(vangui_charts ${VANGUI_MISC_DIR}/vangui_charts.cpp)
    target_compile_definitions(vangui_charts PUBLIC VANGUI_ENABLE_CHARTS)
endif()

# ---- Feedback widgets (animated value / ripple / elevated) -
option(VANGUI_MISC_FEEDBACK "Build feedback widgets (animated value, ripple/elevated buttons)" OFF)
if(VANGUI_MISC_FEEDBACK)
    vangui_add_misc(vangui_feedback ${VANGUI_MISC_DIR}/vangui_feedback.cpp)
    target_compile_definitions(vangui_feedback PUBLIC VANGUI_ENABLE_FEEDBACK)
    if(TARGET vangui_anim)
        target_link_libraries(vangui_feedback PUBLIC vangui_anim)
    endif()
endif()

# ---- Forms (aligned label/field rows + validation) ---------
option(VANGUI_MISC_FORMS "Build form layout + inline validation helpers" OFF)
if(VANGUI_MISC_FORMS)
    vangui_add_misc(vangui_forms ${VANGUI_MISC_DIR}/vangui_forms.cpp)
    target_compile_definitions(vangui_forms PUBLIC VANGUI_ENABLE_FORMS)
endif()

# ---- Panels (splitter / accordion / status bar / toolbar) --
option(VANGUI_MISC_PANELS "Build application chrome (splitter, accordion, status bar, toolbar)" OFF)
if(VANGUI_MISC_PANELS)
    vangui_add_misc(vangui_panels ${VANGUI_MISC_DIR}/vangui_panels.cpp)
    target_compile_definitions(vangui_panels PUBLIC VANGUI_ENABLE_PANELS)
    if(TARGET vangui_anim)
        target_link_libraries(vangui_panels PUBLIC vangui_anim)
    endif()
endif()

# ---- Keyboard shortcut registry ----------------------------
# Optionally drives the command palette when that module is also enabled.
option(VANGUI_MISC_SHORTCUTS "Build the keyboard shortcut registry" OFF)
if(VANGUI_MISC_SHORTCUTS)
    vangui_add_misc(vangui_shortcuts ${VANGUI_MISC_DIR}/vangui_shortcuts.cpp)
    target_compile_definitions(vangui_shortcuts PUBLIC VANGUI_ENABLE_SHORTCUTS)
    if(TARGET vangui_command_palette)
        target_link_libraries(vangui_shortcuts PUBLIC vangui_command_palette)
    endif()
endif()

# ---- .vui declarative UI loader ----------------------------
# Testable parser (vangui_vui_parser.inl) + renderer over the core widgets.
option(VANGUI_MISC_VUI "Build the .vui declarative UI loader (parser + renderer)" OFF)
if(VANGUI_MISC_VUI)
    vangui_add_misc(vangui_vui ${VANGUI_MISC_DIR}/vangui_vui.cpp)
    target_compile_definitions(vangui_vui PUBLIC VANGUI_ENABLE_VUI)
endif()

# ---- Fluent facade (van.h, header-only) --------------------
# One-include ergonomic layer (`van::`): RAII scopes, fluent widgets,
# designated-init option structs. Header-only; just put the repo root on the
# include path (the core `vangui` target already does). This INTERFACE target
# is a convenience so you can `target_link_libraries(app PRIVATE vangui::van)`.
option(VANGUI_FACADE "Expose the van.h fluent facade as an interface target" ON)
if(VANGUI_FACADE)
    add_library(vangui_van INTERFACE)
    add_library(vangui::van ALIAS vangui_van)
    target_include_directories(vangui_van INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(vangui_van INTERFACE cxx_std_23)
    if(TARGET vangui)
        target_link_libraries(vangui_van INTERFACE vangui)
    endif()
endif()

# ---- Theme Engine (Phase B) --------------------------------
# Semantic token system, animated transitions, per-scope overrides, hot-reload.
option(VANGUI_MISC_THEME_ENGINE "Build theme engine (semantic tokens + transitions)" OFF)
if(VANGUI_MISC_THEME_ENGINE)
    # theme_engine depends on vangui_themes for LoadThemeFromFile / SaveThemeToMemory.
    if(NOT VANGUI_MISC_THEMES)
        message(WARNING "VANGUI_MISC_THEME_ENGINE depends on VANGUI_MISC_THEMES. "
                        "Enabling VANGUI_MISC_THEMES automatically.")
        set(VANGUI_MISC_THEMES ON CACHE BOOL "" FORCE)
        vangui_add_misc(vangui_themes
            ${VANGUI_MISC_DIR}/vangui_themes.cpp)
    endif()
    vangui_add_misc(vangui_theme_engine
        ${VANGUI_MISC_DIR}/vangui_theme_engine.cpp)
    target_compile_definitions(vangui_theme_engine PUBLIC VANGUI_ENABLE_THEME_ENGINE)
    target_link_libraries(vangui_theme_engine PUBLIC vangui_themes)
endif()

# ---- Thread pool + main-thread dispatch --------------------
# Provides InitThreadPool / ShutdownThreadPool, PostToMainThread, Async,
# AsyncFuture<T>, and VanFuture<T>. Integrates with NewFrameExtras() (drain)
# and EnhanceWantsRedraw() (wake backend when callbacks are pending).
# Also activates VanSignal::connect_queued() for cross-thread signals.
option(VANGUI_MISC_THREAD "Build thread pool and main-thread dispatch" OFF)
if(VANGUI_MISC_THREAD)
    find_package(Threads REQUIRED)
    vangui_add_misc(vangui_thread ${VANGUI_MISC_DIR}/vangui_thread.cpp)
    target_compile_definitions(vangui_thread PUBLIC VANGUI_ENABLE_THREAD)
    target_link_libraries(vangui_thread PUBLIC Threads::Threads)
    # Thread-safe signals activate automatically when both modules are linked.
    if(TARGET vangui_signals)
        target_compile_definitions(vangui_signals PUBLIC VANGUI_ENABLE_THREAD)
        target_link_libraries(vangui_signals INTERFACE vangui_thread)
    endif()
endif()

# ---- Theme Editor widget (Phase B) -------------------------
# Interactive in-app color/token editor with preset picker and file I/O.
option(VANGUI_MISC_THEME_EDITOR "Build theme editor widget" OFF)
if(VANGUI_MISC_THEME_EDITOR)
    if(NOT VANGUI_MISC_THEME_ENGINE)
        message(FATAL_ERROR "VANGUI_MISC_THEME_EDITOR requires VANGUI_MISC_THEME_ENGINE=ON")
    endif()
    vangui_add_misc(vangui_theme_editor
        ${VANGUI_MISC_DIR}/vangui_theme_editor.cpp)
    target_compile_definitions(vangui_theme_editor PUBLIC VANGUI_ENABLE_THEME_EDITOR)
    target_link_libraries(vangui_theme_editor PUBLIC vangui_theme_engine)
endif()
