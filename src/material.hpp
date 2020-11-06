#pragma once

#include <glm/vec3.hpp>


struct Material {
    glm::vec3 colour;
    float diffuse;
    float specular;
    float reflectivity;
    float shininess;
};
