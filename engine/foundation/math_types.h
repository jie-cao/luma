// Math Types for LUMA Engine
// Core math structures: Vec3, Quat, Mat4
#pragma once

#include <cmath>
#include <cstdint>

namespace luma {

// Maximum bones for skeletal animation (must match shader)
constexpr uint32_t MAX_BONES = 128;

// ===== Vec3 =====
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    
    Vec3 operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vec3 operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalized() const {
        float len = length();
        if (len < 0.0001f) return {0, 0, 0};
        return {x/len, y/len, z/len};
    }
    
    float dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
    
    Vec3 cross(const Vec3& other) const {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }
    
    static Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
        return a + (b - a) * t;
    }
};

// ===== Quat =====
struct Quat {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
    
    Quat() = default;
    Quat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
    
    // Create from Euler angles (radians)
    static Quat fromEuler(float pitch, float yaw, float roll) {
        float cy = std::cos(yaw * 0.5f);
        float sy = std::sin(yaw * 0.5f);
        float cp = std::cos(pitch * 0.5f);
        float sp = std::sin(pitch * 0.5f);
        float cr = std::cos(roll * 0.5f);
        float sr = std::sin(roll * 0.5f);
        
        Quat q;
        q.w = cr * cp * cy + sr * sp * sy;
        q.x = sr * cp * cy - cr * sp * sy;
        q.y = cr * sp * cy + sr * cp * sy;
        q.z = cr * cp * sy - sr * sp * cy;
        return q;
    }
    
    // Create from axis-angle (radians)
    static Quat fromAxisAngle(const Vec3& axis, float angle) {
        float halfAngle = angle * 0.5f;
        float s = std::sin(halfAngle);
        return Quat(axis.x * s, axis.y * s, axis.z * s, std::cos(halfAngle));
    }
    
    // Get Euler angles (radians)
    Vec3 toEuler() const {
        Vec3 euler;
        // Roll (x-axis rotation)
        float sinr_cosp = 2.0f * (w * x + y * z);
        float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
        euler.x = std::atan2(sinr_cosp, cosr_cosp);
        
        // Pitch (y-axis rotation)
        float sinp = 2.0f * (w * y - z * x);
        if (std::abs(sinp) >= 1.0f)
            euler.y = std::copysign(3.14159f / 2.0f, sinp);
        else
            euler.y = std::asin(sinp);
        
        // Yaw (z-axis rotation)
        float siny_cosp = 2.0f * (w * z + x * y);
        float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
        euler.z = std::atan2(siny_cosp, cosy_cosp);
        
        return euler;
    }
    
    Quat operator*(const Quat& other) const {
        return {
            w * other.x + x * other.w + y * other.z - z * other.y,
            w * other.y - x * other.z + y * other.w + z * other.x,
            w * other.z + x * other.y - y * other.x + z * other.w,
            w * other.w - x * other.x - y * other.y - z * other.z
        };
    }
    
    Vec3 rotate(const Vec3& v) const {
        Vec3 u{x, y, z};
        float s = w;
        float dot_uv = u.x*v.x + u.y*v.y + u.z*v.z;
        float dot_uu = u.x*u.x + u.y*u.y + u.z*u.z;
        Vec3 cross{u.y*v.z - u.z*v.y, u.z*v.x - u.x*v.z, u.x*v.y - u.y*v.x};
        return v * (s*s - dot_uu) + u * (2.0f * dot_uv) + cross * (2.0f * s);
    }
    
    Quat normalized() const {
        float len = std::sqrt(x*x + y*y + z*z + w*w);
        if (len < 0.0001f) return Quat();
        return Quat(x/len, y/len, z/len, w/len);
    }
    
    static Quat slerp(const Quat& a, const Quat& b, float t) {
        float dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
        Quat b2 = dot < 0 ? Quat(-b.x, -b.y, -b.z, -b.w) : b;
        dot = std::abs(dot);
        if (dot > 0.9995f) {
            return Quat(a.x + t*(b2.x - a.x), a.y + t*(b2.y - a.y),
                       a.z + t*(b2.z - a.z), a.w + t*(b2.w - a.w)).normalized();
        }
        float theta = std::acos(dot);
        float sinTheta = std::sin(theta);
        float wa = std::sin((1-t)*theta) / sinTheta;
        float wb = std::sin(t*theta) / sinTheta;
        return Quat(wa*a.x + wb*b2.x, wa*a.y + wb*b2.y, wa*a.z + wb*b2.z, wa*a.w + wb*b2.w);
    }
};

// ===== Mat4 (column-major for GPU) =====
struct Mat4 {
    float m[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    
    Mat4() = default;
    
    float& operator()(int row, int col) { return m[col * 4 + row]; }
    float operator()(int row, int col) const { return m[col * 4 + row]; }
    
    static Mat4 identity() {
        Mat4 result;
        return result;
    }
    
    static Mat4 translation(const Vec3& t) {
        Mat4 result;
        result.m[12] = t.x;
        result.m[13] = t.y;
        result.m[14] = t.z;
        return result;
    }
    
    static Mat4 scale(const Vec3& s) {
        Mat4 result;
        result.m[0] = s.x;
        result.m[5] = s.y;
        result.m[10] = s.z;
        return result;
    }
    
    static Mat4 fromQuat(const Quat& q) {
        Mat4 result;
        float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
        float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
        
        result.m[0] = 1.0f - 2.0f * (yy + zz);
        result.m[1] = 2.0f * (xy + wz);
        result.m[2] = 2.0f * (xz - wy);
        result.m[4] = 2.0f * (xy - wz);
        result.m[5] = 1.0f - 2.0f * (xx + zz);
        result.m[6] = 2.0f * (yz + wx);
        result.m[8] = 2.0f * (xz + wy);
        result.m[9] = 2.0f * (yz - wx);
        result.m[10] = 1.0f - 2.0f * (xx + yy);
        return result;
    }
    
    Mat4 operator*(const Mat4& other) const {
        Mat4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i*4+j] = 
                    m[0*4+j] * other.m[i*4+0] +
                    m[1*4+j] * other.m[i*4+1] +
                    m[2*4+j] * other.m[i*4+2] +
                    m[3*4+j] * other.m[i*4+3];
            }
        }
        return result;
    }
    
    Vec3 transformPoint(const Vec3& p) const {
        float w = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
        if (std::abs(w) < 0.0001f) w = 1.0f;
        return {
            (m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12]) / w,
            (m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13]) / w,
            (m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]) / w
        };
    }
};

}  // namespace luma
