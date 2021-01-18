#pragma once

#include "../basic_types.hpp"

#include <cassert>
#include <cstdint>

#include <glm/vec3.hpp>
#include <immintrin.h>
#include <xmmintrin.h>


using PackedFVec3 = glm::vec3;


using PackedU8Vec3 = glm::u8vec3;


struct FVec4 {
    __m128 data;

    FVec4() = default;

    explicit FVec4(__m128 data) :
        data{data}
    {}

    FVec4(float v0, float v1, float v2, float v3) :
        data{v0, v1, v2, v3}
    {}

    explicit FVec4(float f) :
        data{_mm_set1_ps(f)}
    {}

    float& operator[](unsigned index) {
        assert(index < 4);
        return data.m128_f32[index];
    }

    float const& operator[](unsigned index) const {
        assert(index < 4);
        return data.m128_f32[index];
    }

    FVec4& operator+=(FVec4 rhs) {
        FVec4 operator+(FVec4, FVec4);
        *this = *this + rhs;
        return *this;
    }

    FVec4& operator*=(FVec4 rhs) {
        FVec4 operator*(FVec4, FVec4);
        *this = *this * rhs;
        return *this;
    }

    FVec4& operator/=(float rhs) {
        FVec4 operator/(FVec4, float);
        *this = *this / rhs;
        return *this;
    }

    static FVec4 zero() {
        return FVec4{_mm_setzero_ps()};
    }
};


struct FastFVec3 {
    FVec4 data;

    FastFVec3() = default;

    explicit FastFVec3(FVec4 data) :
        data{data}
    {}

    explicit FastFVec3(PackedFVec3 v) :
        data{v.x, v.y, v.z, 0.0f}
    {}

    FastFVec3(float x, float y, float z) :
        data{x, y, z, 0.0f}
    {}

    explicit FastFVec3(float f) :
        data{f}
    {}

    PackedFVec3 pack() {
        return {data[0], data[1], data[2]};
    }

    float& operator[](unsigned index) {
        assert(index < 3);
        return data[index];
    }

    float const& operator[](unsigned index) const {
        assert(index < 3);
        return data[index];
    }

    FastFVec3& operator+=(FastFVec3 rhs) {
        data += rhs.data;
        return *this;
    }

    FastFVec3& operator*=(FastFVec3 rhs) {
        data *= rhs.data;
        return *this;
    }

    FastFVec3& operator/=(float rhs) {
        data /= rhs;
        return *this;
    }
};


struct FVec8 {
    __m256 data;

    FVec8() = default;

    explicit FVec8(__m256 data) :
        data{data}
    {}

    FVec8(float v0, float v1, float v2, float v3, float v4, float v5, float v6, float v7) :
        data{v0, v1, v2, v3, v4, v5, v6, v7}
    {}

    explicit FVec8(float f) :
        data{_mm256_set1_ps(f)}
    {}

    float& operator[](unsigned index) {
        assert(index < 8);
        return data.m256_f32[index];
    }

    float const& operator[](unsigned index) const {
        assert(index < 8);
        return data.m256_f32[index];
    }

    FVec8& operator*=(FVec8 rhs) {
        FVec8 operator*(FVec8, FVec8);
        *this = *this * rhs;
        return *this;
    }

    static FVec8 zero() {
        return FVec8{_mm256_setzero_ps()};
    }
};


struct U32Vec8 {
    __m256i data;

    U32Vec8() = default;

    explicit U32Vec8(__m256i data) :
        data{data}
    {}

    U32Vec8(std::uint32_t v0, std::uint32_t v1, std::uint32_t v2, std::uint32_t v3,
        std::uint32_t v4, std::uint32_t v5, std::uint32_t v6, std::uint32_t v7)
    {
        data.m256i_u32[0] = v0;
        data.m256i_u32[1] = v1;
        data.m256i_u32[2] = v2;
        data.m256i_u32[3] = v3;
        data.m256i_u32[4] = v4;
        data.m256i_u32[5] = v5;
        data.m256i_u32[6] = v6;
        data.m256i_u32[7] = v7;
    }

    std::uint32_t& operator[](unsigned index) {
        assert(index < 8);
        return data.m256i_u32[index];
    }

    std::uint32_t const& operator[](unsigned index) const {
        assert(index < 8);
        return data.m256i_u32[index];
    }
};


struct FVec3_8 {
    FVec8 x;
    FVec8 y;
    FVec8 z;

    FVec3_8() = default;

    FVec3_8(FVec8 x, FVec8 y, FVec8 z) :
        x{x}, y{y}, z{z}
    {}

    explicit FVec3_8(FVec8 v) :
        x{v}, y{v}, z{v}
    {}

    FVec3_8(PackedFVec3 v0, PackedFVec3 v1, PackedFVec3 v2, PackedFVec3 v3,
            PackedFVec3 v4, PackedFVec3 v5, PackedFVec3 v6, PackedFVec3 v7) :
        x{v0.x, v1.x, v2.x, v3.x, v4.x, v5.x, v6.x, v7.x},
        y{v0.y, v1.y, v2.y, v3.y, v4.y, v5.y, v6.y, v7.y},
        z{v0.z, v1.z, v2.z, v3.z, v4.z, v5.z, v6.z, v7.z}
    {}

    void insert(unsigned index, PackedFVec3 v) {
        x[index] = v.x;
        y[index] = v.y;
        z[index] = v.z;
    }

    PackedFVec3 extract(unsigned index) const {
        return {
            x[index],
            y[index],
            z[index]
        };
    }

    static FVec3_8 zero() {
        return {
            FVec8::zero(),
            FVec8::zero(),
            FVec8::zero()
        };
    }
};


inline FVec4 operator+(FVec4 a, FVec4 b) {
    return FVec4{_mm_add_ps(a.data, b.data)};
}

inline FastFVec3 operator+(FastFVec3 a, FastFVec3 b) {
    return FastFVec3{a.data + b.data};
}

inline FVec8 operator+(FVec8 a, FVec8 b) {
    return FVec8{_mm256_add_ps(a.data, b.data)};
}

inline FVec8 operator+(FVec8 a, float b) {
    return a + FVec8{b};
}

inline FVec8 operator+(float a, FVec8 b) {
    return FVec8{a} + b;
}

inline FVec3_8 operator+(FVec3_8 a, FVec3_8 b) {
    return {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
}


inline FVec4 operator-(FVec4 a, FVec4 b) {
    return FVec4{_mm_sub_ps(a.data, b.data)};
}

inline FVec4 operator-(float a, FVec4 b) {
    return FVec4{a} - b;
}

inline FastFVec3 operator-(float a, FastFVec3 b) {
    return FastFVec3{a - b.data};
}

inline FVec8 operator-(FVec8 a, FVec8 b) {
    return FVec8{_mm256_sub_ps(a.data, b.data)};
}

inline FVec8 operator-(FVec8 a, float b) {
    return a - FVec8{b};
}

inline FVec8 operator-(float a, FVec8 b) {
    return FVec8{a} - b;
}

inline FVec3_8 operator-(float a, FVec3_8 b) {
    return {
        a - b.x,
        a - b.y,
        a - b.z
    };
}

inline FVec3_8 operator-(PackedFVec3 a, FVec3_8 b) {
    return {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
}


inline FVec8 operator-(FVec8 v) {
    return FVec8::zero() - v;
}


inline FVec4 operator*(FVec4 a, FVec4 b) {
    return FVec4{_mm_mul_ps(a.data, b.data)};
}

inline FVec4 operator*(FVec4 a, float b) {
    return a * FVec4{b};
}

inline FastFVec3 operator*(FastFVec3 a, FastFVec3 b) {
    return FastFVec3{a.data * b.data};
}

inline FastFVec3 operator*(FastFVec3 a, float b) {
    return FastFVec3{a.data * b};
}

inline FVec8 operator*(FVec8 a, FVec8 b) {
    return FVec8{_mm256_mul_ps(a.data, b.data)};
}

inline FVec8 operator*(FVec8 a, float b) {
    return a * FVec8{b};
}

inline FVec8 operator*(float a, FVec8 b) {
    return FVec8{a} * b;
}

inline FVec3_8 operator*(FVec3_8 a, FVec3_8 b) {
    return {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z
    };
}

inline FVec3_8 operator*(FVec3_8 a, FVec8 b) {
    return {
        a.x * b,
        a.y * b,
        a.z * b
    };
}

inline FVec3_8 operator*(FVec3_8 a, PackedFVec3 b) {
    return {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z
    };
}


inline FVec4 operator/(FVec4 a, FVec4 b) {
    return FVec4{_mm_div_ps(a.data, b.data)};
}

inline FVec4 operator/(FVec4 a, float b) {
    return a / FVec4{b};
}

inline FastFVec3 operator/(FastFVec3 a, float b) {
    return FastFVec3{a.data / b};
}

inline FVec8 operator/(FVec8 a, FVec8 b) {
    return FVec8{_mm256_div_ps(a.data, b.data)};
}

inline FVec8 operator/(float a, FVec8 b) {
    return FVec8{a} / b;
}


inline U32Vec8 operator&(U32Vec8 a, U32Vec8 b) {
    return U32Vec8{_mm256_and_si256(a.data, b.data)};
}


inline U32Vec8 operator<=(FVec8 a, float b) {
    return U32Vec8{_mm256_castps_si256(_mm256_cmp_ps(a.data, FVec8{b}.data, _CMP_LE_OQ))};
}


inline U32Vec8 operator>(FVec8 a, float b) {
    return U32Vec8{_mm256_castps_si256(_mm256_cmp_ps(a.data, FVec8{b}.data, _CMP_GT_OQ))};
}


inline U32Vec8 operator>=(FVec8 a, float b) {
    return U32Vec8{_mm256_castps_si256(_mm256_cmp_ps(a.data, FVec8{b}.data, _CMP_GE_OQ))};
}


inline FVec8 abs(FVec8 v) {
    auto const mask = _mm256_castsi256_ps(_mm256_set_epi32(
        0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF,
        0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF));
    return FVec8{_mm256_and_ps(v.data, mask)};
}


inline FVec8 sqrt(FVec8 v) {
    return FVec8{_mm256_sqrt_ps(v.data)};
}


// a*b + c
inline FVec4 fma(FVec4 a, FVec4 b, FVec4 c) {
    return FVec4{_mm_fmadd_ps(a.data, b.data, c.data)};
}

// a*b + c
inline FastFVec3 fma(FastFVec3 a, FastFVec3 b, FastFVec3 c) {
    return FastFVec3{fma(a.data, b.data, c.data)};
}

// a*b + c
inline FVec8 fma(FVec8 a, FVec8 b, FVec8 c) {
    return FVec8{_mm256_fmadd_ps(a.data, b.data, c.data)};
}

// a*b + c
inline FVec3_8 fma(FVec3_8 a, FVec3_8 b, FVec3_8 c) {
    return {
        fma(a.x, b.x, c.x),
        fma(a.y, b.y, c.y),
        fma(a.z, b.z, c.z)
    };
}


// -a*b + c
inline FVec8 fnma(FVec8 a, FVec8 b, FVec8 c) {
    return FVec8{_mm256_fnmadd_ps(a.data, b.data, c.data)};
}

// -a*b + c
inline FVec3_8 fnma(FVec3_8 a, FVec3_8 b, FVec3_8 c) {
    return {
        fnma(a.x, b.x, c.x),
        fnma(a.y, b.y, c.y),
        fnma(a.z, b.z, c.z)
    };
}


// a*b - c
inline FVec8 fms(FVec8 a, FVec8 b, FVec8 c) {
    return FVec8{_mm256_fmsub_ps(a.data, b.data, c.data)};
}


inline FVec8 conditional(U32Vec8 cond, FVec8 trueVal, FVec8 falseVal) {
    return FVec8{_mm256_blendv_ps(falseVal.data, trueVal.data, _mm256_castsi256_ps(cond.data))};
}

inline FVec3_8 conditional(U32Vec8 cond, FVec3_8 trueVal, FVec3_8 falseVal) {
    return {
        conditional(cond, trueVal.x, falseVal.x),
        conditional(cond, trueVal.y, falseVal.y),
        conditional(cond, trueVal.z, falseVal.z)
    };
}


inline FVec8 dot(FVec3_8 a, PackedFVec3 b) {
    return fma(a.z, FVec8{b.z}, fma(a.y, FVec8{b.y}, a.x * b.x));
}

inline FVec8 dot(FVec3_8 a, FVec3_8 b) {
    return fma(a.z, b.z, fma(a.y, b.y, a.x * b.x));
}


inline FVec3_8 cross(FVec3_8 a, PackedFVec3 b) {
    return {
        fms(a.y, FVec8{b.z}, a.z * b.y),
        fms(a.z, FVec8{b.x}, a.x * b.z),
        fms(a.x, FVec8{b.y}, a.y * b.x)
    };
}
