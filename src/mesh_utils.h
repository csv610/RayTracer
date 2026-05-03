#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <algorithm>

struct Vertex { float x, y, z; };
struct Triangle { unsigned int v0, v1, v2; };

struct Vec3 {
    float x, y, z;
    float length() const { return std::sqrt(x*x + y*y + z*z); }
};

struct Color4b {
    unsigned char r, g, b, a;
};

struct AABB {
    Vertex min, max;
    AABB() : min{1e10, 1e10, 1e10}, max{-1e10, -1e10, -1e10} {}
    void expand(const Vertex& v) {
        min.x = std::min(min.x, v.x); min.y = std::min(min.y, v.y); min.z = std::min(min.z, v.z);
        max.x = std::max(max.x, v.x); max.y = std::max(max.y, v.y); max.z = std::max(max.z, v.z);
    }
    Vec3 size() const { return {max.x - min.x, max.y - min.y, max.z - min.z}; }
    void pad(float p) { float dx = max.x - min.x; float dy = max.y - min.y; float dz = max.z - min.z; min.x -= dx * p; min.y -= dy * p; min.z -= dz * p; max.x += dx * p; max.y += dy * p; max.z += dz * p; }
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    std::vector<Color4b> vertexColors;
    std::vector<Color4b> faceColors;
    std::vector<Vec3> vertexNormals;
};

inline Vec3 computeFaceNormal(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    Vec3 e1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    Vec3 e2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    Vec3 n = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x};
    float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
    return n;
}

inline void computeVertexNormals(Mesh& mesh) {
    mesh.vertexNormals.assign(mesh.vertices.size(), {0, 0, 0});
    for (const auto& t : mesh.triangles) {
        Vec3 n = computeFaceNormal(mesh.vertices[t.v0], mesh.vertices[t.v1], mesh.vertices[t.v2]);
        mesh.vertexNormals[t.v0].x += n.x; mesh.vertexNormals[t.v0].y += n.y; mesh.vertexNormals[t.v0].z += n.z;
        mesh.vertexNormals[t.v1].x += n.x; mesh.vertexNormals[t.v1].y += n.y; mesh.vertexNormals[t.v1].z += n.z;
        mesh.vertexNormals[t.v2].x += n.x; mesh.vertexNormals[t.v2].y += n.y; mesh.vertexNormals[t.v2].z += n.z;
    }
    for (auto& n : mesh.vertexNormals) {
        float len = n.length();
        if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
    }
}

inline float computeFaceArea(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    Vec3 e1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    Vec3 e2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    Vec3 n = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x};
    return 0.5f * std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
}

inline Vec3 computeFaceCenter(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    return {(v0.x + v1.x + v2.x) / 3.0f,
            (v0.y + v1.y + v2.y) / 3.0f,
            (v0.z + v1.z + v2.z) / 3.0f};
}

inline Color4b getJetColor(float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    float r = std::max(0.0f, std::min(1.0f, std::min(4.0f * t - 1.5f, -4.0f * t + 4.5f)));
    float g = std::max(0.0f, std::min(1.0f, std::min(4.0f * t - 0.5f, -4.0f * t + 3.5f)));
    float b = std::max(0.0f, std::min(1.0f, std::min(4.0f * t + 0.5f, -4.0f * t + 2.5f)));
    return {(unsigned char)(r * 255.0f), (unsigned char)(g * 255.0f), (unsigned char)(b * 255.0f), 255};
}

inline void createUVSphere(Mesh& mesh, int stacks, int slices, float radius) {
    mesh.vertices.clear();
    mesh.triangles.clear();
    for (int i = 0; i <= stacks; ++i) {
        float phi = M_PI * i / stacks;
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * M_PI * j / slices;
            mesh.vertices.push_back({
                radius * std::sin(phi) * std::cos(theta),
                radius * std::sin(phi) * std::sin(theta),
                radius * std::cos(phi)
            });
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            unsigned int v0 = i * (slices + 1) + j;
            unsigned int v1 = v0 + 1;
            unsigned int v2 = (i + 1) * (slices + 1) + j;
            unsigned int v3 = v2 + 1;
            mesh.triangles.push_back({v0, v2, v1});
            mesh.triangles.push_back({v1, v2, v3});
        }
    }
}

inline void computeBoundingSphere(const Mesh& mesh, Vertex& center, float& radius) {
    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    center = {(bbox.min.x + bbox.max.x) * 0.5f, (bbox.min.y + bbox.max.y) * 0.5f, (bbox.min.z + bbox.max.z) * 0.5f};
    radius = 0;
    for (const auto& v : mesh.vertices) {
        float d = std::sqrt(std::pow(v.x - center.x, 2) + std::pow(v.y - center.y, 2) + std::pow(v.z - center.z, 2));
        radius = std::max(radius, d);
    }
}

#endif
