// dear vangui, v1.92.9 WIP
// (primary module interface — thin re-export wrapper)
// Delegates all API surface to module partitions.
// The implementation lives in vangui.cpp (module implementation unit paired with this file).
//
// Global fragment is intentionally empty: each partition carries its own headers,
// and including vangui.h here would cause C2572 (default-arg redefinition) when
// MSVC merges global-module declarations from the imported partitions.

module;
// Global module fragment — intentionally empty for the primary interface.
// vangui.cpp (module vangui;) carries the full header includes in its own fragment.

export module vangui;

// Re-export all partition APIs so "import vangui;" gives everything
export import :draw;
export import :core;
export import :widgets;
export import :tables;
export import :docking;
// NOTE: :impl is intentionally NOT export-imported — internal types stay hidden
