#pragma once

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
#include <execution>

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
};


struct RenderData {
    unsigned imageWidth;
    unsigned imageHeight;
    glm::vec3 cameraPosition;
    glm::mat3 pixelToRayTransform;
    RayTraceData rayTraceData;
};


constexpr inline static unsigned PIXEL_SAMPLE_RATE = 8;
constexpr inline static unsigned RAY_BOUNCE_LIMIT = 3;
constexpr inline static float RAY_INTERSECTION_MIN_PARAM = 1e-3f;   // Intersections with line param < this are discarded.
constexpr inline static unsigned RAY_BOUNCE_SAMPLES = 16;


inline glm::vec3 rayTrace(RayTraceData const& data, Ray const& ray, FastRNG& randomEngine, unsigned bounce = 0) {
    if (bounce > RAY_BOUNCE_LIMIT) {
        return {0.0f, 0.0f, 0.0f};
    }

    auto const intersection = rayNearestIntersection<SurfaceConsideration::FRONT_ONLY>(
        data.preprocessedTris, data.preprocessedTriRanges, ray, {RAY_INTERSECTION_MIN_PARAM, INFINITY});
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

    assert(material.roughness > 0.0f);
    auto const roughness4 = iPow<4>(material.roughness);
    auto const ndfAlphaSq = roughness4;
    auto const geometryAlphaSq = roughness4 / 4.0f;

    assert(isNormalised(material.metalness));
    assert(isNormalised(material.colour.r) && isNormalised(material.colour.g) && isNormalised(material.colour.b));
    auto const oneMinusMetalness = 1.0f - material.metalness;
    auto const f0 = oneMinusMetalness * glm::vec3{0.04f} + material.metalness * material.colour;
    auto const oneMinusF0 = 1.0f - f0;

    auto const adjustedColour = oneMinusMetalness * material.colour;

    // GGX microfacet distribution function.
    auto const ndf = [ndfAlphaSq](float nDotH) {
        if (nDotH <= 0.0f) {
            return 0.0f;
        }
        else {
            auto const nDotHSq = square(nDotH);
            return ndfAlphaSq / (glm::pi<float>() * square(nDotHSq) * square(ndfAlphaSq + (1.0f - nDotHSq) / nDotHSq));
        }
    };

    // GGX geometry function + Smith's method.
    auto const geometry = [geometryAlphaSq](float nDotI, float nDotO, float hDotI, float hDotO) {
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

    // Fresnel-Schlick equation.
    auto const fresnel = [&f0, &oneMinusF0](float hDotO) {
        return f0 + oneMinusF0 * iPow<5>(1.0f - hDotO);
    };

    // Cook-Torrance BRDF for importance-sampled ray bounce.
    auto const brdf = [&geometry, &fresnel, &outgoing, &normal, nDotO, &adjustedColour]
            (float nDotI, float nDotH, float hDotI, float hDotO) {
        auto const specularG = geometry(nDotI, nDotO, hDotI, hDotO);
        auto const specularF = fresnel(hDotO);
        auto const specular = specularG * specularF * hDotI / nDotH;
        auto const diffuse = (1.0f - specularF) * nDotI * adjustedColour;
        return diffuse + specular;
    };

    glm::vec3 reflected{0.0f, 0.0f, 0.0f};
    {
        auto const [perpendicular1, perpendicular2] = orthonormalBasis(normal);

        for (unsigned i = 0; i < RAY_BOUNCE_SAMPLES; ++i) {
            // Sample incident rays according to GGX distribution.
            auto const thetaParam = randomEngine.unitFloat();
            auto const cosThetaSq = 1.0f / (1.0f + ndfAlphaSq * thetaParam / (1.0f - thetaParam));
            auto const cosTheta = std::sqrt(cosThetaSq);
            auto const sinTheta = std::sqrt(1.0f - cosThetaSq);
            auto const phi = randomEngine.angle();
            auto const sinPhi = std::sin(phi);
            auto const cosPhi = std::cos(phi);
            
            auto const halfway = cosTheta * normal + sinTheta * (cosPhi * perpendicular1 + sinPhi * perpendicular2);

            auto hDotO = glm::dot(halfway, outgoing);
            auto const incident = 2.0f * hDotO * halfway - outgoing;
            auto const nDotI = glm::dot(normal, incident);

            if (nDotI > 0.0f) {
                hDotO = std::max(hDotO, 0.0f);
                auto const& hDotI = hDotO;
                auto const weight = brdf(nDotI, cosTheta, hDotI, hDotO);
                auto const incidentLight = rayTrace(data, {point, incident}, randomEngine, bounce + 1);
                reflected += weight * incidentLight;
            }
        }
    }
    reflected /= RAY_BOUNCE_SAMPLES;

    // TODO? light transmission

    auto const colour = material.emission + reflected;
    return colour;
}


inline void render(RenderData const& data, Span<glm::vec3> image) {
    assert(image.size() == data.imageWidth * data.imageHeight);
    std::transform(std::execution::par, IndexIterator{0}, IndexIterator{image.size()}, image.begin(), [&data](auto index) {
        auto const pixelX = index % data.imageWidth;
        auto const pixelY = index / data.imageWidth;
        auto& randomEngine = ::randomEngine;    // Access random engine here to force static initialisation.
        glm::vec3 colour{0.0f, 0.0f, 0.0f};
        for (unsigned i = 0; i < PIXEL_SAMPLE_RATE; ++i) {
            auto const sampleX = pixelX + randomEngine.unitFloat();
            auto const sampleY = pixelY + randomEngine.unitFloat();
            auto const rayDirection = glm::normalize(data.pixelToRayTransform * glm::vec3{sampleX, sampleY, 1.0f});
            Ray const ray{data.cameraPosition, rayDirection};
            colour += rayTrace(data.rayTraceData, ray, randomEngine);
        }
        colour /= PIXEL_SAMPLE_RATE;
        return colour;
    });
}
