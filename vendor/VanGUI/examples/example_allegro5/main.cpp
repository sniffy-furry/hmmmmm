// VanGUI: standalone example application for Allegro 5

// Learn about VanGUI:
// - FAQ                  https://dearvangui.com/faq
// - Getting Started      https://dearvangui.com/getting-started
// - Documentation        https://dearvangui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of vangui.cpp

// On Windows, you can install Allegro5 using vcpkg:
//   git clone https://github.com/Microsoft/vcpkg
//   cd vcpkg
//   bootstrap - vcpkg.bat
//   vcpkg install allegro5 --triplet=x86-windows   ; for win32
//   vcpkg install allegro5 --triplet=x64-windows   ; for win64
//   vcpkg integrate install                        ; register include and libs in Visual Studio

#include <stdint.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include "vangui.h"
#include "vangui_impl_allegro5.h"

int main(int, char**)
{
    // Setup Allegro
    al_init();
    al_install_keyboard();
    al_install_mouse();
    al_init_primitives_addon();
    al_set_new_display_flags(ALLEGRO_RESIZABLE);
    ALLEGRO_DISPLAY* display = al_create_display(1280, 800);
    al_set_window_title(display, "VanGUI Allegro 5 example");
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());

    // Setup VanGUI context
    VANGUI_CHECKVERSION();
    VanGui::CreateContext();
    VanGuiIO& io = VanGui::GetIO(); (void)io;
    io.ConfigFlags |= VanGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    // Setup VanGUI style
    VanGui::StyleColorsDark();
    //VanGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    VanGui_ImplAllegro5_Init(display);

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

    bool show_demo_window = true;
    bool show_another_window = false;
    VanVec4 clear_color = VanVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool running = true;
    while (running)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear vangui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear vangui, and hide them from your application based on those two flags.
        ALLEGRO_EVENT ev;
        while (al_get_next_event(queue, &ev))
        {
            VanGui_ImplAllegro5_ProcessEvent(&ev);
            if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
                running = false;
            if (ev.type == ALLEGRO_EVENT_DISPLAY_RESIZE)
            {
                VanGui_ImplAllegro5_InvalidateDeviceObjects();
                al_acknowledge_resize(display);
                VanGui_ImplAllegro5_CreateDeviceObjects();
            }
        }

        // Start the VanGUI frame
        VanGui_ImplAllegro5_NewFrame();
        VanGui::NewFrame();

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
        al_clear_to_color(al_map_rgba_f(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w));
        VanGui_ImplAllegro5_RenderDrawData(VanGui::GetDrawData());
        al_flip_display();
    }

    // Cleanup
    VanGui_ImplAllegro5_Shutdown();
    VanGui::DestroyContext();
    al_destroy_event_queue(queue);
    al_destroy_display(display);

    return 0;
}
