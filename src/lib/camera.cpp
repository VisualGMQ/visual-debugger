#include "camera.hpp"

Camera Camera::CreatePerspective(const Frustum& frustum) {
    return Camera(frustum, glm::perspective(frustum.fov, frustum.aspect, frustum.near, frustum.far));
}
