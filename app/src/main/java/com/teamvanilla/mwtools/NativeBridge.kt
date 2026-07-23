package com.teamvanilla.mwtools

/**
 * Thin Kotlin facade over libnfsmw_jni.so (src/android/jni_bridge.cpp).
 * Links only against nfsmw_core - no VanGUI/GLFW/FFmpeg.
 */
object NativeBridge {
    init {
        System.loadLibrary("nfsmw_jni")
    }

    external fun identify(path: String): String
    external fun info(path: String): String
}
