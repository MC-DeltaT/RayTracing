#pragma once

#include <cassert>
#include <limits>
#include <type_traits>


template<typename IndexType, typename SizeType = IndexType>
struct IndexRange {
    typedef IndexType IndexType;
    typedef SizeType SizeType;

    IndexType begin;
    SizeType size;

    auto end() const {
        return begin + size;
    }
};


// Casts between integer types safely.
template<typename To, typename From>
To intCast(From val) {
    static_assert(std::is_integral_v<From> && std::is_integral_v<To>);
    if constexpr (!std::numeric_limits<From>::is_signed && !std::numeric_limits<To>::is_signed) {
        assert(val <= std::numeric_limits<To>::max());
    }
    else if constexpr (std::numeric_limits<From>::is_signed && std::numeric_limits<To>::is_signed) {
        assert(val >= std::numeric_limits<To>::min());
        assert(val <= std::numeric_limits<To>::max());
    }
    else if constexpr (std::numeric_limits<From>::is_signed && !std::numeric_limits<To>::is_signed) {
        assert(val >= 0);
        assert(val <= std::numeric_limits<To>::max());
    }
    else if constexpr (!std::numeric_limits<From>::is_signed && std::numeric_limits<To>::is_signed) {
        assert(val <= std::numeric_limits<To>::max());
    }
    return static_cast<To>(val);
}
