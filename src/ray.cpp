#include "ray.hpp"

#include <optional>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>


std::optional<RayIntersection> rayTriIntersection(Ray ray, Triangle triangle) {
    auto const edge1 = triangle.v2 - triangle.v1;
    auto const edge2 = triangle.v3 - triangle.v1;
    auto const normal = glm::cross(edge1, edge2);
    auto const det = -glm::dot(ray.direction, normal);
    auto const invDet = 1.0f / det;
    auto const AO = ray.origin - triangle.v1;
    auto const DAO = glm::cross(AO, ray.direction);
    auto const u = glm::dot(edge2, DAO) * invDet;
    auto const v = -glm::dot(edge1, DAO) * invDet;
    auto const t = glm::dot(AO, normal) * invDet;
    if (det >= 1e-6f && t >= 1e-6f && u >= 0.0f && v >= 0.0f && u + v <= 1.0f) {
        return {{t, {1.0f - u - v, u, v}}};
    }
    else {
        return std::nullopt;
    }
}
