#pragma once

#include <glm/vec3.hpp>


// Light that emanates from a single point in all directions.
struct PointLight {
    glm::vec3 position;
    glm::vec3 colour;
};


// Light that emanates from infinity with rays in a specific direction.
// Default light direction is <0, 0, 1>.
struct DirectionalLight {
    glm::vec3 direction;
    glm::vec3 colour;
};
