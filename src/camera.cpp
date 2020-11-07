#include "camera.hpp"

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>


glm::vec3 Camera::forward() const {
    return orientation * glm::vec3{0.0f, 0.0f, 1.0f};
}

glm::vec3 Camera::up() const {
    return orientation * glm::vec3{0.0f, 1.0f, 0.0f};
}


glm::mat3 pixelToRayTransform(glm::vec3 forward, glm::vec3 up, float fov, unsigned imageWidth, unsigned imageHeight) {
    auto const u = glm::normalize(glm::cross(forward, up));
    auto const v = glm::normalize(glm::cross(forward, u));
    auto const o = imageWidth / (2.0f * std::tan(fov / 2.0f)) * glm::normalize(forward)
        - imageWidth / 2.0f * u - imageHeight / 2.0f * v;
    return {u, v, o};
}
