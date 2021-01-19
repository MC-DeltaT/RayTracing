#pragma once

#include <cassert>
#include <cmath>
#include <utility>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>


template<typename T>
constexpr T square(T val) {
    return val * val;
}


template<typename T>
constexpr T iPow(T val, unsigned power) {
    T result{1};
    for (unsigned i = 0; i < power; ++i) {
        result *= val;
    }
    return result;
}


inline bool isNormalised(float val) {
    return val >= 0.0f && val <= 1.0f;
}


inline bool isUnitVector(glm::vec3 vec) {
    return std::abs(glm::length(vec) - 1.0f) <= 1e-3f;
}


inline std::pair<glm::vec3, glm::vec3> orthonormalBasis(glm::vec3 vec) {
    assert(isUnitVector(vec));
    glm::vec3 vec2{0.56863665f, -0.77215318f, 0.28360506f};     // Arbitrary unit vector.
    auto dot = glm::dot(vec, vec2);
    // This branch will almost never be taken.
    if (std::abs(1.0f - std::abs(dot)) < 1e-3f) {
        // If original dot product is 0, then dot product with this vector cannot be 0.
        vec2 = {0.56863665f, 0.77215318f, 0.28360506f};
        dot = glm::dot(vec, vec2);
    }
    auto const perpendicular1 = glm::normalize(vec2 - dot * vec);
    // vec and perpendicular1 are perpendicular unit vectors, so cross product must be unit vector too.
    auto const perpendicular2 = glm::cross(vec, perpendicular1);
    return {perpendicular1, perpendicular2};
}
