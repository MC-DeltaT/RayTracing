#pragma once

#include "utility.hpp"

#include <cassert>
#include <cstddef>
#include <optional>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>


struct Ray {
    // p = origin + t*direction

    glm::vec3 origin;
    glm::vec3 direction;
};


struct PreprocessedTri {
    glm::vec3 v1;
    glm::vec3 v1ToV2;
    glm::vec3 v1ToV3;
    glm::vec3 normal;
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


inline PreprocessedTri preprocessTri(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
    auto const v1ToV2 = v2 - v1;
    auto const v1ToV3 = v3 - v1;
    auto const normal = glm::cross(v1ToV2, v1ToV3);
    return {v1, v1ToV2, v1ToV3, normal};
}


inline void preprocessTris(Span<glm::vec3 const> vertexPositions, Span<IndexRange const> vertexRanges,
        Span<MeshTri const> tris, PermutedSpan<IndexRange const> triRanges, std::vector<PreprocessedTri>& resultTris,
        std::vector<IndexRange>& resultTriRanges) {
    assert(vertexRanges.size() == triRanges.size());

    auto const instanceCount = vertexRanges.size();

    {
        std::size_t triCount = 0;
        for (std::size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex) {
            auto const& triRange = triRanges[instanceIndex];
            auto const instanceTris = tris[triRange];
            triCount += instanceTris.size();
        }
        resultTris.resize(triCount);
    }
    resultTriRanges.resize(instanceCount);
    
    std::size_t trisOffset = 0;
    for (std::size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex) {
        auto const& vertexRange = vertexRanges[instanceIndex];
        auto const instanceVertexPositions = vertexPositions[vertexRange];
        auto const& triRange = triRanges[instanceIndex];
        auto const instanceTris = tris[triRange];
        for (std::size_t i = 0; i < instanceTris.size(); ++i) {
            auto const& tri = instanceTris[i];
            auto const preprocessedTri = preprocessTri(instanceVertexPositions[tri.i1], instanceVertexPositions[tri.i2],
                    instanceVertexPositions[tri.i3]);
            resultTris[trisOffset + i] = preprocessedTri;
        }
        resultTriRanges[instanceIndex] = {trisOffset, instanceTris.size()};
        trisOffset += instanceTris.size();
    }
}


template<bool ConsiderBackFacing>
std::optional<RayTriIntersection> rayTriIntersection(Ray const& ray, PreprocessedTri const& triangle,
    std::pair<float, float> const& paramBounds);

template<>
inline std::optional<RayTriIntersection> rayTriIntersection<false>(Ray const& ray, PreprocessedTri const& triangle,
        std::pair<float, float> const& paramBounds) {
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
inline std::optional<RayTriIntersection> rayTriIntersection<true>(Ray const& ray, PreprocessedTri const& triangle,
        std::pair<float, float> const& paramBounds) {
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


template<bool ConsiderBackFacing>
std::optional<RayMeshIntersection> rayNearestIntersection(Span<PreprocessedTri const> tris,
        Span<IndexRange const> triRanges, Ray const& ray, std::pair<float, float> paramBounds) {
    RayMeshIntersection nearestIntersection{{paramBounds.second}};
    bool hasIntersection = false;
    for (std::size_t meshIndex = 0; meshIndex < triRanges.size(); ++meshIndex) {
        auto const& triRange = triRanges[meshIndex];
        auto const meshTris = tris[triRange];
        for (std::size_t triIndex = 0; triIndex < meshTris.size(); ++triIndex) {
            auto const intersection = rayTriIntersection<ConsiderBackFacing>(ray, meshTris[triIndex],
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


template<bool ConsiderBackFacing>
bool rayIntersectsAny(Span<PreprocessedTri const> tris, Span<IndexRange const> triRanges, Ray ray,
        std::pair<float, float> paramBounds) {
    for (auto const& triRange : triRanges) {
        auto const meshTris = tris[triRange];
        for (auto const& tri : meshTris) {
            if (rayTriIntersection<ConsiderBackFacing>(ray, tri, paramBounds)) {
                return true;
            }
        }
    }
    return false;
}
