#pragma once

#include <cassert>
#include <optional>

#include <glm/geometric.hpp>
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
    float pointCoord2;      // Barycentric coordinate relative to vertex 2.
    float pointCoord3;      // Barycentric coordinate relative to vertex 3.
};


inline std::optional<RayIntersection> rayTriIntersection(Ray ray, Triangle triangle) {
    auto const edge1 = triangle.v2 - triangle.v1;
    auto const edge2 = triangle.v3 - triangle.v1;
    auto const normal = glm::cross(edge1, edge2);
    auto det = glm::dot(ray.direction, normal);
    if (det > -1e-6f) {
        return std::nullopt;
    }
    det = -det;
    assert(det > 0);
    auto const invDet = 1.0f / det;
    assert(invDet > 0);
    auto const AO = ray.origin - triangle.v1;
    auto t = glm::dot(AO, normal);
    if (t < 1e-6f) {
        return std::nullopt;
    }
    auto const DAO = glm::cross(AO, ray.direction);
    auto u = glm::dot(edge2, DAO);
    auto v = -glm::dot(edge1, DAO);
    if (u >= 0.0f && v >= 0.0f && u + v <= det) {
        u *= invDet;
        v *= invDet;
        t *= invDet;
        return {{t, u, v}};
    }
    else {
        return std::nullopt;
    }
}