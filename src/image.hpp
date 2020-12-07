#pragma once

#include "utility.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

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

    std::size_t imageHeight = image.size() / imageWidth;

    auto const getPixel = [&image, imageHeight, imageWidth](std::ptrdiff_t i, std::ptrdiff_t j) -> std::optional<Pixel> {
        if (i >= 0 && j >= 0 && i < imageHeight && j < imageWidth) {
            return image[imageWidth * i + j];
        }
        else {
            return std::nullopt;
        }
    };

    std::array<std::uint8_t, square(2 * Radius + 1)> rs;
    std::array<std::uint8_t, square(2 * Radius + 1)> gs;
    std::array<std::uint8_t, square(2 * Radius + 1)> bs;
    for (std::ptrdiff_t i = 0; i < imageHeight; ++i) {
        for (std::ptrdiff_t j = 0; j < imageWidth; ++j) {
            unsigned neighbours = 0;
            for (std::ptrdiff_t di = -static_cast<std::ptrdiff_t>(Radius); di <= Radius; ++di) {
                for (std::ptrdiff_t dj = -static_cast<std::ptrdiff_t>(Radius); dj <= Radius; ++dj) {
                    auto const pixel = getPixel(i + di, j + dj);
                    if (pixel) {
                        rs[neighbours] = pixel->r;
                        gs[neighbours] = pixel->g;
                        bs[neighbours] = pixel->b;
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
