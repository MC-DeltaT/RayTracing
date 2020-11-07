#include "scene.hpp"


Meshes::Meshes(std::initializer_list<std::pair<std::vector<Vertex>, std::vector<MeshTri>>> meshes) {
    for (auto const& [vertices, tris] : meshes) {
        auto const verticesOffset = this->vertices.size();
        this->vertices.insert(this->vertices.cend(), vertices.cbegin(), vertices.cend());
        auto const trisOffset = this->tris.size();
        this->tris.insert(this->tris.cend(), tris.cbegin(), tris.cend());
        this->meshes.push_back({verticesOffset, vertices.size(), trisOffset, tris.size()});
    }
}
