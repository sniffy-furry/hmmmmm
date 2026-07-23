#pragma once
#include "Common.h"

namespace nfsmw {

/// Editor camera for the Z-up NFSMW world. Orbit/pan/zoom plus Unreal-style
/// fly navigation.
///
/// Controls:
///   MMB drag (or Alt+LMB)   orbit around the target
///   Shift+MMB drag          pan (screen-accurate)
///   Scroll wheel            zoom toward the mouse cursor
///   RMB drag                look around (first-person turn)
///   RMB + W/A/S/D + Q/E     fly (Shift = 4x speed)
///   F                       frame selection
struct OrbitCamera {
    glm::vec3 target  { 0.0f, 0.0f, 0.0f };
    float     yaw     { 45.0f   };   ///< degrees around the Z (up) axis
    float     pitch   { 35.0f   };   ///< degrees above the horizon
    float     radius  { 150.0f  };   ///< distance from target
    float     nearZ   { 0.25f   };
    float     farZ    { 30000.0f };  ///< Rockport spans ~+-10 km
    float     fovDeg  { 60.0f   };

    // Input state (set per-frame by the UI layer)
    struct InputState {
        bool  orbiting = false;
        bool  panning  = false;
        bool  looking  = false;        ///< RMB held: first-person turn
        float deltaX   = 0.0f;
        float deltaY   = 0.0f;
        float scroll   = 0.0f;
        float viewportH = 900.0f;      ///< pixels, for screen-accurate panning
        glm::vec3 moveLocal { 0.0f };  ///< fly move (right, forward, up), pre-scaled
        glm::vec3 zoomTarget { 0.0f }; ///< world point under the cursor
        bool  hasZoomTarget = false;
    };

    glm::vec3 EyePos() const;
    glm::vec3 Forward() const;
    glm::mat4 View()   const;
    /// Inverse of View(). Cached and only recomputed when the camera moves
    /// (Issue #4: avoids a glm::inverse(mat4) per ray-cast / per frame).
    glm::mat4 ViewInv() const;
    glm::mat4 Proj(float aspect) const;

    /// Apply one frame of input.
    void Update(const InputState& input);

    /// Reframe to look at a world-space AABB.
    void FrameAABB(const AABB& box);

    /// Zoom so that the entire scene fits.
    void FrameAll(const AABB& sceneBox);

private:
    // Issue #4: cache the view matrix and its inverse so ScreenToWorldRay()
    // and the sky shader don't pay for glm::inverse() on every call.
    mutable glm::mat4 viewCache_{1.0f};
    mutable glm::mat4 viewInvCache_{1.0f};
    mutable bool      viewDirty_ = true;
    void RefreshViewCache() const;
};

} // namespace nfsmw
