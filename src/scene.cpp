#include "scene.hpp"

#include <cassert>
#include <cstddef>


Meshes::Meshes(std::initializer_list<std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>,
        std::vector<MeshTri>>> meshes) {
    vertexRanges.reserve(meshes.size());
    triRanges.reserve(meshes.size());
    std::size_t verticesOffset = 0;
    std::size_t trisOffset = 0;
    for (auto const& [vertexPositions, vertexNormals, tris] : meshes) {
        assert(vertexPositions.size() == vertexNormals.size());
        this->vertexPositions.insert(this->vertexPositions.cend(), vertexPositions.cbegin(), vertexPositions.cend());
        this->vertexNormals.insert(this->vertexNormals.cend(), vertexNormals.cbegin(), vertexNormals.cend());
        this->tris.insert(this->tris.cend(), tris.cbegin(), tris.cend());
        vertexRanges.push_back({verticesOffset, vertexPositions.size()});
        triRanges.push_back({trisOffset, tris.size()});
        verticesOffset += vertexPositions.size();
        trisOffset += tris.size();
    }
}

std::size_t Meshes::meshCount() const {
    assert(vertexRanges.size() == triRanges.size());
    return vertexRanges.size();
}
