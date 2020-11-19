#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <glm/vec3.hpp>


struct Pixel {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};


inline std::uint8_t linearTo8BitSRGB(float linear) {
    float result;
    if (linear <= 0.0031308f) {
        result = linear * 12.92f;
    }
    else {
        result = 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
    }
    return static_cast<std::uint8_t>(std::clamp(result, 0.0f, 1.0f) * 255.0f);
}

inline Pixel linearTo8BitSRGB(glm::vec3 linear) {
    return {linearTo8BitSRGB(linear.r), linearTo8BitSRGB(linear.g), linearTo8BitSRGB(linear.b)};
}
