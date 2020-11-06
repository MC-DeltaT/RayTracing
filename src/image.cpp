#include "image.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>


static std::uint8_t linearTo8BitSRGB(float linear) {
    float result;
    if (linear <= 0.0031308f) {
        result = linear * 12.92f;
    }
    else {
        result = 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
    }
    return static_cast<std::uint8_t>(std::clamp(result, 0.0f, 1.0f) * 255.0f);
}

static Pixel linearTo8BitSRGB(glm::vec3 linear) {
    return {linearTo8BitSRGB(linear.r), linearTo8BitSRGB(linear.g), linearTo8BitSRGB(linear.b)};
}


void linearTo8BitSRGB(Span<glm::vec3> linear, Span<Pixel> srgb) {
    assert(linear.size() == srgb.size());
    std::transform(linear.begin(), linear.end(), srgb.begin(), [](auto value) {
        return linearTo8BitSRGB(value);
    });
}
