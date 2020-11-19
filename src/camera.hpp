#pragma once

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>


struct Camera {
    glm::vec3 position;
    glm::quat orientation;
    float fov;

    glm::vec3 forward() const {
        return orientation * glm::vec3{0.0f, 0.0f, 1.0f};
    }

    glm::vec3 up() const {
        return orientation * glm::vec3{0.0f, 1.0f, 0.0f};
    }
};


inline glm::mat3 pixelToRayTransform(glm::vec3 forward, glm::vec3 up, float fov, unsigned imageWidth,
        unsigned imageHeight) {
    auto const u = glm::cross(forward, up);
    auto const v = glm::cross(forward, u);
    auto const o = imageWidth / (2.0f * std::tan(fov / 2.0f)) * forward - imageWidth / 2.0f * u - imageHeight / 2.0f * v;
    return {u, v, o};
}
