#pragma once

#include "utility/math.hpp"

#include <cassert>

#include <glm/vec3.hpp>


struct Material {
    glm::vec3 colour;
    float roughness;        // In range (0, 1].
    float metalness;        // In range [0, 1].
    glm::vec3 emission;     // Colour that is inherently emitted.
};


struct PreprocessedMaterial {
    float ndfAlphaSq;
    float geometryAlphaSq;
    glm::vec3 f0;
    glm::vec3 oneMinusF0;
    glm::vec3 adjustedColour;
    glm::vec3 emission;
};


inline PreprocessedMaterial preprocessMaterial(Material const& material) {
    assert(material.roughness > 0.0f);
    auto const roughness4 = iPow(material.roughness, 4);
    auto const ndfAlphaSq = roughness4;
    auto const geometryAlphaSq = roughness4 / 4.0f;

    assert(isNormalised(material.metalness));
    assert(isNormalised(material.colour.r) && isNormalised(material.colour.g) && isNormalised(material.colour.b));
    auto const oneMinusMetalness = 1.0f - material.metalness;
    auto const f0 = oneMinusMetalness * glm::vec3{0.04f} + material.metalness * material.colour;
    auto const oneMinusF0 = 1.0f - f0;
    auto const adjustedColour = oneMinusMetalness * material.colour;

    return {ndfAlphaSq, geometryAlphaSq, f0, oneMinusF0, adjustedColour, material.emission};
}
