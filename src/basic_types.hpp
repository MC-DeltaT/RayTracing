#pragma once

#include "utility/numeric.hpp"

#include <cstdint>

#include <glm/vec3.hpp>


using vec3 = glm::vec3;

using VertexIndex = std::uint16_t;
using TriIndex = std::uint16_t;
using MeshIndex = std::uint16_t;
using MaterialIndex = std::uint16_t;

using VertexRange = IndexRange<std::uint32_t>;
using TriRange = IndexRange<std::uint32_t>;
