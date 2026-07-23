// VanGUI: standalone example application for GLUT/FreeGLUT + OpenGL2, using legacy fixed pipeline

// Learn about VanGUI:
// - FAQ                  https://dearvangui.com/faq
// - Getting Started      https://dearvangui.com/getting-started
// - Documentation        https://dearvangui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of vangui.cpp

// !!! GLUT/FreeGLUT IS OBSOLETE PREHISTORIC SOFTWARE. Using GLUT is not recommended unless you really miss the 90's. !!!
// !!! If someone or something is teaching you GLUT today, you are being abused. Please show some resistance. !!!
// !!! Nowadays, prefer using GLFW or SDL instead!

// On Windows, you can install Freeglut using vcpkg:
//   git clone https://github.com/Microsoft/vcpkg
//   cd vcpkg
//   bootstrap - vcpkg.bat
//   vcpkg install freeglut --triplet=x86-windows   ; for win32
//   vcpkg install freeglut --triplet=x64-windows   ; for win64
//   vcpkg integrate install                        ; register include and libs in Visual Studio

#include "vangui.h"
#include "vangui_impl_glut.h"
#include "vangui_impl_opengl2.h"
#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif

#ifdef _MSC_VER
#pragma warning (disable: 4505) // unreferenced local function has been removed
#endif

// Forward declarations of helper functions
void MainLoopStep();

// Our state
static bool show_demo_window = true;
static bool show_another_window = false;
static VanVec4 clear_color = VanVec4(0.45f, 0.55f, 0.60f, 1.00f);

int main(int argc, char** argv)
{
    // Create GLUT window
    glutInit(&argc, argv);
#ifdef __FREEGLUT_EXT_H__
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#endif
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_MULTISAMPLE);
    glutInitWindowSize(1280, 800);
    glutCreateWindow("VanGUI GLUT+OpenGL2 Example");

    // Setup GLUT display function
    // We will also call VanGui_ImplGLUT_InstallFuncs() to get all the other functions installed for us,
    // otherwise it is possible to install our own functions and call the vangui_impl_glut.h functions ourselves.
    glutDisplayFunc(MainLoopStep);

    // Setup VanGUI context
    VANGUI_CHECKVERSION();
    VanGui::CreateContext();
    VanGuiIO& io = VanGui::GetIO(); (void)io;
    io.ConfigFlags |= VanGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup VanGUI style
    VanGui::StyleColorsDark();
    //VanGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    // FIXME: Consider reworking this example to install our own GLUT funcs + forward calls VanGui_ImplGLUT_XXX ones, instead of using VanGui_ImplGLUT_InstallFuncs().
    VanGui_ImplGLUT_Init();
    VanGui_ImplOpenGL2_Init();

    // Install GLUT handlers (glutReshapeFunc(), glutMotionFunc(), glutPassiveMotionFunc(), glutMouseFunc(), glutKeyboardFunc() etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear vangui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear vangui, and hide them from your application based on those two flags.
    VanGui_ImplGLUT_InstallFuncs();

    // Load Fonts
    // - If fonts are not explicitly loaded, VanGUI will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use VanGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define VANGUI_ENABLE_FREETYPE' in your vanconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //VanFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //VAN_ASSERT(font != nullptr);

    // Main loop
    glutMainLoop();

    // Cleanup
    VanGui_ImplOpenGL2_Shutdown();
    VanGui_ImplGLUT_Shutdown();
    VanGui::DestroyContext();

    return 0;
}

void MainLoopStep()
{
    // Start the VanGUI frame
    VanGui_ImplOpenGL2_NewFrame();
    VanGui_ImplGLUT_NewFrame();
    VanGui::NewFrame();
    VanGuiIO& io = VanGui::GetIO();

    // 1. Show the big demo window (Most of the sample code is in VanGui::ShowDemoWindow()! You can browse its code to learn more about VanGUI!).
    if (show_demo_window)
        VanGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        VanGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        VanGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        VanGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        VanGui::Checkbox("Another Window", &show_another_window);

        VanGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        VanGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (VanGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        VanGui::SameLine();
        VanGui::Text("counter = %d", counter);

        VanGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        VanGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        VanGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        VanGui::Text("Hello from another window!");
        if (VanGui::Button("Close Me"))
            show_another_window = false;
        VanGui::End();
    }

    // Rendering
    VanGui::Render();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound, but prefer using the GL3+ code.
    VanGui_ImplOpenGL2_RenderDrawData(VanGui::GetDrawData());

    glutSwapBuffers();
    glutPostRedisplay();
}
