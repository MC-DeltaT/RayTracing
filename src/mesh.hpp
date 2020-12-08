#pragma once

#include "utility/misc.hpp"
#include "utility/span.hpp"
#include "utility/permuted_span.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x3.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>


struct MeshTri {
    unsigned i1;
    unsigned i2;
    unsigned i3;
};


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


struct InstantiatedMeshes {
    std::vector<glm::vec3> vertexPositions;
    std::vector<glm::vec3> vertexNormals;
    std::vector<IndexRange> vertexRanges;       // Maps from mesh instance index to range of vertices.
};


inline glm::mat3 normalTransform(glm::mat4 modelTransform) {
    return glm::inverseTranspose(glm::mat3{modelTransform});
}


inline void instantiateMeshes(Span<glm::vec3 const> vertexPositions, Span<glm::vec3 const> vertexNormals,
        Span<IndexRange const> vertexRanges, Span<MeshTransform const> instanceTransforms,
        Span<std::size_t const> instanceMeshes, InstantiatedMeshes& result) {
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
        result.vertexRanges[instanceIndex] = {verticesOffset, vertexRange.size};
        verticesOffset += vertexRange.size;
    }
}
