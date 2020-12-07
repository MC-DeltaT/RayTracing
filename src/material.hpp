#pragma once

#include <glm/vec3.hpp>


struct Material {
    glm::vec3 colour;
    float roughness;        // In range (0, 1].
    float metalness;        // In range [0, 1].
    glm::vec3 emission;     // Colour that is inherently emitted.
};
