#pragma once

#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "ray.hpp"
#include "utility.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <execution>
#include <random>

#include <boost/range/counting_range.hpp>
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


constexpr inline static unsigned PIXEL_SAMPLE_RATE = 512;
constexpr inline static unsigned RAY_BOUNCE_LIMIT = 3;
constexpr inline static float RAY_INTERSECTION_MIN_PARAM = 1e-3f;   // Intersections with line param < this are discarded.
constexpr inline static unsigned LIGHT_RECEIVE_SAMPLES = 1;


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
    auto const tris = data.tris[triRange];
    auto const& tri = tris[intersection->tri];
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
    auto const nDotO = std::clamp(glm::dot(normal, outgoing), 0.0f, 1.0f);
    auto const specularNormFactor = 1.0f / (4.0f * nDotO);

    assert(material.roughness > 0.0f);
    auto const roughnessSq = square(material.roughness);
    auto const geometryAlphaSq = roughnessSq;
    auto const ndfAlphaSq = roughnessSq;

    assert(isNormalised(material.metalness));
    assert(isNormalised(material.colour.r) && isNormalised(material.colour.g) && isNormalised(material.colour.b));
    auto const f0 = glm::mix(glm::vec3{0.04f}, material.colour, material.metalness);

    auto const oneMinusMetalness = 1.0f - material.metalness;
    auto const colourOverPi = material.colour / glm::pi<float>();

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

    auto const fresnel = [&f0](float hDotO) {
        // Fresnel-Schlick equation.
        return f0 + (1.0f - f0) * iPow<5>(1.0f - hDotO);
    };

    auto const brdf = [&ndf, &geometry, &fresnel, nDotO, specularNormFactor, oneMinusMetalness, &colourOverPi]
            (float nDotI, float nDotH, float hDotI, float hDotO) {
        // Cook-Torrance microfacet reflection model.
        auto const specularD = ndf(nDotH);
        auto const specularG = geometry(nDotI, nDotO, hDotI, hDotO);
        auto const specularF = fresnel(hDotO);
        auto const specular = specularD * specularG * specularF * specularNormFactor;
        auto const diffuseCoeff = (1.0f - specularF) * oneMinusMetalness;
        auto const diffuse = diffuseCoeff * nDotI * colourOverPi;
        return diffuse + specular;
    };

    glm::vec3 reflected{0.0f, 0.0f, 0.0f};

    {
        auto const receiveLight = [&data, &bounce, &point](glm::vec3 const& direction) {
            return rayTrace(data, {point, direction}, bounce + 1);
        };

        auto const [perpendicular1, perpendicular2] = orthonormalBasis(normal);

        // TODO: importance sampling
        std::uniform_real_distribution<float> cosThetaDist{0.0f, 1.0f};
        std::uniform_real_distribution<float> phiDist{0.0f, glm::two_pi<float>()};

        for (unsigned i = 0; i < LIGHT_RECEIVE_SAMPLES; ++i) {
            auto const cosTheta = cosThetaDist(randomEngine);
            auto const sinTheta = std::sqrt(1.0f - square(cosTheta));
            auto const phi = phiDist(randomEngine);
            auto const cosPhi = std::cos(phi);
            auto const sinPhi = std::sin(phi);
            
            auto const incident = cosTheta * normal + sinTheta * (cosPhi * perpendicular1 + sinPhi * perpendicular2);

            auto const halfway = glm::normalize(incident + outgoing);
            auto const nDotH = std::clamp(glm::dot(normal, halfway), 0.0f, 1.0f);
            auto const hDotO = std::clamp(glm::dot(halfway, outgoing), 0.0f, 1.0f);
            auto const hDotI = std::clamp(glm::dot(halfway, incident), 0.0f, 1.0f);

            auto const weight = brdf(cosTheta, nDotH, hDotI, hDotO);
            auto const incidentLight = receiveLight(incident);
            reflected += weight * incidentLight;
        }
    }

    // TODO: light transmission

    for (auto const& light : data.pointLights) {
        auto incident = light.position - point;
        auto const distanceToLight = glm::length(incident);
        incident /= distanceToLight;
        auto const nDotI = std::clamp(glm::dot(incident, normal), 0.0f, 1.0f);
        if (nDotI > 0.0f) {
            Ray const shadowRay{point, incident};
            auto const isBlocked = rayIntersectsAny<true>(data.preprocessedTris, data.preprocessedTriRanges, shadowRay, 
                {RAY_INTERSECTION_MIN_PARAM, distanceToLight});
            if (!isBlocked) {
                auto const halfway = glm::normalize(incident + outgoing);
                auto const nDotH = std::clamp(glm::dot(normal, halfway), 0.0f, 1.0f);
                auto const hDotO = std::clamp(glm::dot(halfway, outgoing), 0.0f, 1.0f);
                auto const hDotI = std::clamp(glm::dot(halfway, incident), 0.0f, 1.0f);
                auto const weight = brdf(nDotI, nDotH, hDotI, hDotO);
                reflected += weight * light.colour;
            }
        }
    }
    // TODO: other light types

    auto const receivedRays = LIGHT_RECEIVE_SAMPLES + data.pointLights.size();
    reflected *= glm::two_pi<float>() / receivedRays;

    auto const colour = material.emission + reflected;
    return colour;
}


inline void render(RenderData const& data, Span<glm::vec3> image) {
    assert(image.size() == data.imageWidth * data.imageHeight);
    auto const pixelRange = boost::counting_range(0ull, image.size());
    std::transform(std::execution::par, pixelRange.begin(), pixelRange.end(), image.begin(), [&data](auto index) {
        auto const pixelX = index % data.imageWidth;
        auto const pixelY = index / data.imageWidth;
        std::uniform_real_distribution<float> sampleDist{0.0f, 1.0f};
        glm::vec3 colour{0.0f, 0.0f, 0.0f};
        for (unsigned i = 0; i < PIXEL_SAMPLE_RATE; ++i) {
            auto const sampleX = pixelX + sampleDist(randomEngine);
            auto const sampleY = pixelY + sampleDist(randomEngine);
            auto const rayDirection = glm::normalize(data.pixelToRayTransform * glm::vec3{sampleX, sampleY, 1.0f});
            Ray const ray{data.cameraPosition, rayDirection};
            colour += rayTrace(data.rayTraceData, ray);
        }
        colour /= PIXEL_SAMPLE_RATE;
        return colour;
    });
}
