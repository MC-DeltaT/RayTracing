#include "scene.hpp"

#include "utility/numeric.hpp"

#include <cstddef>


Meshes::Meshes(std::initializer_list<std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>,
        std::vector<IndexedTri>>> meshes) {
    vertexRanges.reserve(meshes.size());
    triRanges.reserve(meshes.size());
    std::size_t verticesOffset = 0;
    std::size_t trisOffset = 0;
    for (auto const& [vertexPositions, vertexNormals, tris] : meshes) {
        assert(vertexPositions.size() == vertexNormals.size());
        this->vertexPositions.insert(this->vertexPositions.cend(), vertexPositions.cbegin(), vertexPositions.cend());
        this->vertexNormals.insert(this->vertexNormals.cend(), vertexNormals.cbegin(), vertexNormals.cend());
        this->tris.insert(this->tris.cend(), tris.cbegin(), tris.cend());
        vertexRanges.push_back({
            intCast<VertexRange::IndexType>(verticesOffset),
            intCast<VertexRange::SizeType>(vertexPositions.size())
        });
        triRanges.push_back({
            intCast<TriRange::IndexType>(trisOffset),
            intCast<TriRange::SizeType>(tris.size())
        });
        verticesOffset += vertexPositions.size();
        trisOffset += tris.size();
    }
}
