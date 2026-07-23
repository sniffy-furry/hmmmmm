// Precompiled header for vangui_module target.
// Processed ONCE before each module file is compiled.
// vangui.h and vangui_internal.h set their #pragma once state here,
// preventing re-declaration of default-argument functions when each
// partition's global fragment also includes these headers —
// the second (and later) inclusions are no-ops thanks to #pragma once.
// This eliminates C2572 (default-arg redefinition) across module partitions.

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef VANGUI_DEFINE_MATH_OPERATORS
#define VANGUI_DEFINE_MATH_OPERATORS
#endif

#include "vangui.h"
#ifndef VANGUI_DISABLE
#include "vangui_internal.h"
#endif
