#pragma once
#include <cmath>
#include <imgui.h>

struct Vec2 {
    float x = 0.f;
    float y = 0.f;

    Vec2() = default;
    Vec2(float _x, float _y) : x(_x), y(_y) {}

    Vec2(const ImVec2& v) : x(v.x), y(v.y) {}

    ImVec2 ToImVec2() const { return ImVec2(x, y); }
    operator ImVec2() const { return ImVec2(x, y); }

    Vec2 operator+(const Vec2& o) const { return { x + o.x, y + o.y }; }
    Vec2 operator-(const Vec2& o) const { return { x - o.x, y - o.y }; }
    Vec2 operator*(float s)        const { return { x * s,   y * s }; }
    Vec2 operator/(float s)        const { return { x / s,   y / s }; }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s;   y *= s;   return *this; }
    Vec2& operator/=(float s) { x /= s;   y /= s;   return *this; }

    bool operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vec2& o) const { return !(*this == o); }

    float Length()   const { return sqrtf(x * x + y * y); }
    float LengthSq() const { return x * x + y * y; }

    Vec2 Normalized() const {
        float len = Length();
        if (len < 1e-6f) return { 0.f, 0.f };
        return { x / len, y / len };
    }

    bool NotBothMinusOne() const {
        return !(x == -1.f && y == -1.f);
    }

    static float Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
    static float Distance(const Vec2& a, const Vec2& b) { return (a - b).Length(); }
};

inline Vec2 operator*(float s, const Vec2& v) { return v * s; }

struct Vec3 {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;

    Vec3() = default;
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vec3(const ImVec2& v) : x(v.x), y(v.y), z(0.f) {}

    ImVec2 ToImVec2() const { return ImVec2(x, y); }

    Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator*(float s)        const { return { x * s,   y * s,   z * s }; }
    Vec3 operator/(float s)        const { return { x / s,   y / s,   z / s }; }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s;   y *= s;   z *= s;   return *this; }
    Vec3& operator/=(float s) { x /= s;   y /= s;   z /= s;   return *this; }

    bool operator==(const Vec3& o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator!=(const Vec3& o) const { return !(*this == o); }

    float Length()   const { return sqrtf(x * x + y * y + z * z); }
    float LengthSq() const { return x * x + y * y + z * z; }

    Vec3 Normalized() const {
        float len = Length();
        if (len < 1e-6f) return { 0.f, 0.f, 0.f };
        return { x / len, y / len, z / len };
    }

    bool NotBothMinusOne() const {
        return !(x == -1.f && y == -1.f && z == -1.f);
    }

    static float Dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    static Vec3 Cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
    static float Distance(const Vec3& a, const Vec3& b) { return (a - b).Length(); }
};

inline Vec3 operator*(float s, const Vec3& v) { return v * s; }

struct Quat { float x, y, z, w; };

struct Mat4x4 {
    float m[16] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };

    float  operator()(int row, int col) const { return m[col * 4 + row]; }
    float& operator()(int row, int col) { return m[col * 4 + row]; }

    static Mat4x4 Identity() { return Mat4x4{}; }

    static Mat4x4 Multiply(const Mat4x4& a, const Mat4x4& b) {
        Mat4x4 r;
        for (int c = 0; c < 4; c++) {
            for (int row = 0; row < 4; row++) {
                float s = 0.f;
                for (int k = 0; k < 4; k++)
                    s += a(k, row) * b(c, k);
                r(row, c) = s;
            }
        }
        return r;
    }

    Vec3 GetPosition() const { return { m[12], m[13], m[14] }; }

    Vec3 TransformPoint(const Vec3& v) const {
        float x = m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12];
        float y = m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13];
        float z = m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14];
        float w = m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15];
        if (fabsf(w) > 1e-6f) { x /= w; y /= w; z /= w; }
        return { x, y, z };
    }
};

inline void quat_rotate(float qx, float qy, float qz, float qw, float vx, float vy, float vz, float* ox, float* oy, float* oz)
{
    float tx = qy * vz - qz * vy + qw * vx;
    float ty = qz * vx - qx * vz + qw * vy;
    float tz = qx * vy - qy * vx + qw * vz;

    *ox = vx + 2.0f * (qy * tz - qz * ty);
    *oy = vy + 2.0f * (qz * tx - qx * tz);
    *oz = vz + 2.0f * (qx * ty - qy * tx);
}