#pragma once

#include "camera.hpp"
#include "geometry.hpp"
#include "index_types.hpp"
#include "material.hpp"
#include "mesh.hpp"

#include <initializer_list>
#include <tuple>
#include <vector>

#include <glm/vec3.hpp>


struct Meshes {
    std::vector<glm::vec3> vertexPositions;
    std::vector<glm::vec3> vertexNormals;
    std::vector<MeshTri> tris;
    std::vector<VertexRange> vertexRanges;
    std::vector<TriRange> triRanges;

    Meshes(std::initializer_list<std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>,
        std::vector<MeshTri>>> meshes);
};


struct Scene {
    Camera camera;
    Meshes meshes;
    std::vector<Material> materials;
    struct Models {
        std::vector<MeshTransform> meshTransforms;
        std::vector<MeshIndex> meshes;
        std::vector<MaterialIndex> materials;
    } models;
    InstantiatedMeshes instantiatedMeshes;
    std::vector<PreprocessedTri> preprocessedTris;
    std::vector<TriRange> preprocessedTriRanges;
    std::vector<PreprocessedMaterial> preprocessedMaterials;
};
