#include "renderer/OrbitCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace nfsmw {

namespace {
glm::vec3 DirFromAngles(float yawDeg, float pitchDeg) {
    const float y = glm::radians(yawDeg);
    const float p = glm::radians(pitchDeg);
    return { std::cos(p) * std::cos(y),
             std::cos(p) * std::sin(y),
             std::sin(p) };
}
} // namespace

glm::vec3 OrbitCamera::EyePos() const {
    return target + radius * DirFromAngles(yaw, pitch);
}

glm::vec3 OrbitCamera::Forward() const {
    return glm::normalize(target - EyePos());
}

glm::mat4 OrbitCamera::View() const {
    if (viewDirty_) RefreshViewCache();
    return viewCache_;
}

glm::mat4 OrbitCamera::ViewInv() const {
    if (viewDirty_) RefreshViewCache();
    return viewInvCache_;
}

void OrbitCamera::RefreshViewCache() const {
    viewCache_    = glm::lookAt(EyePos(), target, glm::vec3(0.0f, 0.0f, 1.0f));
    viewInvCache_ = glm::inverse(viewCache_);
    viewDirty_    = false;
}

glm::mat4 OrbitCamera::Proj(float aspect) const {
    return glm::perspective(glm::radians(fovDeg), aspect, nearZ, farZ);
}

void OrbitCamera::Update(const InputState& input) {
    const float orbitSensitivity = 0.35f;
    const float lookSensitivity  = 0.16f;

    // Issue #35: compute the eye position once up-front and reuse it across
    // the input branches below instead of calling EyePos() (which involves
    // trig) repeatedly. Only re-derived if the look branch moves the target.
    glm::vec3 eye = EyePos();

    if (input.looking) {
        // First-person turn: the eye stays put, the target swings around it.
        yaw   -= input.deltaX * lookSensitivity;
        pitch += input.deltaY * lookSensitivity;
        pitch  = std::clamp(pitch, -89.0f, 89.0f);
        target = eye - radius * DirFromAngles(yaw, pitch);
    } else if (input.panning) {
        // Screen-accurate pan: a pixel of mouse motion moves the world by a
        // pixel at the target's depth.
        const float worldPerPixel =
            2.0f * radius * std::tan(glm::radians(fovDeg) * 0.5f) /
            std::max(input.viewportH, 1.0f);
        const glm::vec3 fwd   = glm::normalize(target - eye);
        // Issue #42 fix: guard against degenerate cross product when looking
        // straight up or down (fwd parallel to Z). Fall back to world X axis.
        const float upDot = std::abs(glm::dot(fwd, glm::vec3(0, 0, 1)));
        const glm::vec3 worldUp = (upDot > 0.99f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 0, 1);
        const glm::vec3 right = glm::normalize(glm::cross(fwd, worldUp));
        const glm::vec3 up    = glm::normalize(glm::cross(right, fwd));
        target -= right * input.deltaX * worldPerPixel;
        target += up    * input.deltaY * worldPerPixel;
    } else if (input.orbiting) {
        yaw   -= input.deltaX * orbitSensitivity;
        pitch += input.deltaY * orbitSensitivity;
        pitch  = std::clamp(pitch, -89.0f, 89.0f);
    }

    // Fly movement (already speed/dt-scaled by the caller).
    if (input.moveLocal != glm::vec3(0.0f)) {
        // The look branch above may have moved `target` (and thus changed
        // the eye-to-target direction); recompute eye only in that case.
        if (input.looking) eye = EyePos();
        const glm::vec3 fwd   = glm::normalize(target - eye);
        // Issue #42 fix: same degenerate-cross guard as the panning branch.
        const float flyUpDot = std::abs(glm::dot(fwd, glm::vec3(0, 0, 1)));
        const glm::vec3 flyWorldUp = (flyUpDot > 0.99f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 0, 1);
        const glm::vec3 right = glm::normalize(glm::cross(fwd, flyWorldUp));
        target += right * input.moveLocal.x
                + fwd   * input.moveLocal.y
                + glm::vec3(0, 0, 1) * input.moveLocal.z;
    }

    if (input.scroll != 0.0f) {
        const float oldRadius = radius;
        radius *= std::pow(1.0f / 1.18f, input.scroll);
        radius  = std::clamp(radius, 0.5f, farZ * 0.6f);

        // Zoom toward the cursor: pull the target a matching fraction of the
        // way to the point under the mouse so you can dive into any spot.
        if (input.hasZoomTarget && input.scroll > 0.0f && oldRadius > 0.0f) {
            const float k = 1.0f - radius / oldRadius;   // 0..1
            target += (input.zoomTarget - target) * k;
        }
    }

    // Issue #4: any change above may have moved the camera; invalidate the
    // cached view matrix / inverse so the next View()/ViewInv() call rebuilds it.
    viewDirty_ = true;
}

void OrbitCamera::FrameAABB(const AABB& box) {
    if (!box.Valid()) return;
    target = box.Center();
    float diag = glm::length(box.Extents()) * 2.2f;
    radius = std::clamp(diag, 2.0f, farZ * 0.5f);
    viewDirty_ = true;
}

void OrbitCamera::FrameAll(const AABB& sceneBox) {
    FrameAABB(sceneBox);
}

} // namespace nfsmw
