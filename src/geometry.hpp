#pragma once

#include "basic_types.hpp"
#include "utility/math.hpp"
#include "utility/span.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <optional>

#include <glm/common.hpp>
#include <glm/geometric.hpp>


// Axis-aligned bounding box.
struct BoundingBox {
    vec3 min;
    vec3 max;
};


struct Line {
    // p = origin + t*direction

    vec3 origin;
    vec3 direction;

    vec3 operator()(float t) const {
        return origin + t * direction;
    }
};


struct Tri {
    vec3 v1;
    vec3 v2;
    vec3 v3;
};


// Tri preprocessed for line intersection calculation.
struct PreprocessedTri {
    vec3 v1;
    vec3 v1ToV2;
    vec3 v1ToV3;
    vec3 normal;
};


enum class SurfaceConsideration {
    ALL,
    FRONT_ONLY
};


struct LineTriIntersection {
    float t;                // Line equation parameter.
    float pointCoord2;      // Barycentric coordinate relative to vertex 2.
    float pointCoord3;      // Barycentric coordinate relative to vertex 3.
};


inline PreprocessedTri preprocessTri(Tri const& tri) {
    auto const v1ToV2 = tri.v2 - tri.v1;
    auto const v1ToV3 = tri.v3 - tri.v1;
    auto const normal = glm::cross(v1ToV2, v1ToV3);
    return {tri.v1, v1ToV2, v1ToV3, normal};
}


inline bool inBox(vec3 const& point, BoundingBox const& box) {
    return point.x >= box.min.x && point.x <= box.max.x
        && point.y >= box.min.y && point.y <= box.max.y
        && point.z >= box.min.z && point.z <= box.max.z;
}


inline BoundingBox computeBoundingBox(Span<vec3 const> points) {
    BoundingBox box{{INFINITY, INFINITY, INFINITY}, {-INFINITY, -INFINITY, -INFINITY}};
    for (auto const& point : points) {
        box.min = glm::min(box.min, point);
        box.max = glm::max(box.max, point);
    }
    return box;
}


template<SurfaceConsideration Surfaces>
std::optional<LineTriIntersection> lineTriIntersection(Line const& line, PreprocessedTri const& tri, float tMin,
    float tMax);

template<>
inline std::optional<LineTriIntersection> lineTriIntersection<SurfaceConsideration::FRONT_ONLY>
        (Line const& line, PreprocessedTri const& tri, float tMin, float tMax) {
    assert(isUnitVector(line.direction));

    auto det = glm::dot(line.direction, tri.normal);
    if (det > -1e-6f) {
        return std::nullopt;
    }
    det = -det;
    auto const invDet = 1.0f / det;
    // det and invDet guaranteed to be > 0
    auto const AO = line.origin - tri.v1;
    auto const t = glm::dot(AO, tri.normal) * invDet;
    if (t > tMax || t < tMin) {
        return std::nullopt;
    }
    auto const DAO = glm::cross(AO, line.direction);
    auto u = glm::dot(tri.v1ToV3, DAO);
    auto v = glm::dot(tri.v1ToV2, DAO);
    if (u >= 0.0f && v <= 0.0f && u - v <= det) {
        u *= invDet;
        v *= -invDet;
        return {{t, u, v}};
    }
    else {
        return std::nullopt;
    }
}

template<>
inline std::optional<LineTriIntersection> lineTriIntersection<SurfaceConsideration::ALL>
        (Line const& line, PreprocessedTri const& tri, float tMin, float tMax) {
    assert(isUnitVector(line.direction));

    auto det = glm::dot(line.direction, tri.normal);
    if (std::abs(det) < 1e-6f) {
        return std::nullopt;
    }
    auto const invDet = -1.0f / det;
    auto const AO  = line.origin - tri.v1;
    auto const t = glm::dot(AO, tri.normal) * invDet;
    if (t > tMax || t < tMin) {
        return std::nullopt;
    }
    auto const DAO = glm::cross(AO, line.direction);
    auto const u = glm::dot(tri.v1ToV3, DAO) * invDet;
    auto v = glm::dot(tri.v1ToV2, DAO) * invDet;
    if (u >= 0.0f && v <= 0.0f && u - v <= 1.0f) {
        v = -v;
        return {{t, u, v}};
    }
    else {
        return std::nullopt;
    }
}


inline bool lineIntersectsBox(Line const& line, BoundingBox const& box) {
    auto const linePlaneIntersection = [](float nDotD, float p0MinusL0DotN) -> std::optional<float> {
        if (std::abs(nDotD) < 1e-6f) {
            return std::nullopt;
        }
        else {
            auto const t = p0MinusL0DotN / nDotD;
            if (t >= 0.0f) {
                return {t};
            }
            else {
                return std::nullopt;
            }
        }
    };

    assert(isUnitVector(line.direction));

    // -X face
    if (auto const t = linePlaneIntersection(-line.direction.x, line.origin.x - box.min.x)) {
        auto const y = line.origin.y + *t * line.direction.y;
        if (y >= box.min.y && y <= box.max.y) {
            auto const z = line.origin.z + *t * line.direction.z;
            if (z >= box.min.z && z <= box.max.z) {
                return true;
            }
        }
    }
    // +X face
    if (auto const t = linePlaneIntersection(line.direction.x, box.max.x - line.origin.x)) {
        auto const y = line.origin.y + *t * line.direction.y;
        if (y >= box.min.y && y <= box.max.y) {
            auto const z = line.origin.z + *t * line.direction.z;
            if (z >= box.min.z && z <= box.max.z) {
                return true;
            }
        }
    }
    // -Y face
    if (auto const t = linePlaneIntersection(-line.direction.y, line.origin.y - box.min.y)) {
        auto const x = line.origin.x + *t * line.direction.x;
        if (x >= box.min.x && x <= box.max.x) {
            auto const z = line.origin.z + *t * line.direction.z;
            if (z >= box.min.z && z <= box.max.z) {
                return true;
            }
        }
    }
    // +Y face
    if (auto const t = linePlaneIntersection(line.direction.y, box.max.y - line.origin.y)) {
        auto const x = line.origin.x + *t * line.direction.x;
        if (x >= box.min.x && x <= box.max.x) {
            auto const z = line.origin.z + *t * line.direction.z;
            if (z >= box.min.z && z <= box.max.z) {
                return true;
            }
        }
    }
    // -Z face
    if (auto const t = linePlaneIntersection(-line.direction.z, line.origin.z - box.min.z)) {
        auto const x = line.origin.x + *t * line.direction.x;
        if (x >= box.min.x && x <= box.max.x) {
            auto const y = line.origin.y + *t * line.direction.y;
            if (y >= box.min.y && y <= box.max.y) {
                return true;
            }
        }
    }
    // +Z face
    if (auto const t = linePlaneIntersection(line.direction.z, box.max.z - line.origin.z)) {
        auto const x = line.origin.x + *t * line.direction.x;
        if (x >= box.min.x && x <= box.max.x) {
            auto const y = line.origin.y + *t * line.direction.y;
            if (y >= box.min.y && y <= box.max.y) {
                return true;
            }
        }
    }
    return false;
}


inline bool triIntersectsBox(Tri tri, BoundingBox const& box) {
    // T. Akenine-Moller, "Fast 3D triangle-box overlap testing", 2001.

    auto const intervalsDisjoint = [](float i1Min, float i1Max, float i2Min, float i2Max) {
        return i1Max < i2Min || i2Max < i1Min;
    };

    // Translate tri and box such that box centre is at origin. Simplifies/optimises checks of the box vertices.
    vec3 boxRadius;
    boxRadius.x = (box.max.x - box.min.x) / 2.0f;
    vec3 boxCentre;
    boxCentre.x = box.min.x + boxRadius.x;
    tri.v1.x -= boxCentre.x;
    tri.v2.x -= boxCentre.x;
    tri.v3.x -= boxCentre.x;

    // Test box normals.
    // X-axis.
    {
        auto const triV1Proj = tri.v1.x;
        auto const triV2Proj = tri.v2.x;
        auto const triV3Proj = tri.v3.x;
        auto const [triProjMin, triProjMax] = std::minmax({triV1Proj, triV2Proj, triV3Proj});
        auto const boxProjMin = -boxRadius.x;
        auto const boxProjMax = boxRadius.x;
        if (intervalsDisjoint(triProjMin, triProjMax, boxProjMin, boxProjMax)) {
            return false;
        }
    }
    boxRadius.y = (box.max.y - box.min.y) / 2.0f;
    boxCentre.y = box.min.y + boxRadius.y;
    tri.v1.y -= boxCentre.y;
    tri.v2.y -= boxCentre.y;
    tri.v3.y -= boxCentre.y;
    // Y-axis.
    {
        auto const triV1Proj = tri.v1.y;
        auto const triV2Proj = tri.v2.y;
        auto const triV3Proj = tri.v3.y;
        auto const [triProjMin, triProjMax] = std::minmax({triV1Proj, triV2Proj, triV3Proj});
        auto const boxProjMin = -boxRadius.y;
        auto const boxProjMax = boxRadius.y;
        if (intervalsDisjoint(triProjMin, triProjMax, boxProjMin, boxProjMax)) {
            return false;
        }
    }
    boxRadius.z = (box.max.z - box.min.z) / 2.0f;
    boxCentre.z = box.min.z + boxRadius.z;
    tri.v1.z -= boxCentre.z;
    tri.v2.z -= boxCentre.z;
    tri.v3.z -= boxCentre.z;
    // Z-axis.
    {
        auto const triV1Proj = tri.v1.z;
        auto const triV2Proj = tri.v2.z;
        auto const triV3Proj = tri.v3.z;
        auto const [triProjMin, triProjMax] = std::minmax({triV1Proj, triV2Proj, triV3Proj});
        auto const boxProjMin = -boxRadius.z;
        auto const boxProjMax = boxRadius.z;
        if (intervalsDisjoint(triProjMin, triProjMax, boxProjMin, boxProjMax)) {
            return false;
        }
    }

    // Test box normals cross tri edges.
    auto const triEdge1 = tri.v2 - tri.v1;
    // Box normal 1 cross tri edge 1: (0, -triEdge1.z, triEdge1.y)
    {
        auto const triV1Proj = tri.v1.z * tri.v2.y - tri.v1.y * tri.v2.z;
        // triV2Proj = triV1Proj
        auto const triV3Proj = tri.v3.z * triEdge1.y - tri.v3.y * triEdge1.z;
        auto const [triProjMin, triProjMax] = std::minmax(triV1Proj, triV3Proj);

        auto const boxProjMax = std::abs(boxRadius.y * triEdge1.z) + std::abs(boxRadius.z * triEdge1.y);
        auto const boxProjMin = -boxProjMax;

        if (intervalsDisjoint(triProjMin, triProjMax, boxProjMin, boxProjMax)) {
            return false;
        }
    }
    // Box normal 2 cross tri edge 1: (triEdge1.z, 0, -triEdge1.x)
    {
        auto const triV1Proj = tri.v1.x * tri.v2.z - tri.v1.z * tri.v2.x;
        // triV2Proj = triV1Proj
        auto const triV3Proj = tri.v3.x * triEdge1.z - tri.v3.z * triEdge1.x;
        auto const [triProjMin, triProjMax] = std::minmax(triV1Proj, triV3Proj);

        auto const boxProjMax = std::abs(boxRadius.x * triEdge1.z) + std::abs(boxRadius.z * triEdge1.x);
        auto const boxProjMin = -boxProjMax;

        if (intervalsDisjoint(triProjMin, triProjMax, boxProjMin, boxProjMax)) {
            return false;
        }
    }
    // Box normal 3 cross tri edge 1: (-triEdge1.y, triEdge1.x, 0)
    {
        auto const triV1Proj = tri.v1.y * tri.v2.x - tri.v1.x * tri.v2.y;
        // triV2Proj = triV1Proj
        auto const triV3Proj = tri.v3.y * triEdge1.x - tri.v3.x * triEdge1.y;
        auto const [triProjMin, triProjMax] = std::minmax(triV1Proj, triV3Proj);

        auto const boxProjMax = std::abs(boxRadius.x * triEdge1.y) + std::abs(boxRadius.y * triEdge1.x);
        auto const boxProjMin = -boxProjMax;

        if (intervalsDisjoint(triProjMin, triProjMax, boxProjMin, boxProjMax)) {
            return false;
        }
    }
    auto const triEdge2 = tri.v3 - tri.v1;
    // Box normal 1 cross tri edge 2: (0, -triEdge2.z, triEdge2.y)
    {
        auto const triV1Proj = tri.v1.z * tri.v3.y - tri.v1.y * tri.v3.z;
        auto const triV2Proj = tri.v2.z * triEdge2.y - tri.v2.y * triEdge2.z;
        // triV3Proj = triV1Proj
        auto const [triProjMin, triProjMax] = std::minmax(triV1Proj, triV2Proj);

        auto const boxProjMax = std::abs(boxRadius.y * triEdge2.z) + std::abs(boxRadius.z * triEdge2.y);
        auto const boxProjMin = -boxProjMax;

        if (intervalsDisjoint(triProjMin, triProjMax, boxProjMin, boxProjMax)) {
            return false;
        }
    }
    // Box normal 2 cross tri edge 2: (triEdge2.z, 0, -triEdge2.x)
    {
        auto const triV1Proj = tri.v1.x * tri.v3.z - tri.v1.z * tri.v3.x;
        auto const triV2Proj = tri.v2.x * triEdge2.z - tri.v2.z * triEdge2.x;
        // triV3Proj = triV1Proj
        auto const [triProjMin, triProjMax] = std::minmax(triV1Proj, triV2Proj);

        auto const boxProjMax = std::abs(boxRadius.x * triEdge2.z) + std::abs(boxRadius.z * triEdge2.x);
        auto const boxProjMin = -boxProjMax;

        if (intervalsDisjoint(triProjMin, triProjMax, boxProjMin, boxProjMax)) {
            return false;
        }
    }
    // Box normal 3 cross tri edge 2: (-triEdge2.y, triEdge2.x, 0)
    {
        auto const triV1Proj = tri.v1.y * tri.v3.x - tri.v1.x * tri.v3.y;
        auto const triV2Proj = tri.v2.y * triEdge2.x - tri.v2.x * triEdge2.y;
        // triV3Proj = triV1Proj
        auto const [triProjMin, triProjMax] = std::minmax(triV1Proj, triV2Proj);

        auto const boxProjMax = std::abs(boxRadius.x * triEdge2.y) + std::abs(boxRadius.y * triEdge2.x);
        auto const boxProjMin = -boxProjMax;

        if (intervalsDisjoint(triProjMin, triProjMax, boxProjMin, boxProjMax)) {
            return false;
        }
    }
    auto const triEdge3 = tri.v3 - tri.v2;
    // Box normal 1 cross tri edge 3: (0, -triEdge3.z, triEdge3.y)
    {
        auto const triV1Proj = tri.v1.z * triEdge3.y - tri.v1.y * triEdge3.z;
        auto const triV2Proj = tri.v2.z * tri.v3.y - tri.v2.y * tri.v3.z;
        // triV3Proj = triV2Proj
        auto const [triProjMin, triProjMax] = std::minmax(triV1Proj, triV2Proj);

        auto const boxProjMax = std::abs(boxRadius.y * triEdge3.z) + std::abs(boxRadius.z * triEdge3.y);
        auto const boxProjMin = -boxProjMax;

        if (intervalsDisjoint(triProjMin, triProjMax, boxProjMin, boxProjMax)) {
            return false;
        }
    }
    // Box normal 2 cross tri edge 3: (triEdge3.z, 0, -triEdge3.x)
    {
        auto const triV1Proj = tri.v1.x * triEdge3.z - tri.v1.z * triEdge3.x;
        auto const triV2Proj = tri.v2.x * tri.v3.z - tri.v2.z * tri.v3.x;
        // triV3Proj = triV2Proj
        auto const [triProjMin, triProjMax] = std::minmax(triV1Proj, triV2Proj);

        auto const boxProjMax = std::abs(boxRadius.x * triEdge3.z) + std::abs(boxRadius.z * triEdge3.x);
        auto const boxProjMin = -boxProjMax;

        if (intervalsDisjoint(triProjMin, triProjMax, boxProjMin, boxProjMax)) {
            return false;
        }
    }
    // Box normal 3 cross tri edge 3: (-triEdge3.y, triEdge3.x, 0)
    {
        auto const triV1Proj = tri.v1.y * triEdge3.x - tri.v1.x * triEdge3.y;
        auto const triV2Proj = tri.v2.y * tri.v3.x - tri.v2.x * tri.v3.y;
        // triV3Proj = triV2Proj
        auto const [triProjMin, triProjMax] = std::minmax(triV1Proj, triV2Proj);

        auto const boxProjMax = std::abs(boxRadius.x * triEdge3.y) + std::abs(boxRadius.y * triEdge3.x);
        auto const boxProjMin = -boxProjMax;

        if (intervalsDisjoint(triProjMin, triProjMax, boxProjMin, boxProjMax)) {
            return false;
        }
    }

    // Test tri normal.
    {
        auto const triNormal = glm::cross(triEdge1, triEdge2);
        // Projection is the same for all tri vertices.
        auto const triProj = glm::dot(tri.v1, triNormal);

        auto const boxProjMax = std::abs(triNormal.x * boxRadius.x) + std::abs(triNormal.y * boxRadius.y)
            + std::abs(triNormal.z * boxRadius.z);
        auto const boxProjMin = -boxProjMax;

        if (intervalsDisjoint(triProj, triProj, boxProjMin, boxProjMax)) {
            return false;
        }
    }

    return true;
}
