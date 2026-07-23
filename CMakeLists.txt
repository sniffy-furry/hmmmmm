cmake_minimum_required(VERSION 3.25)
project(NFSMWToolkit CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NFSMW_BUILD_GUI "Build the GUI editor (needs GLFW/VanGUI/glad)" ON)
option(NFSMW_BUILD_CLI "Build the headless mwcli verifier (no GUI deps)" ON)
option(NFSMW_BUILD_TESTS "Build the unit/round-trip test suite (no GUI deps)" ON)
option(NFSMW_BUILD_ANDROID_JNI "Build a lean JNI .so around nfsmw_core for a plain Kotlin UI (no VanGUI/GLFW/FFmpeg)" OFF)
option(NFSMW_ENABLE_VIDEO "Desktop GUI: build FFmpeg-based movie/video playback (VP6). OFF skips libav* entirely and uses the stub (no effect on 3D mesh preview)." ON)

# --- vcpkg integration -----------------------------------------------------
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# --- Output directories -----------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# --- Core dependencies (header-only / lightweight) --------------------------
find_package(glm CONFIG QUIET)
if(NOT TARGET glm::glm)
    find_path(GLM_INCLUDE_DIR glm/glm.hpp)
    if(NOT GLM_INCLUDE_DIR)
        message(FATAL_ERROR "glm not found (vcpkg: glm / apt: libglm-dev)")
    endif()
    add_library(glm_headers INTERFACE)
    target_include_directories(glm_headers INTERFACE ${GLM_INCLUDE_DIR})
    add_library(glm::glm ALIAS glm_headers)
endif()

find_package(spdlog CONFIG QUIET)
if(NOT TARGET spdlog::spdlog)
    find_path(SPDLOG_INCLUDE_DIR spdlog/spdlog.h)
    if(NOT SPDLOG_INCLUDE_DIR)
        message(FATAL_ERROR "spdlog not found (vcpkg: spdlog / apt: libspdlog-dev)")
    endif()
    add_library(spdlog_headers INTERFACE)
    target_include_directories(spdlog_headers INTERFACE ${SPDLOG_INCLUDE_DIR})
    add_library(spdlog::spdlog ALIAS spdlog_headers)
endif()

find_package(nlohmann_json CONFIG QUIET)
if(NOT TARGET nlohmann_json::nlohmann_json)
    find_path(NLOHMANN_INCLUDE_DIR nlohmann/json.hpp)
    if(NOT NLOHMANN_INCLUDE_DIR)
        message(FATAL_ERROR "nlohmann-json not found (vcpkg: nlohmann-json / apt: nlohmann-json3-dev)")
    endif()
    add_library(nlohmann_headers INTERFACE)
    target_include_directories(nlohmann_headers INTERFACE ${NLOHMANN_INCLUDE_DIR})
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_headers)
endif()

# --- nfsmw_core: file format parsers (no GL dependencies) -------------------
# Stays C++20 and GL/VanGUI-free (see CLAUDE.md key constraints).
set(CORE_SOURCES
    src/core/ChunkReader.cpp
    src/core/BINFile.cpp
    src/core/LZCDecompressor.cpp
    src/core/StringHash.cpp
    src/core/Logger.cpp
    src/core/KnownNames.cpp
    src/core/FlagReference.cpp
    src/core/ObjectClassifier.cpp
    src/core/VaultSchema.cpp
)

set(FORMAT_SOURCES
    src/formats/TPKBlock.cpp
    src/formats/SolidList.cpp
    src/formats/StreamBundle.cpp
    src/formats/WorldData.cpp
    src/formats/AudioBank.cpp
    src/formats/AudioCodecs.cpp
    src/formats/MUSFile.cpp
    src/formats/MPFFile.cpp
    src/formats/ABKFile.cpp
    src/formats/GINFile.cpp
    src/formats/HUDLayout.cpp
    src/formats/FontSheet.cpp
    src/formats/CarInfo.cpp
    src/formats/CarSlot.cpp
    src/formats/MinimapFile.cpp
    src/formats/VaultFile.cpp
    src/formats/VaultDecoder.cpp
    src/formats/AnimationBank.cpp
    src/formats/EmitterCatalog.cpp
    src/formats/NisAnim.cpp
    src/formats/NisEventSequence.cpp
)

# Phase 2: patch subsystem (BackupManager + BunRepacker)
set(PATCH_SOURCES
    src/patch/BackupManager.cpp
    src/patch/BunRepacker.cpp
    src/patch/SolidWriter.cpp
    src/patch/MwMod.cpp
)

add_library(nfsmw_core STATIC
    ${CORE_SOURCES}
    ${FORMAT_SOURCES}
    ${PATCH_SOURCES}
)

target_include_directories(nfsmw_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(nfsmw_core PUBLIC
    glm::glm
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

if(MSVC)
    target_compile_options(nfsmw_core PRIVATE
        /W4 /permissive-
        # Workaround for MSVC internal compiler error (C1001) in fmt/format.h
        # with heavy template instantiation (spdlog/fmt).
        $<$<CXX_COMPILER_ID:MSVC>:/d2SSAOptimizer->
    )
else()
    target_compile_options(nfsmw_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- nfsmw_jni: lean Android JNI bridge (no VanGUI/GLFW/FFmpeg) -------------
# Links only against nfsmw_core, same as mwcli. Meant to sit behind a plain
# Kotlin Activity (buttons, file picker, TextView) instead of a NativeActivity
# + immediate-mode renderer. Stays C++20 - no exceptions/RTTI flags needed
# beyond what nfsmw_core already uses.
if(NFSMW_BUILD_ANDROID_JNI AND ANDROID)
    add_library(nfsmw_jni SHARED
        src/android/jni_bridge.cpp
    )
    target_link_libraries(nfsmw_jni PRIVATE nfsmw_core log)
    target_include_directories(nfsmw_jni PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_options(nfsmw_jni PRIVATE -Wall -Wextra)
endif()

# ============================================================================
# VanGUI suite (replaces the stock Dear ImGui vendor copy).
#
# VanGUI is a C++23 fork of Dear ImGui. Its core + enhancement modules are
# compiled as their own STATIC libraries at C++23; only the headers are pulled
# into MWTools, so the GUI executable is bumped to C++23 too (nfsmw_core and
# mwcli stay C++20). All VanGUI include dirs are SYSTEM so /external:W0 keeps
# the vendor warning-clean per the project's zero-warning policy.
# ============================================================================
if(NFSMW_BUILD_GUI AND ANDROID)
    set(VANGUI_DIR   ${CMAKE_SOURCE_DIR}/vendor/VanGUI)
    set(VANGUI_MISC  ${VANGUI_DIR}/misc)
    set(VANGUI_BACK  ${VANGUI_DIR}/backends)

    add_library(vangui STATIC
        ${VANGUI_DIR}/vangui.cpp
        ${VANGUI_DIR}/vangui_draw.cpp
        ${VANGUI_DIR}/vangui_widgets.cpp
        ${VANGUI_DIR}/vangui_tables.cpp
        ${VANGUI_DIR}/vangui_docking.cpp
    )
    target_include_directories(vangui SYSTEM PUBLIC ${VANGUI_DIR})
    target_compile_features(vangui PUBLIC cxx_std_23)
    target_compile_definitions(vangui PUBLIC VANGUI_ENABLE_ANIM)

    macro(mwtools_add_vangui_misc name src)
        add_library(${name} STATIC ${VANGUI_MISC}/${src})
        target_include_directories(${name} SYSTEM PUBLIC ${VANGUI_DIR} ${VANGUI_MISC})
        target_link_libraries(${name} PUBLIC vangui)
        target_compile_features(${name} PUBLIC cxx_std_23)
    endmacro()

    mwtools_add_vangui_misc(vangui_anim          vangui_anim.cpp)
    target_compile_definitions(vangui_anim PUBLIC VANGUI_ENABLE_ANIM)

    find_package(Threads REQUIRED)
    mwtools_add_vangui_misc(vangui_thread        vangui_thread.cpp)
    target_compile_definitions(vangui_thread PUBLIC VANGUI_ENABLE_THREAD)
    target_link_libraries(vangui_thread PUBLIC Threads::Threads)

    mwtools_add_vangui_misc(vangui_notify        vangui_notify.cpp)
    target_compile_definitions(vangui_notify PUBLIC VANGUI_ENABLE_NOTIFY)

    mwtools_add_vangui_misc(vangui_themes        vangui_themes.cpp)
    target_compile_definitions(vangui_themes PUBLIC VANGUI_ENABLE_THEMES)

    mwtools_add_vangui_misc(vangui_theme_engine  vangui_theme_engine.cpp)
    target_compile_definitions(vangui_theme_engine PUBLIC VANGUI_ENABLE_THEME_ENGINE)
    target_link_libraries(vangui_theme_engine PUBLIC vangui_themes)

    mwtools_add_vangui_misc(vangui_command_palette vangui_command_palette.cpp)
    target_compile_definitions(vangui_command_palette PUBLIC VANGUI_ENABLE_COMMAND_PALETTE)

    mwtools_add_vangui_misc(vangui_shortcuts     vangui_shortcuts.cpp)
    target_compile_definitions(vangui_shortcuts PUBLIC VANGUI_ENABLE_SHORTCUTS)
    target_link_libraries(vangui_shortcuts PUBLIC vangui_command_palette)

    mwtools_add_vangui_misc(vangui_node_graph    vangui_node_graph.cpp)
    target_compile_definitions(vangui_node_graph PUBLIC VANGUI_ENABLE_NODE_GRAPH)

    add_library(vangui_signals INTERFACE)
    target_include_directories(vangui_signals SYSTEM INTERFACE ${VANGUI_DIR} ${VANGUI_MISC})
    target_compile_definitions(vangui_signals INTERFACE VANGUI_ENABLE_SIGNALS VANGUI_ENABLE_THREAD)
    target_link_libraries(vangui_signals INTERFACE vangui_thread)

    add_library(vangui_backend_android STATIC ${VANGUI_BACK}/vangui_impl_android.cpp)
    target_include_directories(vangui_backend_android SYSTEM PUBLIC ${VANGUI_DIR} ${VANGUI_BACK})
    target_link_libraries(vangui_backend_android PUBLIC vangui android log)
    target_compile_features(vangui_backend_android PUBLIC cxx_std_23)

    add_library(vangui_backend_opengl3 STATIC ${VANGUI_BACK}/vangui_impl_opengl3.cpp)
    target_include_directories(vangui_backend_opengl3 SYSTEM PUBLIC ${VANGUI_DIR} ${VANGUI_BACK})
    target_link_libraries(vangui_backend_opengl3 PUBLIC vangui GLESv3)
    target_compile_definitions(vangui_backend_opengl3 PUBLIC VANGUI_IMPL_OPENGL_ES3)
    target_compile_features(vangui_backend_opengl3 PUBLIC cxx_std_23)

    add_library(miniaudio_headers INTERFACE)
    target_include_directories(miniaudio_headers SYSTEM INTERFACE
        ${CMAKE_SOURCE_DIR}/vendor/miniaudio
        ${CMAKE_SOURCE_DIR}
    )

    add_library(minimp3_headers INTERFACE)
    target_include_directories(minimp3_headers SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/vendor/minimp3)

    add_library(native_app_glue STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
    target_include_directories(native_app_glue PUBLIC
        ${ANDROID_NDK}/sources/android/native_app_glue)

    set(UI_SOURCES
        src/ui/FileDialog.cpp
        src/ui/AppShell.cpp
        src/ui/TexturePackPanel.cpp
        src/ui/ObjectExportPanel.cpp
        src/ui/EffectPreview.cpp
        src/ui/EffectsPanel.cpp
        src/ui/EmitterEditor.cpp
        src/ui/AudioPanel.cpp
        src/ui/SoundBankPanel.cpp
        src/ui/MusicPanel.cpp
        src/ui/VideoPanel.cpp
        src/ui/UIHudPanel.cpp
        src/ui/UIFrontendPanel.cpp
        src/ui/UIHudAtlasPanel.cpp
        src/ui/UIFontPanel.cpp
        src/ui/CarPanel.cpp
        src/ui/CarBrowserPanel.cpp
        src/ui/CarViewerPanel.cpp
        src/ui/CarMeshPanel.cpp
        src/ui/CarTexturePanel.cpp
        src/ui/CarVinylPanel.cpp
        src/ui/CarPerfPanel.cpp
        src/ui/CarPursuitPanel.cpp
        src/ui/CarEngineAudioPanel.cpp
        src/ui/RecentFiles.cpp
        src/ui/UISounds.cpp
        src/ui/MinimapPanel.cpp
        src/ui/NisPanel.cpp
        src/ui/StreamGraphPanel.cpp
        src/formats/VideoFileStub.cpp
        src/async/TaskQueue.cpp
    )

    set(RENDERER_SOURCES
        src/renderer/ShaderProgram.cpp
        src/renderer/TextureManager.cpp
        src/renderer/MeshRenderer.cpp
        src/renderer/OrbitCamera.cpp
        src/renderer/ThumbnailCache.cpp
        src/renderer/TextureThumbnail.cpp
    )
    set(EDITOR_SOURCES
        src/editor/Exporter.cpp
        src/editor/GLTFExporter.cpp
        src/editor/MeshImporter.cpp
    )

    add_library(mwtools_android SHARED
        src/android/android_main.cpp
        ${UI_SOURCES}
        ${RENDERER_SOURCES}
        ${EDITOR_SOURCES}
    )
    target_compile_features(mwtools_android PRIVATE cxx_std_23)
    target_compile_definitions(mwtools_android PRIVATE
        NFSMW_PLATFORM_ANDROID=1
        NFSMW_ENABLE_VIDEO=0
        VANGUI_IMPL_OPENGL_ES3
    )
    target_include_directories(mwtools_android SYSTEM PRIVATE
        ${VANGUI_DIR}
        ${VANGUI_MISC}
        ${VANGUI_BACK}
        ${ANDROID_NDK}/sources/android/native_app_glue
        ${CMAKE_SOURCE_DIR}/vendor/miniaudio
    )
    target_include_directories(mwtools_android PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(mwtools_android PRIVATE
        nfsmw_core
        vangui
        vangui_backend_android
        vangui_backend_opengl3
        vangui_anim
        vangui_thread
        vangui_notify
        vangui_themes
        vangui_theme_engine
        vangui_command_palette
        vangui_shortcuts
        vangui_node_graph
        vangui_signals
        miniaudio_headers
        minimp3_headers
        native_app_glue
        android
        log
        EGL
        GLESv3
    )
    target_link_options(mwtools_android PRIVATE "-u" "ANativeActivity_onCreate")
    target_compile_options(mwtools_android PRIVATE -Wall -Wextra)

elseif(NFSMW_BUILD_GUI)
    set(VANGUI_DIR   ${CMAKE_SOURCE_DIR}/vendor/VanGUI)
    set(VANGUI_MISC  ${VANGUI_DIR}/misc)
    set(VANGUI_BACK  ${VANGUI_DIR}/backends)

    # ---- Core library (classic non-module build) ---------------------------
    add_library(vangui STATIC
        ${VANGUI_DIR}/vangui.cpp
        ${VANGUI_DIR}/vangui_draw.cpp
        ${VANGUI_DIR}/vangui_widgets.cpp
        ${VANGUI_DIR}/vangui_tables.cpp
        ${VANGUI_DIR}/vangui_docking.cpp
    )
    target_include_directories(vangui SYSTEM PUBLIC ${VANGUI_DIR})
    target_compile_features(vangui PUBLIC cxx_std_23)
    # The animation substrate installs one guarded hook in VanGui::NewFrame(),
    # so the macro must be visible to the core TU and every consumer.
    target_compile_definitions(vangui PUBLIC VANGUI_ENABLE_ANIM)

    # ---- Helper for the compiled misc modules ------------------------------
    macro(mwtools_add_vangui_misc name src)
        add_library(${name} STATIC ${VANGUI_MISC}/${src})
        target_include_directories(${name} SYSTEM PUBLIC ${VANGUI_DIR} ${VANGUI_MISC})
        target_link_libraries(${name} PUBLIC vangui)
        target_compile_features(${name} PUBLIC cxx_std_23)
    endmacro()

    # Animation / transition substrate (Pillar 1) — drives theme transitions
    # and the idle-redraw hint.
    mwtools_add_vangui_misc(vangui_anim          vangui_anim.cpp)
    target_compile_definitions(vangui_anim PUBLIC VANGUI_ENABLE_ANIM)

    # Thread pool + main-thread dispatch (Phase 2 — replaces TaskQueue).
    find_package(Threads REQUIRED)
    mwtools_add_vangui_misc(vangui_thread        vangui_thread.cpp)
    target_compile_definitions(vangui_thread PUBLIC VANGUI_ENABLE_THREAD)
    target_link_libraries(vangui_thread PUBLIC Threads::Threads)

    # Toast notifications (Phase 3 — replaces ToastManager).
    mwtools_add_vangui_misc(vangui_notify        vangui_notify.cpp)
    target_compile_definitions(vangui_notify PUBLIC VANGUI_ENABLE_NOTIFY)

    # Named theme presets (Phase 4).
    mwtools_add_vangui_misc(vangui_themes        vangui_themes.cpp)
    target_compile_definitions(vangui_themes PUBLIC VANGUI_ENABLE_THEMES)

    # Semantic token engine + animated transitions + hot-reload (Phase 4).
    mwtools_add_vangui_misc(vangui_theme_engine  vangui_theme_engine.cpp)
    target_compile_definitions(vangui_theme_engine PUBLIC VANGUI_ENABLE_THEME_ENGINE)
    target_link_libraries(vangui_theme_engine PUBLIC vangui_themes)

    # Command palette (Phase 5) — Ctrl+K / Ctrl+Shift+P.
    mwtools_add_vangui_misc(vangui_command_palette vangui_command_palette.cpp)
    target_compile_definitions(vangui_command_palette PUBLIC VANGUI_ENABLE_COMMAND_PALETTE)

    # Keyboard shortcut registry (Phase 5) — feeds the palette automatically.
    mwtools_add_vangui_misc(vangui_shortcuts     vangui_shortcuts.cpp)
    target_compile_definitions(vangui_shortcuts PUBLIC VANGUI_ENABLE_SHORTCUTS)
    target_link_libraries(vangui_shortcuts PUBLIC vangui_command_palette)

    # Node graph (Phase 8) — stream-bundle dependency visualisation.
    mwtools_add_vangui_misc(vangui_node_graph    vangui_node_graph.cpp)
    target_compile_definitions(vangui_node_graph PUBLIC VANGUI_ENABLE_NODE_GRAPH)

    # Signals / slots (Phase 7) — header-only; connect_queued() needs THREAD.
    add_library(vangui_signals INTERFACE)
    target_include_directories(vangui_signals SYSTEM INTERFACE ${VANGUI_DIR} ${VANGUI_MISC})
    target_compile_definitions(vangui_signals INTERFACE VANGUI_ENABLE_SIGNALS VANGUI_ENABLE_THREAD)
    target_link_libraries(vangui_signals INTERFACE vangui_thread)

    # ---- Platform + renderer backends --------------------------------------
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glad CONFIG QUIET)

    if(NFSMW_ENABLE_VIDEO)
        set(NFSMW_VIDEO_SOURCE src/formats/VideoFile.cpp)
    else()
        set(NFSMW_VIDEO_SOURCE src/formats/VideoFileStub.cpp)
    endif()

    add_library(vangui_backend_glfw STATIC ${VANGUI_BACK}/vangui_impl_glfw.cpp)
    target_include_directories(vangui_backend_glfw SYSTEM PUBLIC ${VANGUI_DIR} ${VANGUI_BACK})
    target_link_libraries(vangui_backend_glfw PUBLIC vangui glfw)
    target_compile_features(vangui_backend_glfw PUBLIC cxx_std_23)

    add_library(vangui_backend_opengl3 STATIC ${VANGUI_BACK}/vangui_impl_opengl3.cpp)
    target_include_directories(vangui_backend_opengl3 SYSTEM PUBLIC ${VANGUI_DIR} ${VANGUI_BACK})
    target_link_libraries(vangui_backend_opengl3 PUBLIC vangui)
    target_compile_features(vangui_backend_opengl3 PUBLIC cxx_std_23)

    # Vendored glad fallback (vendor/glad/{include,src/glad.c}) — MWTools' own
    # renderer uses glad; VanGUI's OpenGL3 backend has its own loader.
    if(NOT TARGET glad::glad)
        if(EXISTS ${CMAKE_SOURCE_DIR}/vendor/glad/src/glad.c)
            add_library(glad_vendored STATIC vendor/glad/src/glad.c)
            target_include_directories(glad_vendored PUBLIC ${CMAKE_SOURCE_DIR}/vendor/glad/include)
            add_library(glad::glad ALIAS glad_vendored)
        else()
            message(FATAL_ERROR "glad not found (vcpkg: glad, or vendor it at vendor/glad)")
        endif()
    endif()

    # miniaudio — header-only, vendored at vendor/miniaudio/miniaudio.h.
    add_library(miniaudio_headers INTERFACE)
    target_include_directories(miniaudio_headers SYSTEM INTERFACE
        ${CMAKE_SOURCE_DIR}/vendor/miniaudio
        ${CMAKE_SOURCE_DIR}
    )

    # minimp3 — header-only, vendored at vendor/minimp3/minimp3.h.
    add_library(minimp3_headers INTERFACE)
    target_include_directories(minimp3_headers SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/vendor/minimp3)

    # FFmpeg (libav*) — VP6 movie decode for the Videos panel. Optional: the
    # 3D mesh/texture preview does not need it at all.
    if(NFSMW_ENABLE_VIDEO)
        find_package(FFMPEG REQUIRED)
    endif()

    set(UI_SOURCES
        src/ui/FileDialog.cpp
        src/ui/AppShell.cpp
        src/ui/TexturePackPanel.cpp
        src/ui/ObjectExportPanel.cpp
        src/ui/EffectPreview.cpp
        src/ui/EffectsPanel.cpp
        src/ui/EmitterEditor.cpp
        src/ui/AudioPanel.cpp
        src/ui/SoundBankPanel.cpp
        src/ui/MusicPanel.cpp
        src/ui/VideoPanel.cpp
        src/ui/UIHudPanel.cpp
        src/ui/UIFrontendPanel.cpp
        src/ui/UIHudAtlasPanel.cpp
        src/ui/UIFontPanel.cpp
        src/ui/CarPanel.cpp
        src/ui/CarBrowserPanel.cpp
        src/ui/CarViewerPanel.cpp
        src/ui/CarMeshPanel.cpp
        src/ui/CarTexturePanel.cpp
        src/ui/CarVinylPanel.cpp
        src/ui/CarPerfPanel.cpp
        src/ui/CarPursuitPanel.cpp
        src/ui/CarEngineAudioPanel.cpp
        src/ui/RecentFiles.cpp
        src/ui/UISounds.cpp
        src/ui/MinimapPanel.cpp
        src/ui/NisPanel.cpp
        src/ui/StreamGraphPanel.cpp
        ${NFSMW_VIDEO_SOURCE}
        src/async/TaskQueue.cpp     # Phase 2: now a thin adapter over vangui_thread
    )
    # NOTE: src/ui/Toast.cpp removed (Phase 3 -> vangui_notify).

    # Phase 6: renderer (GL upload + preview) and editor (Exporter OBJ/glTF writer)
    set(RENDERER_SOURCES
        src/renderer/ShaderProgram.cpp
        src/renderer/TextureManager.cpp
        src/renderer/MeshRenderer.cpp
        src/renderer/OrbitCamera.cpp
        src/renderer/ThumbnailCache.cpp
        src/renderer/TextureThumbnail.cpp
    )
    set(EDITOR_SOURCES
        src/editor/Exporter.cpp
        src/editor/GLTFExporter.cpp
        src/editor/MeshImporter.cpp
    )

    add_executable(NFSMWToolkit
        src/main.cpp
        ${UI_SOURCES}
        ${RENDERER_SOURCES}
        ${EDITOR_SOURCES}
    )

    # VanGUI requires C++23; the GUI executable includes its headers so it must
    # compile at C++23 (nfsmw_core stays C++20 — see CLAUDE.md).
    target_compile_features(NFSMWToolkit PRIVATE cxx_std_23)

    target_link_libraries(NFSMWToolkit PRIVATE
        nfsmw_core
        glfw
        glad::glad
        # VanGUI core + backends
        vangui
        vangui_backend_glfw
        vangui_backend_opengl3
        # VanGUI enhancement modules
        vangui_anim
        vangui_thread
        vangui_notify
        vangui_themes
        vangui_theme_engine
        vangui_command_palette
        vangui_shortcuts
        vangui_node_graph
        vangui_signals
        miniaudio_headers
        minimp3_headers
        ${FFMPEG_LIBRARIES}
    )

    # FFmpeg headers / link dirs (vcpkg's FindFFMPEG provides these variables).
    target_include_directories(NFSMWToolkit SYSTEM PRIVATE ${FFMPEG_INCLUDE_DIRS})
    target_link_directories(NFSMWToolkit PRIVATE ${FFMPEG_LIBRARY_DIRS})

    # VanGUI headers (core + misc + backends) — SYSTEM so /external:W0 silences
    # vendor warnings. MWTools includes <vangui.h>, <vangui_impl_glfw.h>,
    # <vangui_enhance.h> (and friends) directly.
    target_include_directories(NFSMWToolkit SYSTEM PRIVATE
        ${VANGUI_DIR}
        ${VANGUI_MISC}
        ${VANGUI_BACK}
        ${CMAKE_SOURCE_DIR}/vendor/miniaudio
    )
    target_include_directories(NFSMWToolkit PRIVATE
        ${CMAKE_SOURCE_DIR}
    )

    target_compile_definitions(NFSMWToolkit PRIVATE GLFW_INCLUDE_NONE)

    if(WIN32)
        # Embed resources: ICON for shell/taskbar, RCDATA blobs for runtime WIC decode.
        target_sources(NFSMWToolkit PRIVATE resources.rc)
        target_compile_definitions(NFSMWToolkit PRIVATE NFSMW_PLATFORM_WINDOWS=1)
        target_link_libraries(NFSMWToolkit PRIVATE opengl32 dwmapi ole32 winmm windowscodecs)
        set_target_properties(NFSMWToolkit PROPERTIES WIN32_EXECUTABLE "$<CONFIG:Release>")
        # Ship the binary branded as MWTools.exe (keep the CMake target name stable).
        set_target_properties(NFSMWToolkit PROPERTIES OUTPUT_NAME "MWTools")
        if(MSVC)
            target_link_options(NFSMWToolkit PRIVATE "$<$<CONFIG:Release>:/ENTRY:mainCRTStartup>")
        endif()
    elseif(UNIX)
        target_link_libraries(NFSMWToolkit PRIVATE GL ${CMAKE_DL_LIBS} pthread)
    endif()

    if(MSVC)
        target_compile_options(NFSMWToolkit PRIVATE
            /W4 /permissive- /d2SSAOptimizer-
            /external:W0
        )
    else()
        target_compile_options(NFSMWToolkit PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    # Copy runtime dependency DLLs (glfw3 / spdlog / fmt …) next to the exe.
    add_custom_command(TARGET NFSMWToolkit POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:NFSMWToolkit> $<TARGET_FILE_DIR:NFSMWToolkit>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying runtime DLLs next to NFSMWToolkit.exe")

    if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
        add_custom_command(TARGET NFSMWToolkit POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets
                $<TARGET_FILE_DIR:NFSMWToolkit>/assets
            COMMENT "Copying assets...")
    endif()
endif()

# --- mwcli: headless format verifier ----------------------------------------
# Links only against nfsmw_core (glm/spdlog/json) — no GLFW/VanGUI/FFmpeg/
# miniaudio — so it builds and runs anywhere the core library does. Stays C++20.
if(NFSMW_BUILD_CLI)
    add_executable(mwcli tools/mwcli.cpp)
    target_link_libraries(mwcli PRIVATE nfsmw_core)
    target_include_directories(mwcli PRIVATE ${CMAKE_SOURCE_DIR}/include)
    if(MSVC)
        target_compile_options(mwcli PRIVATE /W4 /permissive-)
    else()
        target_compile_options(mwcli PRIVATE -Wall -Wextra)
    endif()
endif()

# --- tests: unit + round-trip regression suite ------------------------------
# Links only against nfsmw_core (no GUI deps), so it builds and runs in CI on
# any platform the core library supports. Run with `ctest` after building.
# Roadmap: R1 (unit tests) + R2 (round-trip integrity).
if(NFSMW_BUILD_TESTS)
    enable_testing()
    file(GLOB MWTOOLS_TEST_SRC ${CMAKE_SOURCE_DIR}/tests/test_*.cpp)
    add_executable(mwtools_tests ${MWTOOLS_TEST_SRC})
    target_link_libraries(mwtools_tests PRIVATE nfsmw_core)
    target_include_directories(mwtools_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests)
    if(MSVC)
        target_compile_options(mwtools_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(mwtools_tests PRIVATE -Wall -Wextra)
    endif()
    add_test(NAME mwtools_tests COMMAND mwtools_tests)
endif()
