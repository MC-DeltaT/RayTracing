#pragma once

#include <optional>

#include <glm/vec3.hpp>


struct Ray {
    // p = origin + t*direction

    glm::vec3 origin;
    glm::vec3 direction;
};


struct Triangle {
    glm::vec3 v1;
    glm::vec3 v2;
    glm::vec3 v3;
};


struct RayIntersection {
    float rayParam;
    glm::vec3 pointCoords;      // Barycentric coordinates.
};


std::optional<RayIntersection> rayTriIntersection(Ray ray, Triangle triangle);
