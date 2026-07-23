# MWTools Android native port

This tree now includes an initial native Android/NDK port that keeps the existing
C++ core and VanGUI UI.

## What was added

- Android Gradle project: `settings.gradle`, root `build.gradle`, `app/build.gradle`
- NativeActivity manifest: `app/src/main/AndroidManifest.xml`
- EGL/OpenGL ES 3 entry point: `src/android/android_main.cpp`
- Android AppShell path using `vangui_impl_android` + `vangui_impl_opengl3`
- GLES compatibility include: `include/renderer/GLCompat.h`
- Android target in `CMakeLists.txt`: `mwtools_android`

## Current limitations

- FFmpeg video support is disabled on Android for the first build target. The
  Android target uses `src/formats/VideoFileStub.cpp`.
- Desktop wireframe polygon mode is disabled on Android because OpenGL ES does
  not provide `glPolygonMode`.
- Android file access still uses the in-app VanGUI file browser. Full Android
  Storage Access Framework integration is a later step.
- External C++ packages (`glm`, `spdlog`, `nlohmann-json`) are still resolved by
  CMake/vcpkg-style package discovery, same as the desktop project.

## Build direction

Open this folder in Android Studio and build the `app` module with an Android
NDK installed. The Gradle target builds `libmwtools_android.so` via CMake.

