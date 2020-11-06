#include "mesh.hpp"

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>


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
