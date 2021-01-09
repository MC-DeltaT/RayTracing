#pragma once

#include "../basic_types.hpp"

#include <immintrin.h>


struct Vec3_8 {
    __m256 x;
    __m256 y;
    __m256 z;

    static Vec3_8 fromVec3(vec3 v0, vec3 v1, vec3 v2, vec3 v3, vec3 v4, vec3 v5, vec3 v6, vec3 v7) {
        return {
            {v0.x, v1.x, v2.x, v3.x, v4.x, v5.x, v6.x, v7.x},
            {v0.y, v1.y, v2.y, v3.y, v4.y, v5.y, v6.y, v7.y},
            {v0.z, v1.z, v2.z, v3.z, v4.z, v5.z, v6.z, v7.z}
        };
    }
};


inline __m256 mm256Broadcast(float x) {
    return _mm256_set1_ps(x);
}


inline __m256 operator+(__m256 a, __m256 b) {
    return _mm256_add_ps(a, b);
}


inline __m256 operator-(__m256 a, __m256 b) {
    return _mm256_sub_ps(a, b);
}


inline __m256 operator-(float a, __m256 b) {
    return _mm256_sub_ps(mm256Broadcast(a), b);
}


inline __m256 operator-(__m256 x) {
    return _mm256_setzero_ps() - x;
}


inline __m256 operator*(__m256 a, __m256 b) {
    return _mm256_mul_ps(a, b);
}


inline __m256 operator*(__m256 a, float b) {
    return a * mm256Broadcast(b);
}


inline __m256 operator/(__m256 a, __m256 b) {
    return _mm256_div_ps(a, b);
}


inline __m256 operator/(float a, __m256 b) {
    return mm256Broadcast(a) / b;
}


inline __m256i operator<=(__m256 a, float b) {
    return _mm256_castps_si256(_mm256_cmp_ps(a, mm256Broadcast(b), _CMP_LE_OQ));
}


inline __m256i operator>=(__m256 a, float b) {
    return _mm256_castps_si256(_mm256_cmp_ps(a, mm256Broadcast(b), _CMP_GE_OQ));
}


inline __m256 abs(__m256 x) {
    return _mm256_and_ps(x, _mm256_castsi256_ps(_mm256_srli_epi32(_mm256_set1_epi32(-1), 1)));
}


inline __m256 fma(__m256 a, __m256 b, __m256 c) {
    return _mm256_fmadd_ps(a, b, c);
}


inline __m256 fms(__m256 a, __m256 b, __m256 c) {
    return _mm256_fmsub_ps(a, b, c);
}


inline __m256i operator&(__m256i a, __m256i b) {
    return _mm256_and_si256(a, b);
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


inline __m256 dot(Vec3_8 a, vec3 b) {
    return fma(a.z, mm256Broadcast(b.z), fma(a.y, mm256Broadcast(b.y), a.x * b.x));
}


inline __m256 dot(Vec3_8 a, Vec3_8 b) {
    return fma(a.z, b.z, fma(a.y, b.y, a.x * b.x));
}


inline Vec3_8 cross(Vec3_8 a, vec3 b) {
    return {
        fms(a.y, mm256Broadcast(b.z), a.z * b.y),
        fms(a.z, mm256Broadcast(b.x), a.x * b.z),
        fms(a.x, mm256Broadcast(b.y), a.y * b.x)
    };
}
