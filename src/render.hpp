#pragma once

#include "basic_types.hpp"
#include "bsp.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "utility/index_iterator.hpp"
#include "utility/math.hpp"
#include "utility/numeric.hpp"
#include "utility/permuted_span.hpp"
#include "utility/random.hpp"
#include "utility/span.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <execution>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/mat3x3.hpp>


struct RayTraceData {
    BSPTree const& bspTree;
    Span<vec3 const> vertexNormals;                     // Vertex normals for instantiated meshes.
    Span<VertexRange const> vertexRanges;               // Maps from model index to range of vertices.
    Span<MeshTri const> tris;                           // Tris for base meshes (not instantiated meshes).
    PermutedSpan<TriRange const, MeshIndex> triRanges;  // Maps from model index to range of tris.
    PermutedSpan<PreprocessedMaterial const, MeshIndex> materials;  // Maps from model index to preprocessed mesh material.
};


struct RenderData {
    unsigned imageWidth;
    unsigned imageHeight;
    vec3 cameraPosition;
    glm::mat3 pixelToRayTransform;
    RayTraceData rayTraceData;
};


constexpr inline static unsigned PIXEL_SAMPLE_RATE = 2048;
constexpr inline static unsigned RAY_BOUNCE_LIMIT = 4;
constexpr inline static float RAY_INTERSECTION_T_MIN = 1e-3f;   // Intersections with line param < this are discarded.


inline vec3 rayTrace(RayTraceData const& data, Line ray, FastRNG& randomEngine) {
    // Lighting model based on this paper:
    // B. Walter, S. R. Marschner, H. Li, and K. E. Torrance, "Microfacet models for refraction through rough surfaces", 2007.

    // GGX microfacet distribution function.
    auto const ndf = [](float alphaSq, float nDotH) {
        assert(nDotH > 0.0f);
        auto const nDotHSq = square(nDotH);
        auto const tanThetaSq = (1.0f - nDotHSq) / nDotHSq;
        return alphaSq / (glm::pi<float>() * square(nDotHSq) * square(alphaSq + tanThetaSq));
    };

    // GGX geometry function + Smith's method.
    auto const geometry = [](float alphaSq, float nDotI, float nDotO, float hDotI, float hDotO) {
        auto const partial = [alphaSq](float nDotR) {
            auto const nDotRSq = square(nDotR);
            return 2.0f / (1.0f + std::sqrt(1.0f + alphaSq * (1.0f - nDotRSq) / nDotRSq));
        };

        assert(nDotI > 0.0f && nDotO > 0.0f && hDotI > 0.0f && hDotO == hDotI);
        return partial(nDotI) * partial(nDotO);
    };

    // Fresnel-Schlick equation.
    auto const fresnel = [](vec3 f0, vec3 oneMinusF0, float hDotO) {
        assert(hDotO >= 0.0f);
        // Sometimes hDotO is very slightly > 1 due to FP error, which technically invalidates this formula.
        // But this error becomes extremely small when raised to the 5th power, so it doesn't have much effect.
        return f0 + oneMinusF0 * iPow(1.0f - hDotO, 5);
    };

    vec3 outgoingLight{0.0f, 0.0f, 0.0f};
    vec3 lightWeight{1.0f, 1.0f, 1.0f};
    for (unsigned bounce = 0; ; ++bounce) {
        auto const intersection = data.bspTree.lineTriNearestIntersection<SurfaceConsideration::FRONT_ONLY>(
            ray, RAY_INTERSECTION_T_MIN);
        if (!intersection) {
            break;
        }

        auto const& material = data.materials[intersection->meshTriIndex.mesh];

        outgoingLight += lightWeight * material.emission;

        if (bounce == RAY_BOUNCE_LIMIT) {
            break;
        }

        auto const& vertexRange = data.vertexRanges[intersection->meshTriIndex.mesh];
        auto const vertexNormals = data.vertexNormals[vertexRange];
        auto const& triRange = data.triRanges[intersection->meshTriIndex.mesh];
        auto const& tri = data.tris[triRange][intersection->meshTriIndex.tri];
        auto const& pointCoord2 = intersection->pointCoord2;
        auto const& pointCoord3 = intersection->pointCoord3;
        auto const pointCoord1 = 1.0f - pointCoord2 - pointCoord3;
        auto normal = vertexNormals[tri.v1] * pointCoord1 + vertexNormals[tri.v2] * pointCoord2
            + vertexNormals[tri.v3] * pointCoord3;
        auto const& point = intersection->point;
        auto const outgoing = -ray.direction;

        assert(isUnitVector(normal));
        assert(isUnitVector(outgoing));
        auto nDotO = glm::dot(normal, outgoing);
        // Flip normal direction if ray strikes back of surface.
        if (nDotO < 0.0f) {
            nDotO = -nDotO;
            normal = -normal;
        }

        auto const [perpendicular1, perpendicular2] = orthonormalBasis(normal);

        // Sample incident rays according to GGX distribution.
        auto const thetaParam = randomEngine.unitFloatOpen();
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
            // Cook-Torrance BRDF.
            assert(hDotO > 0.0f);
            auto const& nDotH = cosTheta;
            auto const& hDotI = hDotO;
            auto const specularD = ndf(material.ndfAlphaSq, nDotH);
            auto const specularF = fresnel(material.f0, material.oneMinusF0, hDotO);
            // ray probability = specularD * nDotH / (4 * hDotO)
            auto const diffuse = 4.0f * (1.0f - specularF) * material.adjustedColour * nDotI * hDotO / (specularD * nDotH);
            auto localWeight = diffuse;
            if (nDotO > 0.0f) {
                auto const specularG = geometry(material.geometryAlphaSq, nDotI, nDotO, hDotI, hDotO);
                auto const specular = specularG * specularF * hDotO / (nDotO * nDotH);
                localWeight += specular;
            }
            lightWeight *= localWeight;

            ray = {point, incident};
        }
        else {
            // lightWeight becomes 0.
            break;
        }
    }

    // TODO? light transmission

    return outgoingLight;
}


inline void render(RenderData const& data, Span<vec3> image) {
    assert(image.size() == data.imageWidth * data.imageHeight);
    std::transform(std::execution::par, IndexIterator<>{0}, IndexIterator<>{image.size()}, image.begin(),
            [&data](std::size_t index) {
        auto const pixelX = index % data.imageWidth;
        auto const pixelY = index / data.imageWidth;
        auto& randomEngine = ::randomEngine;    // Access random engine here to force static initialisation.
        vec3 colour{0.0f, 0.0f, 0.0f};
        for (unsigned i = 0; i < PIXEL_SAMPLE_RATE; ++i) {
            auto const sampleX = pixelX + randomEngine.unitFloatOpen();
            auto const sampleY = pixelY + randomEngine.unitFloatOpen();
            auto const rayDirection = glm::normalize(data.pixelToRayTransform * vec3{sampleX, sampleY, 1.0f});
            Line const ray{data.cameraPosition, rayDirection};
            colour += rayTrace(data.rayTraceData, ray, randomEngine);
        }
        colour /= PIXEL_SAMPLE_RATE;
        return colour;
    });
}
