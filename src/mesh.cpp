#include "mesh.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>


glm::mat4 MeshTransform::matrix() const {
    glm::mat4 m{1.0f};
    m = glm::translate(m, position);
    m *= glm::mat4_cast(orientation);
    m = glm::scale(m, scale);
    return m;
}


glm::mat3 normalTransform(glm::mat4 modelTransform) {
    return glm::inverseTranspose(glm::mat3{modelTransform});
}


void instantiateMeshes(Span<Vertex const> vertices, Span<Mesh const> meshes,
        Span<MeshTransform const> instanceTransforms, Span<std::size_t const> instanceMeshes,
        InstantiatedMeshes& result) {
    assert(instanceTransforms.size() == instanceMeshes.size());
    auto const instanceCount = instanceTransforms.size();

    result.vertices.clear();
    result.meshes.clear();
    for (std::size_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex) {
        auto const& mesh = meshes[instanceMeshes[instanceIndex]];
        auto const& transform = instanceTransforms[instanceIndex];
        auto const modelTransform = transform.matrix();
        auto const normalTransform = ::normalTransform(modelTransform);
        auto const verticesOffset = result.vertices.size();
        auto it = result.vertices.insert(result.vertices.cend(), vertices.begin() + mesh.verticesOffset,
            vertices.begin() + mesh.verticesOffset + mesh.vertexCount);
        std::for_each_n(it, mesh.vertexCount, [&modelTransform, &normalTransform](auto& vertex) {
            vertex.position = modelTransform * glm::vec4{vertex.position, 1.0f};
            vertex.normal = glm::normalize(normalTransform * vertex.normal);
        });
        result.meshes.push_back({verticesOffset, mesh.vertexCount, mesh.trisOffset, mesh.triCount});
    }
}
