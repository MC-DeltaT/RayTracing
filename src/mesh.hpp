#pragma once

#include "basic_types.hpp"
#include "geometry.hpp"
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

#include <glm/vec4.hpp>


struct MeshTri {
    VertexIndex v1;
    VertexIndex v2;
    VertexIndex v3;
};


struct MeshTriIndex {
    MeshIndex mesh;
    TriIndex tri;
};


struct MeshTransform {
    PackedFVec3 position{0.0f, 0.0f, 0.0f};
    glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
    PackedFVec3 scale{1.0f, 1.0f, 1.0f};

    glm::mat4x3 matrix() const {
        glm::mat4x3 m{glm::mat3_cast(orientation)};
        m[0] *= scale.x;
        m[1] *= scale.y;
        m[2] *= scale.z;
        m[3] = position;
        return m;
    }
};


struct InstantiatedMeshes {
    std::vector<PackedFVec3> vertexPositions;
    std::vector<PackedFVec3> vertexNormals;
    std::vector<VertexRange> vertexRanges;      // Maps from mesh instance index to range of vertices.
};


inline glm::mat3 normalTransform(glm::mat4 modelTransform) {
    return glm::inverseTranspose(glm::mat3{modelTransform});
}


inline void instantiateMeshes(Span<PackedFVec3 const> vertexPositions, Span<PackedFVec3 const> vertexNormals,
        Span<VertexRange const> vertexRanges, Span<MeshTransform const> instanceTransforms,
        Span<MeshIndex const> instanceMeshes, InstantiatedMeshes& result) {
    assert(vertexPositions.size() == vertexNormals.size());
    assert(instanceTransforms.size() == instanceMeshes.size());

    auto const instanceCount = instanceTransforms.size();
    PermutedSpan const instanceVertexRanges{vertexRanges, instanceMeshes};

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
}


inline void preprocessTris(Span<PackedFVec3 const> vertexPositions, Span<VertexRange const> vertexRanges,
        Span<MeshTri const> tris, PermutedSpan<TriRange const, MeshIndex> triRanges,
        std::vector<PreprocessedTri>& resultTris, std::vector<TriRange>& resultTriRanges) {
    assert(vertexRanges.size() == triRanges.size());

    auto const instanceCount = vertexRanges.size();

    {
        std::size_t triCount = 0;
        for (std::size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex) {
            triCount += triRanges[instanceIndex].size;
        }
        resultTris.resize(triCount);
    }
    resultTriRanges.resize(instanceCount);
    
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
            resultTris[trisOffset + i] = preprocessedTri;
        }
        resultTriRanges[instanceIndex] = {
            intCast<TriRange::IndexType>(trisOffset),
            intCast<TriRange::SizeType>(instanceTris.size())
        };
        trisOffset += instanceTris.size();
    }
}
