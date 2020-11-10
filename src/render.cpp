#include "render.hpp"

#include "ray.hpp"
#include "span.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <execution>
#include <optional>

#include <boost/iterator/counting_iterator.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>


constexpr unsigned RAY_BOUNCE_LIMIT = 4;


struct RayMeshIntersection {
    RayIntersection intersection;
    std::size_t mesh;
    std::size_t tri;
};


static std::optional<RayMeshIntersection> nearestIntersection(RenderData::Models::Meshes const& meshes, Ray ray) {
    RayMeshIntersection nearestIntersection{{1e6}};
    bool hasIntersection = false;
    for (std::size_t meshIndex = 0; meshIndex < meshes.meshes.size(); ++meshIndex) {
        auto const& mesh = meshes.meshes[meshIndex];
        Span const vertices{meshes.vertices.data() + mesh.verticesOffset, mesh.vertexCount};
        Span const tris{meshes.tris.data() + mesh.trisOffset, mesh.triCount};
        for (std::size_t triIndex = 0; triIndex < tris.size(); ++triIndex) {
            auto const& tri = tris[triIndex];
            auto const& v1 = vertices[tri.i1];
            auto const& v2 = vertices[tri.i2];
            auto const& v3 = vertices[tri.i3];
            Triangle const triangle{v1.position, v2.position, v3.position};
            if (auto const intersection = rayTriIntersection(ray, triangle)) {
                if (intersection->rayParam < nearestIntersection.intersection.rayParam) {
                    nearestIntersection = {*intersection, meshIndex, triIndex};
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

static bool intersectsAny(RenderData::Models::Meshes const& meshes, Ray ray, float paramBound) {
    for (std::size_t meshIndex = 0; meshIndex < meshes.meshes.size(); ++meshIndex) {
        auto const& mesh = meshes.meshes[meshIndex];
        Span const vertices{meshes.vertices.data() + mesh.verticesOffset, mesh.vertexCount};
        Span const tris{meshes.tris.data() + mesh.trisOffset, mesh.triCount};
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

static glm::vec3 rayTrace(RenderData::Models const& models, Span<Material const> materials,
        RenderData::Lights const& lights, Ray ray) {
    glm::vec3 colour{0.0f, 0.0f, 0.0f};
    glm::vec3 coeff{1.0f, 1.0f, 1.0f};
    for (unsigned bounce = 0; bounce <= RAY_BOUNCE_LIMIT; ++bounce) {
        auto const intersection = nearestIntersection(models.meshes, ray);
        if (intersection) {
            auto const& mesh = models.meshes.meshes[intersection->mesh];
            Span const vertices{models.meshes.vertices.data() + mesh.verticesOffset, mesh.vertexCount};
            Span const tris{models.meshes.tris.data() + mesh.trisOffset, mesh.triCount};
            auto const& tri = tris[intersection->tri];
            auto const& v1 = vertices[tri.i1];
            auto const& v2 = vertices[tri.i2];
            auto const& v3 = vertices[tri.i3];
            auto const pointCoord2 = intersection->intersection.pointCoord2;
            auto const pointCoord3 = intersection->intersection.pointCoord3;
            auto const pointCoord1 = 1.0f - pointCoord2 - pointCoord3;
            auto const normal = v1.normal * pointCoord1 + v2.normal * pointCoord2 + v3.normal * pointCoord3;
            auto const& material = materials[models.materials[intersection->mesh]];
            auto const intersectPos = ray.origin + intersection->intersection.rayParam * ray.direction;

            glm::vec3 localColour{0.0f, 0.0f, 0.0f};
            for (auto const& light : lights.point) {
                auto const pointToLight = light.position - intersectPos;
                auto const lightDistance = glm::length(pointToLight);
                
                auto const rawColour = light.colour * material.colour;
                auto const diffuseColour = material.diffuse * rawColour;

                auto const ambient = light.ambientStrength * diffuseColour;
                localColour += ambient;

                Ray const shadowRay{intersectPos, pointToLight};
                if (!intersectsAny(models.meshes, shadowRay, 1.0f)) {
                    auto const lightRay = -pointToLight / lightDistance;
                    auto const diffuseCoeff = std::max(glm::dot(normal, -lightRay), 0.0f);
                    auto const diffuse = diffuseCoeff * diffuseColour;
                    localColour += diffuse;

                    auto const pointToViewer = glm::normalize(ray.origin - intersectPos);
                    auto const reflectedRay = glm::reflect(lightRay, normal);
                    auto const specularCoeff = std::pow(std::max(glm::dot(pointToViewer, reflectedRay), 0.0f), material.shininess);
                    auto const specular = specularCoeff * material.specular * rawColour;
                    localColour += specular;
                }
            }

            colour += coeff * localColour;
            coeff *= material.colour * material.reflectivity;
            ray = {intersectPos, glm::reflect(ray.direction, normal)};
        }
        else {
            break;
        }
    }
    return colour;
}


void render(RenderData const& data, Span<glm::vec3> image) {
    assert(image.size() == data.imageWidth * data.imageHeight);
    auto const begin = boost::make_counting_iterator<std::size_t>(0);
    auto const end = boost::make_counting_iterator<std::size_t>(image.size());
    std::transform(std::execution::par_unseq, begin, end, image.begin(), [&data](auto index) {
        auto const imageX = index % data.imageWidth;
        auto const imageY = index / data.imageWidth;
        auto const rayDirection = data.pixelToRayTransform * glm::vec3{imageX, imageY, 1.0f};
        Ray const ray{data.cameraPosition, rayDirection};
        auto const colour = rayTrace(data.models, data.materials, data.lights, ray);
        return colour;
    });
}
