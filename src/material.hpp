#pragma once

#include "basic_types.hpp"
#include "utility/math.hpp"

#include <cassert>

#include <glm/gtc/constants.hpp>



struct Material {
    PackedFVec3 colour;
    float roughness;        // In range (0, 1].
    float metalness;        // In range [0, 1].
    PackedFVec3 emission;   // Colour that is inherently emitted.
};


struct PreprocessedMaterial {
    float ndfAlphaSq;
    float geometryAlphaSq;
    FastFVec3 f0;
    FastFVec3 oneMinusF0;
    FastFVec3 adjustedColour;
    FastFVec3 emission;
};


inline PreprocessedMaterial preprocessMaterial(Material const& material) {
    assert(material.roughness > 0.0f);
    auto const roughness4 = iPow(material.roughness, 4);
    auto const ndfAlphaSq = roughness4;
    auto const geometryAlphaSq = roughness4 / 4.0f;

    assert(isNormalised(material.metalness));
    assert(isNormalised(material.colour.r) && isNormalised(material.colour.g) && isNormalised(material.colour.b));
    auto const oneMinusMetalness = 1.0f - FastFVec3{material.metalness};
    auto const f0 = oneMinusMetalness * 0.04f + FastFVec3{material.metalness} * FastFVec3{material.colour};
    auto const oneMinusF0 = 1.0f - f0;
    auto const adjustedColour = oneMinusMetalness * FastFVec3{material.colour} / glm::pi<float>();

    return {ndfAlphaSq, geometryAlphaSq, f0, oneMinusF0, adjustedColour, FastFVec3{material.emission}};
}
