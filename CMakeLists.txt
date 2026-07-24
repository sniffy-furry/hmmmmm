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

# --- For Android build, force GUI OFF and JNI ON ----------------------------
if(ANDROID)
    set(NFSMW_BUILD_GUI OFF CACHE BOOL "Disable GUI on Android" FORCE)
    set(NFSMW_BUILD_CLI OFF CACHE BOOL "Disable CLI on Android" FORCE)
    set(NFSMW_BUILD_TESTS OFF CACHE BOOL "Disable tests on Android" FORCE)
    set(NFSMW_BUILD_ANDROID_JNI ON CACHE BOOL "Enable JNI on Android" FORCE)
    set(NFSMW_ENABLE_VIDEO OFF CACHE BOOL "Disable video on Android" FORCE)
endif()

# --- Output directories -----------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# --- Find dependencies (installed via apt-get on CI) ------------------------
# NOTE: NO_CMAKE_FIND_ROOT_PATH added below. When cross-compiling for Android
# via the NDK toolchain file, CMake restricts find_path()/find_package() to
# the NDK's own sysroot by default (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY),
# which hides /usr/include - so apt-get installing these never helped. These
# three are header-only (no compiled/arch-specific code), so it's safe to
# tell CMake to also look at the host's normal system paths for them.

# GLM - header-only
find_path(GLM_INCLUDE_DIR NAMES glm/glm.hpp PATHS /usr/include /usr/local/include NO_CMAKE_FIND_ROOT_PATH)
if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "glm not found (install libglm-dev)")
endif()
add_library(glm::glm INTERFACE IMPORTED)
set_target_properties(glm::glm PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE_DIR}")

# SPDLog - header-only library
find_path(SPDLOG_INCLUDE_DIR NAMES spdlog/spdlog.h PATHS /usr/include /usr/local/include NO_CMAKE_FIND_ROOT_PATH)
if(NOT SPDLOG_INCLUDE_DIR)
    message(FATAL_ERROR "spdlog not found (install libspdlog-dev)")
endif()
add_library(spdlog::spdlog INTERFACE IMPORTED)
set_target_properties(spdlog::spdlog PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_INCLUDE_DIR}")

# nlohmann-json - header-only
find_path(NLOHMANN_INCLUDE_DIR NAMES nlohmann/json.hpp PATHS /usr/include /usr/local/include NO_CMAKE_FIND_ROOT_PATH)
if(NOT NLOHMANN_INCLUDE_DIR)
    message(FATAL_ERROR "nlohmann-json not found (install nlohmann-json3-dev)")
endif()
add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
set_target_properties(nlohmann_json::nlohmann_json PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${NLOHMANN_INCLUDE_DIR}")

# --- nfsmw_core: file format parsers (no GL dependencies) -------------------
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
        $<$<CXX_COMPILER_ID:MSVC>:/d2SSAOptimizer->
    )
else()
    target_compile_options(nfsmw_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- nfsmw_jni: lean Android JNI bridge (no VanGUI/GLFW/FFmpeg) -------------
if(NFSMW_BUILD_ANDROID_JNI AND ANDROID)
    add_library(nfsmw_jni SHARED
        src/android/jni_bridge.cpp
    )
    target_link_libraries(nfsmw_jni PRIVATE nfsmw_core log)
    target_include_directories(nfsmw_jni PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_compile_options(nfsmw_jni PRIVATE -Wall -Wextra)
endif()

# --- mwcli: headless format verifier ----------------------------------------
if(NFSMW_BUILD_CLI AND NOT ANDROID)
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
if(NFSMW_BUILD_TESTS AND NOT ANDROID)
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
