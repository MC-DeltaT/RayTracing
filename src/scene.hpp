#pragma once

#include "basic_types.hpp"
#include "camera.hpp"
#include "geometry.hpp"
#include "material.hpp"
#include "mesh.hpp"

#include <initializer_list>
#include <tuple>
#include <vector>


struct Meshes {
    std::vector<PackedFVec3> vertexPositions;
    std::vector<PackedFVec3> vertexNormals;
    std::vector<MeshTri> tris;
    std::vector<VertexRange> vertexRanges;
    std::vector<TriRange> triRanges;

    Meshes(std::initializer_list<std::tuple<std::vector<PackedFVec3>, std::vector<PackedFVec3>,
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
