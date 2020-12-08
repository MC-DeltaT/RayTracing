#pragma once

#include <cstddef>


struct IndexRange {
    std::size_t begin;
    std::size_t size;

    std::size_t end() const {
        return begin + size;
    }
};
