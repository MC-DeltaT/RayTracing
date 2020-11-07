#pragma once

#include "camera.hpp"
#include "light.hpp"
#include "material.hpp"
#include "mesh.hpp"

#include <cstddef>
#include <initializer_list>
#include <utility>
#include <vector>


struct Meshes {
    std::vector<Vertex> vertices;
    std::vector<MeshTri> tris;
    std::vector<Mesh> meshes;

    Meshes(std::initializer_list<std::pair<std::vector<Vertex>, std::vector<MeshTri>>> meshes);
};


struct Models {
    std::vector<MeshTransform> meshTransforms;
    std::vector<std::size_t> meshes;
    std::vector<std::size_t> materials;
};


struct Lights {
    std::vector<PointLight> point;
    std::vector<DirectionalLight> directional;
    std::vector<SpotLight> spot;
};


struct Scene {
    Camera camera;
    Lights lights;
    Meshes meshes;
    std::vector<Material> materials;
    Models models;
    InstantiatedMeshes instantiatedMeshes;
};
