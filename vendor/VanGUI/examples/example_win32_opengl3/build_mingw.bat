@REM Build for MINGW64 or 32 from MSYS2.
@set OUT_DIR=Debug
@set OUT_EXE=example_win32_opengl3
@set INCLUDES=-I../.. -I../../backends
@set SOURCES=main.cpp ../../backends/vangui_impl_opengl3.cpp ../../backends/vangui_impl_win32.cpp ../../vangui*.cpp
@set LIBS=-lopengl32 -lgdi32 -ldwmapi
mkdir %OUT_DIR%
g++ -DUNICODE %INCLUDES% %SOURCES% -o %OUT_DIR%/%OUT_EXE%.exe --static -mwindows %LIBS% %LIBS%
