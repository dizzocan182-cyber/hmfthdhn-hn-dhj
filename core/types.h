#pragma once
#include <cstdint>
#include <cmath>

// ── Integer / Float aliases ──────────────────────────────────────────────────
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

// ── Vector types ─────────────────────────────────────────────────────────────
struct vec2 {
    f32 x = 0.f, y = 0.f;

    vec2() = default;
    vec2(f32 _x, f32 _y) : x(_x), y(_y) {}

    vec2 operator+(const vec2& o) const { return { x + o.x, y + o.y }; }
    vec2 operator-(const vec2& o) const { return { x - o.x, y - o.y }; }
    vec2 operator*(f32 s)       const { return { x * s, y * s }; }
};

struct vec3 {
    f32 x = 0.f, y = 0.f, z = 0.f;

    vec3() = default;
    vec3(f32 _x, f32 _y, f32 _z) : x(_x), y(_y), z(_z) {}

    vec3 operator+(const vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    vec3 operator-(const vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    vec3 operator*(f32 s)       const { return { x * s, y * s, z * s }; }
    vec3 operator/(f32 s)       const { return { x / s, y / s, z / s }; }

    f32 length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    f32 distance_to(const vec3& o) const {
        return (*this - o).length();
    }

    vec3 normalized() const {
        f32 len = length();
        if (len == 0.f) return {};
        return *this / len;
    }

    f32 dot(const vec3& o) const {
        return x * o.x + y * o.y + z * o.z;
    }

    vec3 cross(const vec3& o) const {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }
};

struct vec4 {
    f32 x = 0.f, y = 0.f, z = 0.f, w = 0.f;

    vec4() = default;
    vec4(f32 _x, f32 _y, f32 _z, f32 _w) : x(_x), y(_y), z(_z), w(_w) {}

    vec4 operator+(const vec4& o) const { return { x + o.x, y + o.y, z + o.z, w + o.w }; }
    vec4 operator-(const vec4& o) const { return { x - o.x, y - o.y, z - o.z, w - o.w }; }
    vec4 operator*(f32 s)       const { return { x * s, y * s, z * s, w * s }; }
};

// ── 4×4 Matrix (row-major) ──────────────────────────────────────────────────
struct matrix4x4 {
    f32 m[4][4] = {};

    f32* operator[](int row)       { return m[row]; }
    const f32* operator[](int row) const { return m[row]; }
};

// ── Free operators ───────────────────────────────────────────────────────────
inline vec3 operator*(f32 s, const vec3& v) { return v * s; }
inline vec4 operator*(f32 s, const vec4& v) { return v * s; }
inline vec2 operator*(f32 s, const vec2& v) { return v * s; }
