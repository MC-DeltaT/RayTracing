#pragma once

#include "bsp.hpp"
#include "material.hpp"
#include "mesh.hpp"
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

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>


struct RayTraceData {
    BSPTree const& bspTree;
    Span<glm::vec3 const> vertexNormals;                    // Vertex normals for instantiated meshes.
    Span<IndexRange const> vertexRanges;                    // Maps from model index to range of vertices.
    Span<MeshTri const> tris;                               // Tris for base meshes (not instantiated meshes).
    PermutedSpan<IndexRange const> triRanges;               // Maps from model index to range of tris.
    PermutedSpan<PreprocessedMaterial const> materials;     // Maps from model index to preprocessed mesh material.
};


struct RenderData {
    unsigned imageWidth;
    unsigned imageHeight;
    glm::vec3 cameraPosition;
    glm::mat3 pixelToRayTransform;
    RayTraceData rayTraceData;
};


constexpr inline static unsigned PIXEL_SAMPLE_RATE = 2048;
constexpr inline static unsigned RAY_BOUNCE_LIMIT = 4;
constexpr inline static float RAY_INTERSECTION_T_MIN = 1e-3f;   // Intersections with line param < this are discarded.


inline glm::vec3 rayTrace(RayTraceData const& data, Line const& ray, FastRNG& randomEngine, unsigned bounce = 0) {
    if (bounce > RAY_BOUNCE_LIMIT) {
        return {0.0f, 0.0f, 0.0f};
    }

    auto const intersection = data.bspTree.lineTriNearestIntersection<SurfaceConsideration::FRONT_ONLY>(
        ray, RAY_INTERSECTION_T_MIN);
    if (!intersection) {
        return {0.0f, 0.0f, 0.0f};
    }

    auto const& vertexRange = data.vertexRanges[intersection->meshTriIndex.mesh];
    auto const vertexNormals = data.vertexNormals[vertexRange];
    auto const& triRange = data.triRanges[intersection->meshTriIndex.mesh];
    auto const& tri = data.tris[triRange][intersection->meshTriIndex.tri];
    auto const pointCoord2 = intersection->pointCoord2;
    auto const pointCoord3 = intersection->pointCoord3;
    auto const pointCoord1 = 1.0f - pointCoord2 - pointCoord3;
    auto const normal = vertexNormals[tri.i1] * pointCoord1 + vertexNormals[tri.i2] * pointCoord2
        + vertexNormals[tri.i3] * pointCoord3;
    auto const& material = data.materials[intersection->meshTriIndex.mesh];
    auto const point = ray(intersection->t);
    auto const outgoing = -ray.direction;

    // Lighting model based on this paper:
    // B. Walter, S. R. Marschner, H. Li, and K. E. Torrance, "Microfacet models for refraction through rough surfaces", 2007.

    assert(isUnitVector(normal));
    assert(isUnitVector(outgoing));
    auto const nDotO = std::max(glm::dot(normal, outgoing), 0.0f);

    // GGX microfacet distribution function.
    auto const ndf = [&material](float nDotH) {
        if (nDotH <= 0.0f) {
            return 0.0f;
        }
        else {
            auto const nDotHSq = square(nDotH);
            return material.ndfAlphaSq /
                (glm::pi<float>() * square(nDotHSq) * square(material.ndfAlphaSq + (1.0f - nDotHSq) / nDotHSq));
        }
    };

    // GGX geometry function + Smith's method.
    auto const geometry = [&material](float nDotI, float nDotO, float hDotI, float hDotO) {
        auto const partial = [&material](float nDotR, float hDotR) {
            if ((nDotR > 0.0f && hDotR > 0.0f) || (nDotR < 0.0f && hDotR < 0.0f)) {
                auto const nDotRSq = square(nDotR);
                return 2.0f / (1.0f + std::sqrt(1.0f + material.geometryAlphaSq * (1.0f - nDotRSq) / nDotRSq));
            }
            else {
                return 0.0f;
            }
        };

        return partial(nDotI, hDotI) * partial(nDotO, hDotO);
    };

    // Fresnel-Schlick equation.
    auto const fresnel = [&material](float hDotO) {
        return material.f0 + material.oneMinusF0 * iPow(1.0f - hDotO, 5);
    };

    // Take more samples as ray bounces further to try to represent surfaces uniformly even after several bounces
    // (the probability of a specific ray being selected decreases as more bounces occur).
    auto const samples = bounce + 1;

    auto const [perpendicular1, perpendicular2] = orthonormalBasis(normal);

    glm::vec3 reflected{0.0f, 0.0f, 0.0f};
    for (unsigned i = 0; i < samples; ++i) {
        // Sample incident rays according to GGX distribution.
        auto const thetaParam = randomEngine.unitFloat();
        auto const cosThetaSq = 1.0f / (1.0f + material.ndfAlphaSq * thetaParam / (1.0f - thetaParam));
        auto const cosTheta = std::sqrt(cosThetaSq);
        auto const sinTheta = std::sqrt(1.0f - cosThetaSq);
        auto const phi = randomEngine.angle();
        auto const sinPhi = std::sin(phi);
        auto const cosPhi = std::cos(phi);
        
        auto const halfway = cosTheta * normal + sinTheta * (cosPhi * perpendicular1 + sinPhi * perpendicular2);

        auto hDotO = glm::dot(halfway, outgoing);
        auto const incident = 2.0f * hDotO * halfway - outgoing;
        assert(isUnitVector(incident));
        auto const nDotI = glm::dot(normal, incident);

        if (nDotI > 0.0f) {
            auto const incidentLight = rayTrace(data, {point, incident}, randomEngine, bounce + 1);

            // Cook-Torrance BRDF.
            assert(hDotO > 0.0f);
            auto const& nDotH = cosTheta;
            auto const& hDotI = hDotO;
            auto const specularD = ndf(nDotH);
            auto const specularG = geometry(nDotI, nDotO, hDotI, hDotO);
            auto const specularF = fresnel(hDotO);
            // ray probability = specularD * nDotH / (4 * hDotO)
            auto const specular = specularG * specularF * hDotO / (nDotO * nDotH);
            auto const diffuse = 4.0f * (1.0f - specularF) * material.adjustedColour * nDotI * hDotO / (specularD * nDotH);
            auto const weight = diffuse + specular;
            
            reflected += incidentLight * weight;
        }
    }
    reflected /= samples;

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
            Line const ray{data.cameraPosition, rayDirection};
            colour += rayTrace(data.rayTraceData, ray, randomEngine);
        }
        colour /= PIXEL_SAMPLE_RATE;
        return colour;
    });
}
