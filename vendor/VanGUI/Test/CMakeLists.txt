cmake_minimum_required(VERSION 3.28)

set(BACKEND_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../backends")

add_executable(vangui_clock_test)

# main.cpp is a C++23 module unit (module vangui_clock_test;)
# It must be listed in FILE_SET CXX_MODULES so CMake scans its
# `import vangui;` dependency and feeds the correct IFCs to MSVC.
target_sources(vangui_clock_test
    PUBLIC
        FILE_SET CXX_MODULES FILES
            main.cpp
    PRIVATE
        ${BACKEND_DIR}/vangui_impl_win32.cpp
        ${BACKEND_DIR}/vangui_impl_dx11.cpp
)

target_compile_features(vangui_clock_test PRIVATE cxx_std_23)
set_target_properties(vangui_clock_test PROPERTIES CXX_SCAN_FOR_MODULES ON)

target_compile_definitions(vangui_clock_test PRIVATE UNICODE _UNICODE)

target_include_directories(vangui_clock_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${BACKEND_DIR}
)

target_link_libraries(vangui_clock_test PRIVATE
    vangui_module
    d3d11
    dxgi
    d3dcompiler
    dwmapi
)
