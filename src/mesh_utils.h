#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include <algorithm>

struct Vertex { float x, y, z; };
struct Triangle { unsigned int v0, v1, v2; };
struct Vec3 { float x, y, z; };

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
};

struct AABB {
    Vec3 min = {1e20f, 1e20f, 1e20f};
    Vec3 max = {-1e20f, -1e20f, -1e20f};
    void expand(const Vertex& v) {
        min.x = std::min(min.x, v.x); min.y = std::min(min.y, v.y); min.z = std::min(min.z, v.z);
        max.x = std::max(max.x, v.x); max.y = std::max(max.y, v.y); max.z = std::max(max.z, v.z);
    }
    void pad(float f) {
        Vec3 s = size();
        min.x -= s.x * f; min.y -= s.y * f; min.z -= s.z * f;
        max.x += s.x * f; max.y += s.y * f; max.z += s.z * f;
    }
    Vec3 size() const { return {max.x - min.x, max.y - min.y, max.z - min.z}; }
    Vec3 center() const { return {(min.x + max.x)*0.5f, (min.y + max.y)*0.5f, (min.z + max.z)*0.5f}; }
};

inline bool readOFF(const std::string& filename, Mesh& mesh) {
    // Deprecated: use MeshIO::load
    return false;
}


inline Vec3 computeFaceNormal(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    Vec3 e1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    Vec3 e2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    Vec3 n = {e1.y * e2.z - e1.z * e2.y,
                e1.z * e2.x - e1.x * e2.z,
                e1.x * e2.y - e1.y * e2.x};
    float len = sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
    if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
    return n;
}

inline Vec3 computeFaceCenter(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    return {(v0.x + v1.x + v2.x) / 3.0f,
            (v0.y + v1.y + v2.y) / 3.0f,
            (v0.z + v1.z + v2.z) / 3.0f};
}

inline void computeBoundingSphere(const Mesh& mesh, Vertex& center, float& radius) {
    if (mesh.vertices.empty()) {
        center = {0, 0, 0};
        radius = 0;
        return;
    }
    center = {0, 0, 0};
    for (const auto& v : mesh.vertices) {
        center.x += v.x;
        center.y += v.y;
        center.z += v.z;
    }
    center.x /= mesh.vertices.size();
    center.y /= mesh.vertices.size();
    center.z /= mesh.vertices.size();

    radius = 0;
    for (const auto& v : mesh.vertices) {
        float dx = v.x - center.x;
        float dy = v.y - center.y;
        float dz = v.z - center.z;
        float dist = sqrt(dx*dx + dy*dy + dz*dz);
        if (dist > radius) radius = dist;
    }
}

inline Vec3 getJetColor(float v) {
    v = std::max(0.0f, std::min(1.0f, v));
    float r = std::max(0.0f, std::min(1.0f, 1.5f - std::abs(4.0f * v - 3.0f)));
    float g = std::max(0.0f, std::min(1.0f, 1.5f - std::abs(4.0f * v - 2.0f)));
    float b = std::max(0.0f, std::min(1.0f, 1.5f - std::abs(4.0f * v - 1.0f)));
    return {r, g, b};
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

inline void createUVSphere(Mesh& mesh, int stacks, int slices, float radius) {
    mesh.vertices.clear();
    mesh.triangles.clear();
    for (int i = 0; i <= stacks; ++i) {
        float phi = M_PI * i / stacks;
        for (int j = 0; j <= slices; ++j) {
            float theta = 2 * M_PI * j / slices;
            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);
            mesh.vertices.push_back({x, y, z});
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            unsigned int first = (i * (slices + 1)) + j;
            unsigned int second = first + slices + 1;
            mesh.triangles.push_back({first, second, first + 1});
            mesh.triangles.push_back({second, second + 1, first + 1});
        }
    }
}

inline void writePPM(const std::string& filename, int width, int height, const std::vector<Vec3>& buffer) {
    std::ofstream file(filename, std::ios::binary);
    file << "P6\n" << width << " " << height << "\n255\n";
    for (const auto& c : buffer) {
        unsigned char r = (unsigned char)(std::max(0.0f, std::min(1.0f, c.x)) * 255);
        unsigned char g = (unsigned char)(std::max(0.0f, std::min(1.0f, c.y)) * 255);
        unsigned char b = (unsigned char)(std::max(0.0f, std::min(1.0f, c.z)) * 255);
        file << r << g << b;
    }
    file.close();
}

#endif // MESH_UTILS_H
