#pragma once

#include "geometry.hpp"
#include "index_types.hpp"
#include "utility/numeric.hpp"
#include "utility/span.hpp"
#include "utility/permuted_span.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>


// MESH STORAGE:
//   Each mesh is fundamentally an array of triangles (tris). However, vertices are often duplicated between tris, so
//   for memory efficiency we store an array of vertices (positions and normals), and each tri contains the indices of
//   its vertices.
//   A vertex consists of a position and, if required, a normal vector. For cache performance, we store the positions
//   and normals as separate arrays (structure-of-arrays rather than array-of-structures).
//   Therefore, for a single mesh we have 3 arrays: vertex positions, vertex normals, and tris.
//
//   When dealing with an array of meshes, for cache performance, we concatenate each of the vertex and tri arrays of
//   the meshes together. In order to know which vertices and tris belong to which mesh, we introduce arrays of
//   "vertex ranges" and "tri ranges", which map from mesh index to a range of indices within the concatenated vertex
//   and tri arrays, respectively. We take the vertex indices contained in the tris to be relative to that mesh's vertex
//   range.
//   Therefore, we end up with 5 arrays: vertex positions, vertex normals, tris, vertex ranges, and tri ranges. These 5
//   arrays, or subsets of them, cen be frequently seen throughout the mesh handling.
//   Unfortunately the multiple levels of indexing required can become confusing, however it is required for high
//   performance.


struct IndexedTri {
    VertexIndex v1;
    VertexIndex v2;
    VertexIndex v3;
};


// Index of a tri in a mesh.
struct MeshTriIndex {
    MeshIndex mesh;
    TriIndex tri;
};


// Physical transformation of a mesh's vertices.
struct MeshTransform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    glm::mat4x3 matrix() const {
        glm::mat4x3 m{glm::mat3_cast(orientation)};
        m[0] *= scale.x;
        m[1] *= scale.y;
        m[2] *= scale.z;
        m[3] = position;
        return m;
    }
};


// Calculates the vertex normal transformation matrix from the mesh transformation.
inline glm::mat3 normalTransform(glm::mat4 modelTransform) {
    return glm::inverseTranspose(glm::mat3{modelTransform});
}


struct InstantiatedMeshes {
    std::vector<glm::vec3> vertexPositions;
    std::vector<glm::vec3> vertexNormals;
    std::vector<VertexRange> vertexRanges;      // Maps from mesh instance index to range of vertices.
};


// Takes a set of "base" (template) meshes and applies transformations to their vertices, producing a new set of
// "instantiated" meshes.
// Produces a new set of vertex positions, vertex normals, and vertex ranges, which specify the instantiated meshes.
// The tris and tri ranges are unchanged and can be reused from the base meshes.
inline InstantiatedMeshes instantiateMeshes(Span<glm::vec3 const> vertexPositions, Span<glm::vec3 const> vertexNormals,
        Span<VertexRange const> vertexRanges, Span<MeshTransform const> instanceTransforms,
        Span<MeshIndex const> instanceMeshes) {
    assert(vertexPositions.size() == vertexNormals.size());
    assert(instanceTransforms.size() == instanceMeshes.size());

    auto const instanceCount = instanceTransforms.size();
    PermutedSpan const instanceVertexRanges{vertexRanges, instanceMeshes};

    InstantiatedMeshes result;

    {
        std::size_t vertexCount = 0;
        for (auto const& range : instanceVertexRanges) {
            vertexCount += range.size;
        }
        result.vertexPositions.resize(vertexCount);
        result.vertexNormals.resize(vertexCount);
    }
    result.vertexRanges.resize(instanceCount);

    std::size_t verticesOffset = 0;
    for (std::size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex) {
        auto const modelTransform = instanceTransforms[instanceIndex].matrix();
        auto const& vertexRange = instanceVertexRanges[instanceIndex];
        {
            auto const meshVertexPositions = vertexPositions[vertexRange];
            std::transform(meshVertexPositions.begin(), meshVertexPositions.end(), result.vertexPositions.begin() + verticesOffset,
                [&modelTransform](auto const& position) {
                    return modelTransform * glm::vec4{position, 1.0f};
                });
        }
        {
            auto const normalTransform = ::normalTransform(modelTransform);
            auto const meshVertexNormals = vertexNormals[vertexRange];
            std::transform(meshVertexNormals.begin(), meshVertexNormals.end(), result.vertexNormals.begin() + verticesOffset,
                    [&normalTransform](auto const& normal) {
                    return glm::normalize(normalTransform * normal);
                });
        }
        result.vertexRanges[instanceIndex] = {
            intCast<VertexRange::IndexType>(verticesOffset),
            intCast<VertexRange::SizeType>(vertexRange.size)
        };
        verticesOffset += vertexRange.size;
    }

    return result;
}


struct PreprocessedTris {
    std::vector<PreprocessedTri> tris;
    std::vector<TriRange> triRanges;        // Maps from mesh index to range of preprocessed tris.
};


// Preprocesses the tris of a set of meshes.
// Produces a new set of tri ranges mapping from mesh index to range of preprocessed tris.
inline PreprocessedTris preprocessTris(Span<glm::vec3 const> vertexPositions, Span<VertexRange const> vertexRanges,
        Span<IndexedTri const> tris, PermutedSpan<TriRange const, MeshIndex> triRanges) {
    assert(vertexRanges.size() == triRanges.size());

    auto const instanceCount = vertexRanges.size();

    PreprocessedTris result;

    {
        std::size_t triCount = 0;
        for (std::size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex) {
            triCount += triRanges[instanceIndex].size;
        }
        result.tris.resize(triCount);
    }
    result.triRanges.resize(instanceCount);
    
    std::size_t trisOffset = 0;
    for (std::size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex) {
        auto const instanceVertexPositions = vertexPositions[vertexRanges[instanceIndex]];
        auto const instanceTris = tris[triRanges[instanceIndex]];
        for (std::size_t i = 0; i < instanceTris.size(); ++i) {
            auto const& meshTri = instanceTris[i];
            Tri const tri{
                instanceVertexPositions[meshTri.v1],
                instanceVertexPositions[meshTri.v2],
                instanceVertexPositions[meshTri.v3]
            };
            auto const preprocessedTri = preprocessTri(tri);
            result.tris[trisOffset + i] = preprocessedTri;
        }
        result.triRanges[instanceIndex] = {
            intCast<TriRange::IndexType>(trisOffset),
            intCast<TriRange::SizeType>(instanceTris.size())
        };
        trisOffset += instanceTris.size();
    }

    return result;
}
