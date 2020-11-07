#pragma once

#include "span.hpp"

#include <cstdint>

#include <glm/vec3.hpp>


struct Pixel {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};


void linearTo8BitSRGB(Span<glm::vec3 const> linear, Span<Pixel> srgb);
