#pragma once

#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "ray.hpp"
#include "utility.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <execution>
#include <optional>

#include <boost/range/counting_range.hpp>
#include <glm/geometric.hpp>
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
    Span<SpotLight const> spotLights;
};


struct RenderData {
    unsigned imageWidth;
    unsigned imageHeight;
    glm::vec3 cameraPosition;
    glm::mat3 pixelToRayTransform;
    RayTraceData rayTraceData;
};


struct RayCastResult {
    glm::vec3 colour;
    Material const& material;
    glm::vec3 point;
    glm::vec3 normal;
};


constexpr inline unsigned RAY_BOUNCE_LIMIT = 4;


inline std::optional<RayCastResult> rayCast(RayTraceData const& data,
        Ray const& ray) {
    auto const intersection = rayNearestIntersection(data.preprocessedTris, data.preprocessedTriRanges, ray);
    if (!intersection) {
        return std::nullopt;
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

    glm::vec3 colour{0.0f, 0.0f, 0.0f};
    for (auto const& light : data.pointLights) {
        auto const pointToLight = light.position - point;
        auto const lightDistance = glm::length(pointToLight);
        
        auto const rawColour = light.colour * material.colour;
        auto const diffuseColour = material.diffuse * rawColour;

        auto const ambient = light.ambientStrength * diffuseColour;
        colour += ambient;

        Ray const shadowRay{point, pointToLight};
        if (!rayIntersectsAny(data.preprocessedTris, data.preprocessedTriRanges, shadowRay, 1.0f)) {
            auto const lightRay = -pointToLight / lightDistance;
            auto const diffuseCoeff = std::max(glm::dot(normal, -lightRay), 0.0f);
            auto const diffuse = diffuseCoeff * diffuseColour;
            colour += diffuse;

            auto const pointToViewer = glm::normalize(ray.origin - point);
            auto const reflectedRay = glm::reflect(lightRay, normal);
            auto const specularCoeff = std::pow(std::max(glm::dot(pointToViewer, reflectedRay), 0.0f), material.shininess);
            auto const specular = specularCoeff * material.specular * rawColour;
            colour += specular;
        }
    }
    // TODO: other light types

    return {{colour, material, point, normal}};
}


inline glm::vec3 rayTrace(RayTraceData const& data, Ray ray) {
    glm::vec3 colour{0.0f, 0.0f, 0.0f};
    glm::vec3 coeff{1.0f, 1.0f, 1.0f};
    for (unsigned bounce = 0; bounce <= RAY_BOUNCE_LIMIT; ++bounce) {
        auto const castResult = rayCast(data, ray);
        if (!castResult) {
            break;
        }
        colour += coeff * castResult->colour;
        coeff *= castResult->material.colour * castResult->material.reflectivity;
        ray = {castResult->point, glm::reflect(ray.direction, castResult->normal)};
    }
    return colour;
}


inline void render(RenderData const& data, Span<glm::vec3> image) {
    assert(image.size() == data.imageWidth * data.imageHeight);
    auto const pixelRange = boost::counting_range(0ull, image.size());
    std::transform(std::execution::par_unseq, pixelRange.begin(), pixelRange.end(), image.begin(), [&data](auto index) {
        auto const imageX = index % data.imageWidth;
        auto const imageY = index / data.imageWidth;
        auto const rayDirection = data.pixelToRayTransform * glm::vec3{imageX, imageY, 1.0f};
        Ray const ray{data.cameraPosition, rayDirection};
        auto const colour = rayTrace(data.rayTraceData, ray);
        return colour;
    });
}
