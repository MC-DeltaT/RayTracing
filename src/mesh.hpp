#pragma once

#include <array>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>


struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 textureCoords;
};


struct MeshTri {
    unsigned i1;
    unsigned i2;
    unsigned i3;
};


struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<MeshTri> tris;
};


struct MeshTransform {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    glm::mat4 matrix() const;
};


struct MeshInstance {
    std::size_t mesh;
    std::size_t material;
    MeshTransform transform;
};


glm::mat3 normalTransform(glm::mat4 modelTransform);
