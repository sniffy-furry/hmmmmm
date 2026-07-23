// dear vangui: "null" example application
// (compile and link vangui, create context, run headless with NO INPUTS, NO GRAPHICS OUTPUT)
// This is useful to test building, but you cannot interact with anything here!
#include "vangui.h"
#include <cstdio>

// For vangui_impl_null: use relative filename + embed implementation directly by including the .cpp file.
// This is to simplify casual building of this example from all sorts of test scripts.
#include "../../backends/vangui_impl_null.h"
#include "../../backends/vangui_impl_null.cpp"

int main(int, char**)
{
    VANGUI_CHECKVERSION();

    VanGui::CreateContext();
    VanGuiIO& io = VanGui::GetIO();

    VanGui_ImplNullPlatform_Init();
    VanGui_ImplNullRender_Init();

    for (int n = 0; n < 20; n++)
    {
        printf("NewFrame() %d\n", n);
        VanGui_ImplNullPlatform_NewFrame();
        VanGui_ImplNullRender_NewFrame();
        VanGui::NewFrame();

        static float f = 0.0f;
        VanGui::Text("Hello, world!");
        VanGui::SliderFloat("float", &f, 0.0f, 1.0f);
        VanGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        VanGui::ShowDemoWindow(nullptr);

        VanGui::Render();
    }

    printf("DestroyContext()\n");
    VanGui_ImplNullRender_Shutdown();
    VanGui_ImplNullPlatform_Shutdown();
    VanGui::DestroyContext();
    return 0;
}
