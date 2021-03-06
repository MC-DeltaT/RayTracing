#pragma once

#include <cstdint>
#include <random>

#include <glm/gtc/constants.hpp>


// Probably doesn't pass any randomness tests, but is super fast.
class FastRNG {
public:
    FastRNG(std::uint64_t state) :
        _state{state}
    {}

    // Generates random uint in range [0, 2^32).
    std::uint32_t value() {
        _state = (214013 * _state + 2531011); 
        return static_cast<std::uint32_t>(_state >> 16);
    }

    // Generates random float in range [0, 1).
    float unitFloatOpen() {
        return static_cast<float>(value()) / 0x100000000;
    }

    // Generates random float in range [0, 2*pi).
    float angle() {
        return glm::two_pi<float>() * unitFloatOpen();;
    }

private:
    std::uint64_t _state;
};


inline static thread_local FastRNG randomEngine{std::random_device{}()};
