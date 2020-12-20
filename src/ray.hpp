#pragma once

#include "utility/math.hpp"
#include "utility/misc.hpp"
#include "utility/span.hpp"

#include <cassert>
#include <cstddef>
#include <optional>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>


struct Ray {
    // p = origin + t*direction

    glm::vec3 origin;
    glm::vec3 direction;
};


struct RayTriIntersection {
    float rayParam;
    float pointCoord2;      // Barycentric coordinate relative to vertex 2.
    float pointCoord3;      // Barycentric coordinate relative to vertex 3.
};


struct RayMeshIntersection : RayTriIntersection {
    std::size_t mesh;
    std::size_t tri;
};


enum class SurfaceConsideration {
    ALL,
    FRONT_ONLY
};


template<SurfaceConsideration Surfaces>
std::optional<RayTriIntersection> rayTriIntersection(Ray const& ray, PreprocessedTri const& triangle,
    std::pair<float, float> const& paramBounds);

template<>
inline std::optional<RayTriIntersection> rayTriIntersection<SurfaceConsideration::FRONT_ONLY>
        (Ray const& ray, PreprocessedTri const& triangle, std::pair<float, float> const& paramBounds) {
    assert(isUnitVector(ray.direction));
    assert(paramBounds.first >= 0.0f && paramBounds.first <= paramBounds.second);

    auto det = glm::dot(ray.direction, triangle.normal);
    if (det >= -1e-6f) {
        return std::nullopt;
    }
    det = -det;
    auto const invDet = 1.0f / det;
    auto const AO = ray.origin - triangle.v1;
    auto const t = glm::dot(AO, triangle.normal) * invDet;
    if (t > paramBounds.second || t < paramBounds.first) {
        return std::nullopt;
    }
    auto const DAO = glm::cross(AO, ray.direction);
    auto u = glm::dot(triangle.v1ToV3, DAO);
    auto v = -glm::dot(triangle.v1ToV2, DAO);
    if (u >= 0.0f && v >= 0.0f && u + v <= det) {
        u *= invDet;
        v *= invDet;
        return {{t, u, v}};
    }
    else {
        return std::nullopt;
    }
}

template<>
inline std::optional<RayTriIntersection> rayTriIntersection<SurfaceConsideration::ALL>
        (Ray const& ray, PreprocessedTri const& triangle, std::pair<float, float> const& paramBounds) {
    assert(isUnitVector(ray.direction));
    assert(paramBounds.first >= 0.0f && paramBounds.first <= paramBounds.second);

    auto det = glm::dot(ray.direction, triangle.normal);
    if (det >= -1e-6f && det <= 1e-6f) {
        return std::nullopt;
    }
    det = -det;
    auto const invDet = 1.0f / det;
    auto const AO  = ray.origin - triangle.v1;
    auto const t = glm::dot(AO, triangle.normal) * invDet;
    if (t > paramBounds.second || t < paramBounds.first) {
        return std::nullopt;
    }
    auto const DAO = glm::cross(AO, ray.direction);
    auto const u = glm::dot(triangle.v1ToV3, DAO) * invDet;
    auto const v = -glm::dot(triangle.v1ToV2, DAO) * invDet;
    if (u >= 0.0f && v >= 0.0f && u + v <= 1.0f) {
        return {{t, u, v}};
    }
    else {
        return std::nullopt;
    }
}


template<SurfaceConsideration Surfaces>
std::optional<RayMeshIntersection> rayNearestIntersection(Span<PreprocessedTri const> tris,
        Span<IndexRange const> triRanges, Ray const& ray, std::pair<float, float> paramBounds) {
    RayMeshIntersection nearestIntersection{{paramBounds.second}};
    bool hasIntersection = false;
    for (std::size_t meshIndex = 0; meshIndex < triRanges.size(); ++meshIndex) {
        auto const& triRange = triRanges[meshIndex];
        auto const meshTris = tris[triRange];
        for (std::size_t triIndex = 0; triIndex < meshTris.size(); ++triIndex) {
            auto const intersection = rayTriIntersection<Surfaces>(ray, meshTris[triIndex],
                {paramBounds.first, nearestIntersection.rayParam});
            if (intersection) {
                nearestIntersection = {*intersection, meshIndex, triIndex};
                hasIntersection = true;
            }
        }
    }
    if (hasIntersection) {
        return nearestIntersection;
    }
    else {
        return std::nullopt;
    }
}
