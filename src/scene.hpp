#pragma once

#include "camera.hpp"
#include "index_types.hpp"
#include "material.hpp"
#include "mesh.hpp"

#include <initializer_list>
#include <tuple>
#include <vector>

#include <glm/vec3.hpp>


// Stores polygon meshes as structure-of-arrays.
// Please see the note on mesh storage in mesh.hpp.
struct Meshes {
    std::vector<glm::vec3> vertexPositions;
    std::vector<glm::vec3> vertexNormals;
    std::vector<IndexedTri> tris;       // Each tri's indices are relative to mesh's vertex range.
    std::vector<VertexRange> vertexRanges;  // Maps mesh index to range of vertices in vertexPositions and vertexNormals.
    std::vector<TriRange> triRanges;        // Maps mesh index to range of tris.

    Meshes(std::initializer_list<std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>,
        std::vector<IndexedTri>>> meshes);
};


struct Scene {
    Camera camera;
    Meshes meshes;
    std::vector<Material> materials;    // Materials for all objects in scene.
    struct Models {     // Data for each object (model) in scene.
        std::vector<MeshTransform> meshTransforms;
        std::vector<MeshIndex> meshes;      // Maps from model index to base mesh index.
        std::vector<MaterialIndex> materials;       // Maps from model index to material index.
    } models;
    InstantiatedMeshes instantiatedMeshes;
    PreprocessedTris preprocessedTris;
    std::vector<PreprocessedMaterial> preprocessedMaterials;
};
