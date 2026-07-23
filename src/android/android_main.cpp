#include "ui/AppShell.h"
#include "core/Logger.h"

#include <vangui_impl_android.h>
#include <android_native_app_glue.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <unistd.h>

#include <memory>

namespace {

constexpr const char* kLogTag = "MWToolsAndroid";

struct AndroidEngine {
    android_app* app = nullptr;

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    EGLConfig  config  = nullptr;

    int width = 0;
    int height = 0;

    std::unique_ptr<nfsmw::AppShell> shell;
    bool shellReady = false;

    bool InitDisplay() {
        if (!app || !app->window)
            return false;

        const EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
            EGL_BLUE_SIZE,       8,
            EGL_GREEN_SIZE,      8,
            EGL_RED_SIZE,        8,
            EGL_ALPHA_SIZE,      8,
            EGL_DEPTH_SIZE,      24,
            EGL_STENCIL_SIZE,    8,
            EGL_NONE
        };

        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY)
            return false;
        if (!eglInitialize(display, nullptr, nullptr))
            return false;

        EGLint numConfigs = 0;
        if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs) || numConfigs < 1)
            return false;

        EGLint format = 0;
        eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
        ANativeWindow_setBuffersGeometry(app->window, 0, 0, format);

        const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_NONE
        };
        surface = eglCreateWindowSurface(display, config, app->window, nullptr);
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
        if (surface == EGL_NO_SURFACE || context == EGL_NO_CONTEXT)
            return false;
        if (!eglMakeCurrent(display, surface, surface, context))
            return false;

        eglQuerySurface(display, surface, EGL_WIDTH, &width);
        eglQuerySurface(display, surface, EGL_HEIGHT, &height);
        eglSwapInterval(display, 1);
        return true;
    }

    void InitShell() {
        if (shellReady)
            return;

        // Keep relative writes (log/recent files) inside app-private storage.
        if (app && app->activity && app->activity->internalDataPath)
            chdir(app->activity->internalDataPath);

        shell = std::make_unique<nfsmw::AppShell>();
        auto r = shell->InitAndroid(app, width > 0 ? width : 1400, height > 0 ? height : 900);
        if (!r) {
            __android_log_print(ANDROID_LOG_ERROR, kLogTag, "App init failed: %s", r.error.c_str());
            shell.reset();
            shellReady = false;
            ANativeActivity_finish(app->activity);
            return;
        }
        shellReady = true;
    }

    void TermShell() {
        if (shellReady && shell) {
            shell->ShutdownAndroid();
            shellReady = false;
        }
        shell.reset();
    }

    void TermDisplay() {
        TermShell();
        if (display != EGL_NO_DISPLAY) {
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (context != EGL_NO_CONTEXT)
                eglDestroyContext(display, context);
            if (surface != EGL_NO_SURFACE)
                eglDestroySurface(display, surface);
            eglTerminate(display);
        }
        display = EGL_NO_DISPLAY;
        context = EGL_NO_CONTEXT;
        surface = EGL_NO_SURFACE;
        config = nullptr;
        width = height = 0;
    }

    void DrawFrame() {
        if (!shellReady || display == EGL_NO_DISPLAY)
            return;
        eglQuerySurface(display, surface, EGL_WIDTH, &width);
        eglQuerySurface(display, surface, EGL_HEIGHT, &height);
        shell->RenderAndroidFrame(width, height);
        eglSwapBuffers(display, surface);
        if (shell->AndroidExitRequested())
            ANativeActivity_finish(app->activity);
    }
};

void HandleCmd(android_app* app, int32_t cmd) {
    auto* engine = static_cast<AndroidEngine*>(app->userData);
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window && engine->InitDisplay())
                engine->InitShell();
            break;
        case APP_CMD_TERM_WINDOW:
            engine->TermDisplay();
            break;
        case APP_CMD_DESTROY:
            engine->TermDisplay();
            break;
        default:
            break;
    }
}

int32_t HandleInput(android_app* app, AInputEvent* event) {
    auto* engine = static_cast<AndroidEngine*>(app->userData);
    if (!engine || !engine->shellReady)
        return 0;
    return VanGui_ImplAndroid_HandleInputEvent(event);
}

} // namespace

void android_main(android_app* app) {
    app_dummy();

    AndroidEngine engine;
    engine.app = app;
    app->userData = &engine;
    app->onAppCmd = HandleCmd;
    app->onInputEvent = HandleInput;

    if (app->activity)
        ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_FULLSCREEN, 0);

    while (true) {
        int events = 0;
        android_poll_source* source = nullptr;

        while (ALooper_pollOnce(engine.shellReady ? 0 : -1, nullptr, &events,
                                reinterpret_cast<void**>(&source)) >= 0) {
            if (source)
                source->process(app, source);
            if (app->destroyRequested) {
                engine.TermDisplay();
                return;
            }
        }

        engine.DrawFrame();
    }
}
