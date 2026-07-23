cmake_minimum_required(VERSION 3.28)

project(VanGuiExample)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(${CMAKE_PROJECT_NAME} SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../vangui.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../vangui_demo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../vangui_draw.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../vangui_tables.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../vangui_widgets.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../backends/vangui_impl_android.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../../backends/vangui_impl_opengl3.cpp
  ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

set(CMAKE_SHARED_LINKER_FLAGS
  "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate"
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
  VANGUI_IMPL_OPENGL_ES3
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
  ${CMAKE_CURRENT_SOURCE_DIR}/../../backends
  ${ANDROID_NDK}/sources/android/native_app_glue
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
  android
  EGL
  GLESv3
  log
)
