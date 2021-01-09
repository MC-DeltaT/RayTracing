#pragma once

#include "../basic_types.hpp"

#include <cassert>
#include <cstdint>

#include <immintrin.h>


struct FVec8 {
    __m256 value;

    FVec8() :
        value{}
    {}

    explicit FVec8(__m256 value) :
        value{value}
    {}

    FVec8(float v0, float v1, float v2, float v3, float v4, float v5, float v6, float v7) :
        value{v0, v1, v2, v3, v4, v5, v6, v7}
    {}

    explicit FVec8(float f) :
        value{_mm256_set1_ps(f)}
    {}

    float& operator[](unsigned index) {
        assert(index < 8);
        return value.m256_f32[index];
    }

    float const& operator[](unsigned index) const {
        assert(index < 8);
        return value.m256_f32[index];
    }

    operator __m256&() {
        return value;
    }

    operator __m256 const&() const {
        return value;
    }

    static FVec8 zero() {
        return FVec8{_mm256_setzero_ps()};
    }
};


struct U32Vec8 {
    __m256i value;

    U32Vec8() :
        value{}
    {}

    explicit U32Vec8(__m256i value) :
        value{value}
    {}

    std::uint32_t& operator[](unsigned index) {
        assert(index < 8);
        return value.m256i_u32[index];
    }

    std::uint32_t const& operator[](unsigned index) const {
        assert(index < 8);
        return value.m256i_u32[index];
    }

    operator __m256i&() {
        return value;
    }

    operator __m256i const&() const {
        return value;
    }
};


struct Vec3_8 {
    FVec8 x;
    FVec8 y;
    FVec8 z;

    static Vec3_8 fromVec3(vec3 v0, vec3 v1, vec3 v2, vec3 v3, vec3 v4, vec3 v5, vec3 v6, vec3 v7) {
        return {
            {v0.x, v1.x, v2.x, v3.x, v4.x, v5.x, v6.x, v7.x},
            {v0.y, v1.y, v2.y, v3.y, v4.y, v5.y, v6.y, v7.y},
            {v0.z, v1.z, v2.z, v3.z, v4.z, v5.z, v6.z, v7.z}
        };
    }
};


inline FVec8 operator+(FVec8 a, FVec8 b) {
    return FVec8{_mm256_add_ps(a, b)};
}


inline FVec8 operator-(FVec8 a, FVec8 b) {
    return FVec8{_mm256_sub_ps(a, b)};
}


inline FVec8 operator-(float a, FVec8 b) {
    return FVec8{a} - b;
}


inline FVec8 operator-(FVec8 x) {
    return FVec8::zero() - x;
}


inline FVec8 operator*(FVec8 a, FVec8 b) {
    return FVec8{_mm256_mul_ps(a, b)};
}


inline FVec8 operator*(FVec8 a, float b) {
    return a * FVec8{b};
}


inline FVec8 operator/(FVec8 a, FVec8 b) {
    return FVec8{_mm256_div_ps(a, b)};
}


inline FVec8 operator/(float a, FVec8 b) {
    return FVec8{a} / b;
}


inline U32Vec8 operator<=(FVec8 a, float b) {
    return U32Vec8{_mm256_castps_si256(_mm256_cmp_ps(a, FVec8{b}, _CMP_LE_OQ))};
}


inline U32Vec8 operator>=(FVec8 a, float b) {
    return U32Vec8{_mm256_castps_si256(_mm256_cmp_ps(a, FVec8{b}, _CMP_GE_OQ))};
}


inline FVec8 abs(FVec8 x) {
    return FVec8{_mm256_and_ps(x, _mm256_castsi256_ps(_mm256_srli_epi32(_mm256_set1_epi32(-1), 1)))};
}


inline FVec8 fma(FVec8 a, FVec8 b, FVec8 c) {
    return FVec8{_mm256_fmadd_ps(a, b, c)};
}


inline FVec8 fms(FVec8 a, FVec8 b, FVec8 c) {
    return FVec8{_mm256_fmsub_ps(a, b, c)};
}


inline U32Vec8 operator&(U32Vec8 a, U32Vec8 b) {
    return U32Vec8{_mm256_and_si256(a, b)};
}


inline Vec3_8 operator-(vec3 a, Vec3_8 b) {
    return {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
}


inline Vec3_8 operator*(Vec3_8 a, vec3 b) {
    return {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z
    };
}


inline Vec3_8 operator*(Vec3_8 a, Vec3_8 b) {
    return {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z
    };
}


inline FVec8 dot(Vec3_8 a, vec3 b) {
    return fma(a.z, FVec8{b.z}, fma(a.y, FVec8{b.y}, a.x * b.x));
}


inline FVec8 dot(Vec3_8 a, Vec3_8 b) {
    return fma(a.z, b.z, fma(a.y, b.y, a.x * b.x));
}


inline Vec3_8 cross(Vec3_8 a, vec3 b) {
    return {
        fms(a.y, FVec8{b.z}, a.z * b.y),
        fms(a.z, FVec8{b.x}, a.x * b.z),
        fms(a.x, FVec8{b.y}, a.y * b.x)
    };
}
