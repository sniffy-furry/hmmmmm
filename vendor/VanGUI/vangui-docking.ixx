// VanGUI Docking Module Partition — thin interface
// Implementation lives in vangui-docking-impl.cpp (partition implementation unit).
// Intentionally empty global module fragment — prevents MSVC C2572.
module;
// intentionally empty global module fragment

export module vangui:docking;

//=============================================================================
// EXPORTED PUBLIC API — docking and viewport API visible to importers
//=============================================================================
export namespace VanGui {
    // Docking flags (global-namespace typedefs — must use :: prefix)
    using ::VanGuiDockNodeFlags;

    // Viewport flags
    using ::VanGuiViewportFlags;

    // Structs
    using ::VanGuiWindowClass;
    using ::VanGuiViewport;

    // Docking API
    using VanGui::DockSpace;
    using VanGui::DockSpaceOverViewport;
    using VanGui::SetNextWindowDockID;
    using VanGui::SetNextWindowClass;
    using VanGui::GetWindowDockID;
    using VanGui::IsWindowDocked;

    // Viewport API
    using VanGui::GetMainViewport;
    using VanGui::GetWindowViewport;
    using VanGui::FindViewportByID;
    using VanGui::FindViewportByPlatformHandle;
} // export namespace VanGui
