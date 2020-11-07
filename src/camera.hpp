#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>


struct Camera {
    glm::vec3 position;
    glm::quat orientation;
    float fov;

    glm::vec3 forward() const;
    glm::vec3 up() const;
};


glm::mat3 pixelToRayTransform(glm::vec3 forward, glm::vec3 up, float fov, unsigned imageWidth, unsigned imageHeight);
