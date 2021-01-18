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
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <execution>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/mat3x3.hpp>


struct RayTraceData {
    BSPTree const& bspTree;
    Span<PackedFVec3 const> vertexNormals;              // Vertex normals for instantiated meshes.
    Span<VertexRange const> vertexRanges;               // Maps from model index to range of vertices.
    Span<MeshTri const> tris;                           // Tris for base meshes (not instantiated meshes).
    PermutedSpan<TriRange const, MeshIndex> triRanges;  // Maps from model index to range of tris.
    PermutedSpan<PreprocessedMaterial const, MeshIndex> materials;  // Maps from model index to preprocessed mesh material.
};


struct RenderData {
    unsigned imageWidth;
    unsigned imageHeight;
    PackedFVec3 cameraPosition;
    glm::mat3 pixelToRayTransform;
    RayTraceData rayTraceData;
};


constexpr inline static unsigned PIXEL_SAMPLE_RATE = 2048;
constexpr inline static float RAY_INTERSECTION_T_MIN = 1e-3f;   // Intersections with line param < this are discarded.


inline FastFVec3 rayTrace(RayTraceData const& data, Line ray, FastRNG& randomEngine) {
    // Lighting model based on this paper:
    // B. Walter, S. R. Marschner, H. Li, and K. E. Torrance, "Microfacet models for refraction through rough surfaces", 2007.

    // GGX microfacet distribution function.
    auto const ndf = [](FVec8 alphaSq, FVec8 nDotH) {
        // assert(nDotH > 0.0f);
        auto const nDotHSq = square(nDotH);
        auto const tanThetaSq = 1.0f / nDotHSq - 1.0f;
        return alphaSq / (glm::pi<float>() * square(nDotHSq) * square(alphaSq + tanThetaSq));
    };

    // GGX geometry function + Smith's method.
    auto const geometry = [](FVec8 alphaSq, FVec8 nDotI, FVec8 nDotO, FVec8 hDotI, FVec8 hDotO) {
        auto const partial = [alphaSq](FVec8 nDotR) {
            auto const nDotRSq = square(nDotR);
            return 1.0f + sqrt(1.0f + alphaSq / nDotRSq - alphaSq);
        };

        // assert(nDotI > 0.0f && nDotO > 0.0f && hDotI > 0.0f && hDotO == hDotI);
        return 4.0f / (partial(nDotI) * partial(nDotO));
    };

    // Fresnel-Schlick equation.
    auto const fresnel = [](FVec3_8 f0, FVec8 hDotO) {
        // assert(hDotO >= 0.0f);
        // Sometimes hDotO is very slightly > 1 due to FP error, which technically invalidates this formula.
        // But this error becomes extremely small when raised to the 5th power, so it doesn't have much effect.
        auto const tmp = FVec3_8{iPow(1.0f - hDotO, 5)};
        return fnma(f0, tmp, f0 + tmp);
    };

    // TODO? allow for >8 bounces
    constexpr unsigned RAY_BOUNCE_LIMIT = 8;        // Must be <= 8 !
    constexpr auto RAY_DEPTH_LIMIT = RAY_BOUNCE_LIMIT + 1;

    FVec8 ndfAlphaSqs{};
    FVec8 geometryAlphaSqs{};
    FVec3_8 f0s{};
    FVec3_8 adjustedColours{};
    std::array<FastFVec3, RAY_DEPTH_LIMIT> emissions;
    FVec8 nDotOs{};
    FVec8 nDotIs{};
    FVec8 nDotHs{};
    FVec8 hDotOs{};
    unsigned depth = 0;
    while (true) {
        auto const intersection = data.bspTree.lineTriNearestIntersection<SurfaceConsideration::FRONT_ONLY>(
            ray, RAY_INTERSECTION_T_MIN);
        if (!intersection) {
            break;
        }

        auto const bounce = depth;

        auto const& material = data.materials[intersection->meshTriIndex.mesh];
        emissions[bounce] = material.emission;

        ++depth;

        if (bounce >= RAY_BOUNCE_LIMIT) {
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

        ndfAlphaSqs[bounce] = material.ndfAlphaSq;
        geometryAlphaSqs[bounce] = material.geometryAlphaSq;
        f0s.insert(bounce, material.f0);
        adjustedColours.insert(bounce, material.adjustedColour);
        nDotOs[bounce] = nDotO;
        nDotIs[bounce] = nDotI;
        nDotHs[bounce] = cosTheta;
        hDotOs[bounce] = hDotO;

        if (nDotI > 0.0f) {
            ray = {point, incident};
        }
        else {
            // Weight of incident light becomes 0.
            break;
        }
    }

    if (depth == 0) {
        return {0.0f, 0.0f, 0.0f};
    }

    assert(depth <= RAY_DEPTH_LIMIT);

    // Cook-Torrance BRDF.
    // assert(hDotO > 0.0f);
    auto const& hDotIs = hDotOs;
    auto const specularFs = fresnel(f0s, hDotOs);
    auto const specularDs = ndf(ndfAlphaSqs, nDotHs);
    auto const specularGs = geometry(geometryAlphaSqs, nDotIs, nDotOs, hDotIs, hDotOs);
    // ray probability = specularD * nDotH / (4 * hDotO)
    auto const diffuses = fnma(specularFs, adjustedColours, adjustedColours) * (4.0f * nDotIs * hDotOs / (specularDs * nDotHs));
    auto const speculars = specularFs * (specularGs * hDotOs / (nDotOs * nDotHs));
    auto const weights = diffuses + conditional(nDotOs > 0.0f, speculars, FVec3_8::zero());

    std::array<FastFVec3, RAY_DEPTH_LIMIT> lightWeights;
    lightWeights[0] = {1.0f, 1.0f, 1.0f};
    for (unsigned i = 0; i < RAY_BOUNCE_LIMIT; ++i) {
        lightWeights[i + 1] = FastFVec3{weights.extract(i)};
    }
    for (unsigned i = 1; i < lightWeights.size(); ++i) {
        lightWeights[i] *= lightWeights[i - 1];
    }

    FastFVec3 outgoingLight{0.0f, 0.0f, 0.0f};
    for (unsigned i = 0; i < depth; ++i) {
        outgoingLight = fma(lightWeights[i], emissions[i], outgoingLight);
    }

    // TODO? light transmission

    return outgoingLight;
}


inline void render(RenderData const& data, Span<PackedFVec3> image) {
    assert(image.size() == data.imageWidth * data.imageHeight);
    std::transform(std::execution::par, IndexIterator<>{0}, IndexIterator<>{image.size()}, image.begin(),
            [&data](std::size_t index) {
        auto const pixelX = index % data.imageWidth;
        auto const pixelY = index / data.imageWidth;
        auto& randomEngine = ::randomEngine;    // Access random engine here to force static initialisation.
        FastFVec3 colour{0.0f, 0.0f, 0.0f};
        for (unsigned i = 0; i < PIXEL_SAMPLE_RATE; ++i) {
            auto const sampleX = pixelX + randomEngine.unitFloatOpen();
            auto const sampleY = pixelY + randomEngine.unitFloatOpen();
            auto const rayDirection = glm::normalize(data.pixelToRayTransform * PackedFVec3{sampleX, sampleY, 1.0f});
            Line const ray{data.cameraPosition, rayDirection};
            colour += rayTrace(data.rayTraceData, ray, randomEngine);
        }
        colour /= PIXEL_SAMPLE_RATE;
        return colour.pack();
    });
}
