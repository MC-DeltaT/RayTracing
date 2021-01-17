#pragma once

#include "basic_types.hpp"
#include "utility/math.hpp"

#include <cassert>
#include <cmath>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>


struct Camera {
    PackedFVec3 position;
    glm::quat orientation;
    float fov;

    PackedFVec3 forward() const {
        return orientation * PackedFVec3{0.0f, 0.0f, 1.0f};
    }

    PackedFVec3 down() const {
        return orientation * PackedFVec3{0.0f, -1.0f, 0.0f};
    }

    PackedFVec3 right() const {
        return orientation * PackedFVec3{-1.0f, 0.0f, 0.0f};
    }
};


inline glm::mat3 pixelToRayTransform(PackedFVec3 const& forward, PackedFVec3 const& down, PackedFVec3 const& right,
        float fov, unsigned imageWidth, unsigned imageHeight) {
    assert(isUnitVector(forward));
    assert(isUnitVector(down));
    assert(isUnitVector(right));

    // Using double precision to avoid issues with FP optimisation when single precision is used.
    glm::dvec3 dforward = forward;
    glm::dvec3 ddown = down;
    glm::dvec3 dright = right;
    auto const o = (imageWidth / std::tan(fov / 2.0) * dforward - static_cast<double>(imageWidth) * dright
        - static_cast<double>(imageHeight) * ddown) / 2.0;
    return {right, down, o};
}
