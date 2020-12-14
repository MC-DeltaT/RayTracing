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
    T result = 1;
    for (unsigned i = 0; i < power; ++i) {
        result *= val;
    }
    return val;
}


inline bool isNormalised(float val) {
    return val >= 0.0f && val <= 1.0f;
}


inline bool isUnitVector(glm::vec3 const& vec) {
    return std::abs(glm::length(vec) - 1.0f) <= 1e-3f;
}


inline std::pair<glm::vec3, glm::vec3> orthonormalBasis(glm::vec3 const& vec) {
    assert(isUnitVector(vec));
    glm::vec3 tmp{0.56863665f, -0.77215318f, 0.28360506f};      // Arbitrary vector.
    auto dot = glm::dot(tmp, vec);
    // This branch will almost never be taken.
    if (std::abs(1.0f - std::abs(dot)) < 1e-3f) {
        tmp = {0.56863665f, 0.77215318f, 0.28360506f};
        dot = glm::dot(tmp, vec);
    }
    auto const perpendicular1 = tmp - dot * vec;
    auto const perpendicular2 = glm::cross(vec, perpendicular1);
    return {perpendicular1, perpendicular2};
}
