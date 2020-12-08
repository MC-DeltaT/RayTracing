#pragma once

#include "utility/span.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
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
    return static_cast<std::uint8_t>(std::clamp(result * 255.0f, 0.0f, 255.0f));
}

inline Pixel linearTo8BitSRGB(glm::vec3 linear) {
    return {linearTo8BitSRGB(linear.r), linearTo8BitSRGB(linear.g), linearTo8BitSRGB(linear.b)};
}


template<unsigned Radius>
void medianFilter(Span<Pixel const> image, std::size_t imageWidth, Span<Pixel> result) {
    assert(image.size() % imageWidth == 0);
    assert(image.data() != result.data());
    assert(image.size() == result.size());

    std::ptrdiff_t const imageHeightS = image.size() / imageWidth;
    std::ptrdiff_t const imageWidthS = imageWidth;
    constexpr std::ptrdiff_t radiusS = Radius;

    std::array<std::uint8_t, square(2 * Radius + 1)> rs;
    std::array<std::uint8_t, square(2 * Radius + 1)> gs;
    std::array<std::uint8_t, square(2 * Radius + 1)> bs;
    for (std::ptrdiff_t i = 0; i < imageHeightS; ++i) {
        for (std::ptrdiff_t j = 0; j < imageWidthS; ++j) {
            unsigned neighbours = 0;
            for (std::ptrdiff_t di = -radiusS; di <= radiusS; ++di) {
                for (std::ptrdiff_t dj = -radiusS; dj <= radiusS; ++dj) {
                    auto const i_ = i + di;
                    auto const j_ = j + dj;
                    if (i_ >= 0 && j_ >= 0 && i_ < imageHeightS && j_ < imageWidthS) {
                        auto const pixel = image[i_ * imageWidth + j_];
                        rs[neighbours] = pixel.r;
                        gs[neighbours] = pixel.g;
                        bs[neighbours] = pixel.b;
                        ++neighbours;
                    }
                }
            }
            auto const middle = neighbours / 2;
            std::nth_element(rs.begin(), rs.begin() + middle, rs.end());
            std::nth_element(gs.begin(), gs.begin() + middle, gs.end());
            std::nth_element(bs.begin(), bs.begin() + middle, bs.end());
            result[i * imageWidth + j] = {rs[middle], gs[middle], bs[middle]};
        }
    }
}
