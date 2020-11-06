#include "render.hpp"

#include "ray.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>


struct RayMeshIntersection {
    RayIntersection intersection;
    std::size_t mesh;
    std::size_t tri;
};


static std::optional<RayMeshIntersection> nearestIntersection(MeshData const& meshData, Ray ray) {
    RayMeshIntersection nearestIntersection{{1e6}};
    bool hasIntersection = false;
    for (std::size_t instanceIndex = 0; instanceIndex < meshData.instances.size(); ++instanceIndex) {
        auto const& meshInstance = meshData.instances[instanceIndex];
        auto const& vertices = meshData.verticesWorld[instanceIndex];
        auto const& tris = meshData.tris[meshInstance.mesh];
        for (std::size_t triIndex = 0; triIndex < tris.size(); ++triIndex) {
            auto const& tri = tris[triIndex];
            auto const& v1 = vertices[tri.i1];
            auto const& v2 = vertices[tri.i2];
            auto const& v3 = vertices[tri.i3];
            Triangle const triangle{v1.position, v2.position, v3.position};
            if (auto const intersection = rayTriIntersection(ray, triangle)) {
                if (intersection->rayParam < nearestIntersection.intersection.rayParam) {
                    nearestIntersection = {*intersection, instanceIndex, triIndex};
                    hasIntersection = true;
                }
            }
        }
    }
    if (hasIntersection) {
        return nearestIntersection;
    }
    else {
        return std::nullopt;
    }
}

static bool intersectsAny(MeshData const& meshData, Ray ray, float paramBound) {
    for (std::size_t instanceIndex = 0; instanceIndex < meshData.instances.size(); ++instanceIndex) {
        auto const& instance = meshData.instances[instanceIndex];
        auto const& vertices = meshData.verticesWorld[instanceIndex];
        auto const& tris = meshData.tris[instance.mesh];
        for (auto const& tri : tris) {
            auto const& v1 = vertices[tri.i1];
            auto const& v2 = vertices[tri.i2];
            auto const& v3 = vertices[tri.i3];
            Triangle const triangle{v1.position, v2.position, v3.position};
            if (auto const intersection = rayTriIntersection(ray, triangle)) {
                if (intersection->rayParam <= paramBound) {
                    return true;
                }
            }
        }
    }
    return false;
}

static glm::vec3 rayTrace(MeshData const& meshData, Span<Material const> materials, Lights const& lights, Ray ray,
        unsigned maxDepth, unsigned depth) {
    glm::vec3 colour{0.0f, 0.0f, 0.0f};
    if (depth < maxDepth) {
        auto const intersection = nearestIntersection(meshData, ray);
        if (intersection) {
            auto const& instance = meshData.instances[intersection->mesh];
            auto const& vertices = meshData.verticesWorld[intersection->mesh];
            auto const& tris = meshData.tris[instance.mesh];
            auto const& tri = tris[intersection->tri];
            auto const& v1 = vertices[tri.i1];
            auto const& v2 = vertices[tri.i2];
            auto const& v3 = vertices[tri.i3];
            auto const pointCoords = intersection->intersection.pointCoords;
            auto const normal = v1.normal * pointCoords.x + v2.normal * pointCoords.y + v3.normal * pointCoords.z;
            auto const& material = materials[instance.material];
            auto const intersectPos = ray.origin + intersection->intersection.rayParam * ray.direction;

            for (auto const& light : lights.point) {
                auto const pointToLight = light.position - intersectPos;
                auto const lightDistance = glm::length(pointToLight);
                
                auto const rawColour = light.colour * material.colour;
                auto const diffuseColour = material.diffuse * rawColour;

                auto const ambient = light.ambientStrength * diffuseColour;
                colour += ambient;

                Ray const shadowRay{intersectPos, pointToLight};
                if (!intersectsAny(meshData, shadowRay, 1.0f)) {
                    auto const lightRay = -pointToLight / lightDistance;
                    auto const diffuseCoeff = std::max(glm::dot(normal, -lightRay), 0.0f);
                    auto const diffuse = diffuseCoeff * diffuseColour;
                    colour += diffuse;

                    auto const pointToViewer = glm::normalize(ray.origin - intersectPos);
                    auto const reflectedRay = glm::reflect(lightRay, normal);
                    auto const specularCoeff = std::pow(std::max(glm::dot(pointToViewer, reflectedRay), 0.0f), material.shininess);
                    auto const specular = specularCoeff * material.specular * rawColour;
                    colour += specular;
                }
            }

            Ray const reflectedRay{intersectPos, glm::reflect(ray.direction, normal)};
            auto const reflection = rayTrace(meshData, materials, lights, reflectedRay, maxDepth, depth + 1);
            colour += material.reflectivity * material.colour * reflection;
        }
    }
    return colour;
}


void render(RenderData const& data, Span<glm::vec3> image) {
    std::size_t pixelIndex = 0;
    for (unsigned imageY = 0; imageY < data.imageHeight; ++imageY) {
        for (unsigned imageX = 0; imageX < data.imageWidth; ++imageX) {
            auto const rayDirection = data.pixelToRayTransform * glm::vec3{imageX, imageY, 1.0f};
            Ray const ray{data.cameraPosition, rayDirection};
            auto const colour = rayTrace(data.meshData, data.materials, data.lights, ray, data.maxRayDepth, 0);
            image[pixelIndex] = colour;
            ++pixelIndex;
        }
    }
}
