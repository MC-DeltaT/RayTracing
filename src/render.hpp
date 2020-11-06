#pragma once

#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "span.hpp"

#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>


struct MeshData {
    Span<std::vector<Vertex> const> verticesWorld;
    Span<std::vector<MeshTri> const> tris;
    Span<MeshInstance const> instances;
};


struct Lights {
    Span<PointLight const> point;
    Span<DirectionalLight const> directional;
    Span<SpotLight const> spot;
};


struct RenderData {
    unsigned imageWidth;
    unsigned imageHeight;
    glm::mat3 pixelToRayTransform;
    glm::vec3 cameraPosition;
    Lights lights;
    MeshData meshData;
    Span<Material const> materials;
    unsigned maxRayDepth;
};


void render(RenderData const& data, Span<glm::vec3> image);
