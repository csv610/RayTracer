#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <algorithm>

/**
 * @file Mesh.h
 * @brief Core geometric structures for the mesh processing engine.
 * 
 * This header defines the foundational types used throughout the project.
 * It is strictly a data-definition header; all geometric logic and 
 * calculations must reside in MeshGeometry or MeshTopology.
 */

struct Node { float x, y, z; };
struct Triangle { unsigned int v0, v1, v2; };

struct Vec3 {
    float x, y, z;
    float length() const { return std::sqrt(x*x + y*y + z*z); }
};

struct Color4b {
    unsigned char r, g, b, a;
};

/**
 * @struct Quaternion
 * @brief Represents a 3D rotation using four-dimensional complex numbers.
 */
struct Quaternion {
    float x, y, z, w;

    static Quaternion identity() { return {0, 0, 0, 1}; }

    static Quaternion fromAxisAngle(Vec3 axis, float angle) {
        float l = axis.length();
        if (l > 0) { axis.x /= l; axis.y /= l; axis.z /= l; }
        float s = std::sin(angle * 0.5f);
        return {axis.x * s, axis.y * s, axis.z * s, std::cos(angle * 0.5f)};
    }

    static Quaternion fromTo(Vec3 a, Vec3 b) {
        float al = a.length(); if (al > 0) { a.x /= al; a.y /= al; a.z /= al; }
        float bl = b.length(); if (bl > 0) { b.x /= bl; b.y /= bl; b.z /= bl; }
        float dot = a.x*b.x + a.y*b.y + a.z*b.z;
        if (dot > 0.999999f) return identity();
        if (dot < -0.999999f) {
            Vec3 axis = {0, 1, 0};
            if (std::abs(a.x) < 0.9f) axis = {1, 0, 0};
            Vec3 cross = {a.y * axis.z - a.z * axis.y, a.z * axis.x - a.x * axis.z, a.x * axis.y - a.y * axis.x};
            return fromAxisAngle(cross, M_PI);
        }
        Vec3 cross = {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
        float s = std::sqrt((1 + dot) * 2);
        return {cross.x / s, cross.y / s, cross.z / s, s * 0.5f};
    }

    static Quaternion random(float u1, float u2, float u3) {
        float s1 = std::sqrt(1.0f - u1);
        float s2 = std::sqrt(u1);
        return {
            static_cast<float>(s1 * std::sin(2.0f * M_PI * u2)),
            static_cast<float>(s1 * std::cos(2.0f * M_PI * u2)),
            static_cast<float>(s2 * std::sin(2.0f * M_PI * u3)),
            static_cast<float>(s2 * std::cos(2.0f * M_PI * u3))
        };
    }

    Vec3 rotate(Vec3 v) const {
        Vec3 qv = {x, y, z};
        Vec3 t = {2 * (qv.y * v.z - qv.z * v.y), 2 * (qv.z * v.x - qv.x * v.z), 2 * (qv.x * v.y - qv.y * v.x)};
        return {
            v.x + w * t.x + (qv.y * t.z - qv.z * t.y),
            v.y + w * t.y + (qv.z * t.x - qv.x * t.z),
            v.z + w * t.z + (qv.x * t.y - qv.y * t.x)
        };
    }

    Node rotate(Node v) const {
        Vec3 res = rotate(Vec3{v.x, v.y, v.z});
        return {res.x, res.y, res.z};
    }
};

struct AABB {
    Node min, max;
    AABB() : min{1e10, 1e10, 1e10}, max{-1e10, -1e10, -1e10} {}
    void expand(const Node& v) {
        min.x = std::min(min.x, v.x); min.y = std::min(min.y, v.y); min.z = std::min(min.z, v.z);
        max.x = std::max(max.x, v.x); max.y = std::max(max.y, v.y); max.z = std::max(max.z, v.z);
    }
    Vec3 size() const { return {max.x - min.x, max.y - min.y, max.z - min.z}; }
    void pad(float p) { float dx = max.x - min.x; float dy = max.y - min.y; float dz = max.z - min.z; min.x -= dx * p; min.y -= dy * p; min.z -= dz * p; max.x += dx * p; max.y += dy * p; max.z += dz * p; }
};

struct Mesh {
    std::vector<Node> nodes;
    std::vector<Triangle> triangles;
    std::vector<Color4b> nodeColors;
    std::vector<Color4b> faceColors;
    std::vector<Vec3> nodeNormals;
    std::vector<Vec3> faceNormals;
};

#endif // MESH_H
