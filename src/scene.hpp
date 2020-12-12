#pragma once

#include "camera.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "ray.hpp"
#include "utility/misc.hpp"

#include <cstddef>
#include <initializer_list>
#include <tuple>
#include <vector>

#include <glm/vec3.hpp>


struct Meshes {
    std::vector<glm::vec3> vertexPositions;
    std::vector<glm::vec3> vertexNormals;
    std::vector<MeshTri> tris;
    std::vector<IndexRange> vertexRanges;
    std::vector<IndexRange> triRanges;

    Meshes(std::initializer_list<std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>,
        std::vector<MeshTri>>> meshes);
    
    std::size_t meshCount() const;
};


struct Scene {
    Camera camera;
    Meshes meshes;
    std::vector<Material> materials;
    struct Models {
        std::vector<MeshTransform> meshTransforms;
        std::vector<std::size_t> meshes;
        std::vector<std::size_t> materials;
    } models;
    InstantiatedMeshes instantiatedMeshes;
    std::vector<PreprocessedTri> preprocessedTris;
    std::vector<IndexRange> preprocessedTriRanges;
};
