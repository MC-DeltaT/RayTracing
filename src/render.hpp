#pragma once

#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "ray.hpp"
#include "utility/index_iterator.hpp"
#include "utility/math.hpp"
#include "utility/misc.hpp"
#include "utility/permuted_span.hpp"
#include "utility/random.hpp"
#include "utility/span.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <execution>
#include <random>
#include <utility>
#include <vector>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>


struct RayTraceData {
    Span<glm::vec3 const> vertexNormals;            // Vertex normals for instantiated meshes.
    Span<MeshTri const> tris;                       // Tris for base meshes (not instantiated meshes).
    Span<PreprocessedTri const> preprocessedTris;   // Preprocessed tris for instantiated meshes.
    Span<IndexRange const> vertexRanges;            // Maps from model index to range of vertices.
    PermutedSpan<IndexRange const> triRanges;       // Maps from model index to range of tris.
    Span<IndexRange const> preprocessedTriRanges;   // Maps from model index to range of preprocessed tris.
    PermutedSpan<Material const> materials;         // Maps from model index to mesh material.
    Span<PointLight const> pointLights;
    Span<DirectionalLight const> directionalLights;
};


struct RenderData {
    unsigned imageWidth;
    unsigned imageHeight;
    glm::vec3 cameraPosition;
    glm::mat3 pixelToRayTransform;
    RayTraceData rayTraceData;
};


constexpr inline static unsigned PIXEL_SAMPLE_RATE = 128;
constexpr inline static unsigned RAY_BOUNCE_LIMIT = 3;
constexpr inline static float RAY_INTERSECTION_MIN_PARAM = 1e-3f;   // Intersections with line param < this are discarded.
constexpr inline static unsigned LIGHT_RECEIVE_SAMPLES = 4;
constexpr inline static std::size_t SIN_COS_LUT_SAMPLES = 1ull << 20;


inline static std::vector<std::pair<float, float>> const SIN_COS_LUT = []() {
    std::vector<std::pair<float, float>> result(SIN_COS_LUT_SAMPLES);
    for (std::size_t i = 0; i < SIN_COS_LUT_SAMPLES; ++i) {
        auto const x = i / glm::two_pi<double>();
        result[i] = {static_cast<float>(std::sin(x)), static_cast<float>(std::cos(x))};
    }
    return result;
}();


inline glm::vec3 rayTrace(RayTraceData const& data, Ray const& ray, unsigned bounce = 0) {
    if (bounce > RAY_BOUNCE_LIMIT) {
        return {0.0f, 0.0f, 0.0f};
    }

    auto const intersection = rayNearestIntersection<false>(data.preprocessedTris, data.preprocessedTriRanges, ray,
        {RAY_INTERSECTION_MIN_PARAM, INFINITY});
    if (!intersection) {
        return {0.0f, 0.0f, 0.0f};
    }

    auto const& vertexRange = data.vertexRanges[intersection->mesh];
    auto const vertexNormals = data.vertexNormals[vertexRange];
    auto const& triRange = data.triRanges[intersection->mesh];
    auto const& tri = data.tris[triRange][intersection->tri];
    auto const pointCoord2 = intersection->pointCoord2;
    auto const pointCoord3 = intersection->pointCoord3;
    auto const pointCoord1 = 1.0f - pointCoord2 - pointCoord3;
    auto const normal = vertexNormals[tri.i1] * pointCoord1 + vertexNormals[tri.i2] * pointCoord2
        + vertexNormals[tri.i3] * pointCoord3;
    auto const& material = data.materials[intersection->mesh];
    auto const point = ray.origin + intersection->rayParam * ray.direction;
    auto const outgoing = -ray.direction;

    assert(isUnitVector(normal));
    assert(isUnitVector(outgoing));
    auto const nDotO = std::max(glm::dot(normal, outgoing), 0.0f);
    auto const specularNormFactor = 1.0f / (4.0f * nDotO);

    assert(material.roughness > 0.0f);
    auto const roughnessSq = square(material.roughness);
    auto const ndfAlphaSq = roughnessSq;
    auto const geometryAlphaSq = roughnessSq;

    assert(isNormalised(material.metalness));
    assert(isNormalised(material.colour.r) && isNormalised(material.colour.g) && isNormalised(material.colour.b));
    auto const oneMinusMetalness = 1.0f - material.metalness;
    auto const f0 = oneMinusMetalness * glm::vec3{0.04f} + material.metalness * material.colour;
    auto const oneMinusF0 = 1.0f - f0;

    auto const adjustedColour = oneMinusMetalness / glm::pi<float>() * material.colour;

    auto const ndf = [ndfAlphaSq](float nDotH) {
        // GGX microfacet distribution function.
        if (nDotH <= 0.0f) {
            return 0.0f;
        }
        else {
            auto const nDotHSq = square(nDotH);
            return ndfAlphaSq / (glm::pi<float>() * square(nDotHSq) * square(ndfAlphaSq + (1.0f - nDotHSq) / nDotHSq));
        }
    };

    auto const geometry = [geometryAlphaSq](float nDotI, float nDotO, float hDotI, float hDotO) {
        // GGX geometry function + Smith's method.
        auto const partial = [geometryAlphaSq](float nDotR, float hDotR) {
            if (hDotR < 0.0f != nDotR < 0.0f) {
                return 0.0f;
            }
            else {
                auto const nDotRSq = square(nDotR);
                return 2.0f / (1.0f + std::sqrt(1.0f + geometryAlphaSq * (1.0f - nDotRSq) / nDotRSq));
            }
        };

        return partial(nDotI, hDotI) * partial(nDotO, hDotO);
    };

    auto const fresnel = [&f0, &oneMinusF0](float hDotO) {
        // Fresnel-Schlick equation.
        return f0 + oneMinusF0 * iPow<5>(1.0f - hDotO);
    };

    auto const brdf = [&ndf, &geometry, &fresnel, &normal, nDotO, specularNormFactor, &adjustedColour]
            (float nDotI, glm::vec3 const& incident, glm::vec3 const& outgoing) {
        // Cook-Torrance microfacet reflection model.
        auto const halfway = glm::normalize(incident + outgoing);
        auto const nDotH = std::max(glm::dot(normal, halfway), 0.0f);
        // Angle between H and I or O must be < 90 degrees, so no need to clamp dot product.
        auto const hDotI = glm::dot(halfway, incident);
        auto const hDotO = glm::dot(halfway, outgoing);

        auto const specularD = ndf(nDotH);
        auto const specularG = geometry(nDotI, nDotO, hDotI, hDotO);
        auto const specularF = fresnel(hDotO);
        auto const specular = specularD * specularG * specularF * specularNormFactor;
        auto const diffuse = (1.0f - specularF) * nDotI * adjustedColour;
        return diffuse + specular;
    };

    glm::vec3 reflected{0.0f, 0.0f, 0.0f};

    for (auto const& light : data.pointLights) {
        auto incident = light.position - point;
        auto nDotI = glm::dot(incident, normal);
        if (nDotI > 0.0f) {
            auto const distanceToLight = glm::length(incident);
            incident /= distanceToLight;
            nDotI /= distanceToLight;
            Ray const shadowRay{point, incident};
            auto const isBlocked = rayIntersectsAny<true>(data.preprocessedTris, data.preprocessedTriRanges, shadowRay, 
                {RAY_INTERSECTION_MIN_PARAM, distanceToLight});
            if (!isBlocked) {
                auto const weight = brdf(nDotI, incident, outgoing);
                reflected += weight * light.colour;
            }
        }
    }
    
    for (auto const& light : data.directionalLights) {
        assert(isUnitVector(light.direction));
        auto nDotI = glm::dot(light.direction, normal);
        if (nDotI < 0.0f) {
            auto const incident = -light.direction;
            Ray const shadowRay{point, incident};
            auto const isBlocked = rayIntersectsAny<true>(data.preprocessedTris, data.preprocessedTriRanges, shadowRay, 
                {RAY_INTERSECTION_MIN_PARAM, INFINITY});
            if (!isBlocked) {
                nDotI = -nDotI;
                auto const weight = brdf(nDotI, incident, outgoing);
                reflected += weight * light.colour;
            }
        }
    }

    unsigned const bounceSamples = std::max<unsigned>(LIGHT_RECEIVE_SAMPLES >> bounce, 1);
    {
        auto const [perpendicular1, perpendicular2] = orthonormalBasis(normal);

        // TODO: importance sampling

        for (unsigned i = 0; i < bounceSamples; ++i) {
            auto const cosTheta = randomEngine.unitFloat();
            auto const sinTheta = std::sqrt(1.0f - square(cosTheta));
            auto const [sinPhi, cosPhi] = SIN_COS_LUT[randomEngine.value() % SIN_COS_LUT_SAMPLES];
            
            auto const incident = cosTheta * normal + sinTheta * (cosPhi * perpendicular1 + sinPhi * perpendicular2);

            auto const weight = brdf(cosTheta, incident, outgoing);
            auto const incidentLight = rayTrace(data, {point, incident}, bounce + 1);
            reflected += weight * incidentLight;
        }
    }

    // TODO: light transmission

    auto const receivedRays = bounceSamples + data.pointLights.size() + data.directionalLights.size();
    reflected *= glm::two_pi<float>() / receivedRays;

    auto const colour = material.emission + reflected;
    return colour;
}


inline void render(RenderData const& data, Span<glm::vec3> image) {
    assert(image.size() == data.imageWidth * data.imageHeight);
    std::transform(std::execution::par, IndexIterator{0}, IndexIterator{image.size()}, image.begin(), [&data](auto index) {
        auto const pixelX = index % data.imageWidth;
        auto const pixelY = index / data.imageWidth;
        glm::vec3 colour{0.0f, 0.0f, 0.0f};
        for (unsigned i = 0; i < PIXEL_SAMPLE_RATE; ++i) {
            auto const sampleX = pixelX + randomEngine.unitFloat();
            auto const sampleY = pixelY + randomEngine.unitFloat();
            auto const rayDirection = glm::normalize(data.pixelToRayTransform * glm::vec3{sampleX, sampleY, 1.0f});
            Ray const ray{data.cameraPosition, rayDirection};
            colour += rayTrace(data.rayTraceData, ray);
        }
        colour /= PIXEL_SAMPLE_RATE;
        return colour;
    });
}
