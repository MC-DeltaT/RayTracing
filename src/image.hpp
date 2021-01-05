#pragma once

#include "basic_types.hpp"
#include "utility/math.hpp"
#include "utility/span.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include <glm/common.hpp>
#include <glm/vec3.hpp>


inline vec3 reinhardToneMap(vec3 const& hdr) {
    return hdr / (1.0f + hdr);
}


inline float linearToSRGB(float linear) {
    if (linear <= 0.0031308f) {
        return linear * 12.92f;
    }
    else {
        return 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
    }
}

inline vec3 linearToSRGB(vec3 const& linear) {
    return {linearToSRGB(linear.r), linearToSRGB(linear.g), linearToSRGB(linear.b)};
}


inline float srgbToLinear(float srgb) {
    if (srgb <= 0.04045f) {
        return srgb / 12.92f;
    }
    else {
        return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
    }
}

inline vec3 srgbToLinear(vec3 const& srgb) {
    return {srgbToLinear(srgb.r), srgbToLinear(srgb.g), srgbToLinear(srgb.b)};
}


inline glm::u8vec3 floatTo8BitUInt(vec3 const& pixel) {
    return glm::clamp(255.0f * pixel, 0.0f, 255.0f);
}


inline vec3 nanToRed(vec3 const& pixel) {
    if (std::isnan(pixel.r) || std::isnan(pixel.g) || std::isnan(pixel.b)) {
        return {1.0f, 0.0f, 0.0f};
    }
    else {
        return pixel;
    }
}


inline vec3 infToGreen(vec3 const& pixel) {
    if (std::isinf(pixel.r) || std::isinf(pixel.g) || std::isinf(pixel.b)) {
        return {0.0f, 1.0f, 0.0f};
    }
    else {
        return pixel;
    }
}


template<unsigned Radius>
void medianFilter(Span<vec3 const> image, std::size_t imageWidth, Span<vec3> result) {
    assert(image.size() % imageWidth == 0);
    assert(image.data() != result.data());
    assert(image.size() == result.size());

    std::ptrdiff_t const imageHeightS = image.size() / imageWidth;
    std::ptrdiff_t const imageWidthS = imageWidth;
    constexpr std::ptrdiff_t radiusS = Radius;

    std::array<float, square(2 * Radius + 1)> rs;
    std::array<float, square(2 * Radius + 1)> gs;
    std::array<float, square(2 * Radius + 1)> bs;
    for (std::ptrdiff_t i = 0; i < imageHeightS; ++i) {
        for (std::ptrdiff_t j = 0; j < imageWidthS; ++j) {
            unsigned neighbours = 0;
            for (std::ptrdiff_t di = -radiusS; di <= radiusS; ++di) {
                for (std::ptrdiff_t dj = -radiusS; dj <= radiusS; ++dj) {
                    auto const i_ = i + di;
                    auto const j_ = j + dj;
                    if (i_ >= 0 && j_ >= 0 && i_ < imageHeightS && j_ < imageWidthS) {
                        auto const& pixel = image[i_ * imageWidth + j_];
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
            result[i * imageWidth + j] = {rs.at(middle), gs.at(middle), bs.at(middle)};
        }
    }
}
