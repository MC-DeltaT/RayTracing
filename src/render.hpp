#pragma once

#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "span.hpp"

#include <cstddef>

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>


struct RenderData {
    unsigned imageWidth;
    unsigned imageHeight;
    glm::vec3 cameraPosition;
    glm::mat3 pixelToRayTransform;
    struct Models {
        struct Meshes {
            Span<Vertex const> vertices;
            Span<MeshTri const> tris;
            Span<Mesh const> meshes;
        } meshes;
        Span<std::size_t const> materials;
    } models;
    struct Lights {
        Span<PointLight const> point;
        Span<DirectionalLight const> directional;
        Span<SpotLight const> spot;
    } lights;
    Span<Material const> materials;
};


void render(RenderData const& data, Span<glm::vec3> image);
