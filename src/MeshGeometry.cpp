#include "MeshGeometry.h"
#include <tbb/parallel_for.h>
#include <cmath>
#include <algorithm>

MeshGeometry::MeshGeometry(const Mesh& mesh) : m_mesh(mesh) {}

Vec3 MeshGeometry::computeFaceNormal(const Node& v0, const Node& v1, const Node& v2) {
    Vec3 e1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    Vec3 e2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    Vec3 n = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x};
    float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
    return n;
}

float MeshGeometry::computeFaceArea(const Node& v0, const Node& v1, const Node& v2) {
    Vec3 e1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    Vec3 e2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    Vec3 n = {e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x};
    return 0.5f * std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
}

Vec3 MeshGeometry::computeFaceCenter(const Node& v0, const Node& v1, const Node& v2) {
    return {(v0.x + v1.x + v2.x) / 3.0f,
            (v0.y + v1.y + v2.y) / 3.0f,
            (v0.z + v1.z + v2.z) / 3.0f};
}

Vec3 MeshGeometry::computeFaceNormal(unsigned int triIdx) const {
    const Triangle& tri = m_mesh.triangles[triIdx];
    return computeFaceNormal(m_mesh.nodes[tri.v0], m_mesh.nodes[tri.v1], m_mesh.nodes[tri.v2]);
}

std::vector<Vec3> MeshGeometry::computeAllFaceNormals() const {
    std::vector<Vec3> normals(m_mesh.triangles.size());
    tbb::parallel_for(size_t(0), m_mesh.triangles.size(), [&](size_t i) {
        normals[i] = computeFaceNormal((unsigned int)i);
    });
    return normals;
}

std::vector<Vec3> MeshGeometry::computeNodeNormals() const {
    std::vector<Vec3> normals(m_mesh.nodes.size(), {0, 0, 0});
    std::vector<Vec3> faceNormals = computeAllFaceNormals();

    for (size_t t = 0; t < m_mesh.triangles.size(); ++t) {
        const auto& tri = m_mesh.triangles[t];
        const Vec3& n = faceNormals[t];
        normals[tri.v0].x += n.x; normals[tri.v0].y += n.y; normals[tri.v0].z += n.z;
        normals[tri.v1].x += n.x; normals[tri.v1].y += n.y; normals[tri.v1].z += n.z;
        normals[tri.v2].x += n.x; normals[tri.v2].y += n.y; normals[tri.v2].z += n.z;
    }
    for (auto& n : normals) {
        float len = n.length();
        if (len > 0) { n.x /= len; n.y /= len; n.z /= len; }
    }
    return normals;
}

Vec3 MeshGeometry::computeFaceCenter(unsigned int triIdx) const {
    const Triangle& tri = m_mesh.triangles[triIdx];
    return computeFaceCenter(m_mesh.nodes[tri.v0], m_mesh.nodes[tri.v1], m_mesh.nodes[tri.v2]);
}

std::vector<Vec3> MeshGeometry::computeAllFaceCenters() const {
    std::vector<Vec3> centers(m_mesh.triangles.size());
    tbb::parallel_for(size_t(0), m_mesh.triangles.size(), [&](size_t i) {
        centers[i] = computeFaceCenter((unsigned int)i);
    });
    return centers;
}

float MeshGeometry::computeFaceArea(unsigned int triIdx) const {
    const Triangle& tri = m_mesh.triangles[triIdx];
    return computeFaceArea(m_mesh.nodes[tri.v0], m_mesh.nodes[tri.v1], m_mesh.nodes[tri.v2]);
}

std::vector<float> MeshGeometry::computeAllFaceAreas() const {
    std::vector<float> areas(m_mesh.triangles.size());
    tbb::parallel_for(size_t(0), m_mesh.triangles.size(), [&](size_t i) {
        areas[i] = computeFaceArea((unsigned int)i);
    });
    return areas;
}

std::vector<float> MeshGeometry::computeNodeAreas() const {
    std::vector<float> vAreas(m_mesh.nodes.size(), 0.0f);
    std::vector<float> fAreas = computeAllFaceAreas();
    for (size_t t = 0; t < m_mesh.triangles.size(); ++t) {
        const auto& tri = m_mesh.triangles[t];
        float thirdArea = fAreas[t] / 3.0f;
        vAreas[tri.v0] += thirdArea; vAreas[tri.v1] += thirdArea; vAreas[tri.v2] += thirdArea;
    }
    return vAreas;
}

AABB MeshGeometry::computeAABB() const {
    AABB bbox;
    for (const auto& v : m_mesh.nodes) bbox.expand(v);
    return bbox;
}

void MeshGeometry::computeBoundingSphere(Node& center, float& radius) const {
    AABB bbox = computeAABB();
    center = {(bbox.min.x + bbox.max.x) * 0.5f, (bbox.min.y + bbox.max.y) * 0.5f, (bbox.min.z + bbox.max.z) * 0.5f};
    radius = 0;
    for (const auto& v : m_mesh.nodes) {
        float d = std::sqrt(std::pow(v.x - center.x, 2) + std::pow(v.y - center.y, 2) + std::pow(v.z - center.z, 2));
        radius = std::max(radius, d);
    }
}

void MeshGeometry::createUVSphere(Mesh& mesh, int stacks, int slices, float radius) {
    mesh.nodes.clear();
    mesh.triangles.clear();
    for (int i = 0; i <= stacks; ++i) {
        float phi = (float)M_PI * i / stacks;
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * (float)M_PI * j / slices;
            mesh.nodes.push_back({
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

Color4b MeshGeometry::getJetColor(float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    float r = std::max(0.0f, std::min(1.0f, std::min(4.0f * t - 1.5f, -4.0f * t + 4.5f)));
    float g = std::max(0.0f, std::min(1.0f, std::min(4.0f * t - 0.5f, -4.0f * t + 3.5f)));
    float b = std::max(0.0f, std::min(1.0f, std::min(4.0f * t + 0.5f, -4.0f * t + 2.5f)));
    return {(unsigned char)(std::round(r * 255.0f)), (unsigned char)(std::round(g * 255.0f)), (unsigned char)(std::round(b * 255.0f)), 255};
}

MeshGeometry::Curvature MeshGeometry::computeCurvature() const {
    MeshTopology topo(m_mesh);
    size_t nv = m_mesh.nodes.size();
    Curvature res;
    res.gaussian.resize(nv, 0.0f);
    res.mean.resize(nv, 0.0f);
    
    std::vector<Vec3> vNormals = computeNodeNormals();
    const auto& nodeNeighbors = topo.getNodeNeighbors();
    const auto& nodeTriangles = topo.getNodeTriangles();

    tbb::parallel_for(size_t(0), nv, [&](size_t vidx) {
        const auto& neighbors = nodeNeighbors[vidx];
        const auto& triangles = nodeTriangles[vidx];
        if (neighbors.size() < 2) return;

        Vec3 p0 = {m_mesh.nodes[vidx].x, m_mesh.nodes[vidx].y, m_mesh.nodes[vidx].z};
        float angleSum = 0.0f;
        float areaSum = 0.0f;
        for (unsigned int tidx : triangles) {
            const auto& tri = m_mesh.triangles[tidx];
            unsigned int v1, v2;
            if (tri.v0 == vidx) { v1 = tri.v1; v2 = tri.v2; }
            else if (tri.v1 == vidx) { v1 = tri.v0; v2 = tri.v2; }
            else { v1 = tri.v0; v2 = tri.v1; }

            Vec3 p1 = {m_mesh.nodes[v1].x, m_mesh.nodes[v1].y, m_mesh.nodes[v1].z};
            Vec3 p2 = {m_mesh.nodes[v2].x, m_mesh.nodes[v2].y, m_mesh.nodes[v2].z};
            Vec3 e1 = {p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
            Vec3 e2 = {p2.x - p0.x, p2.y - p0.y, p2.z - p0.z};
            float l1 = e1.length(), l2 = e2.length();
            if (l1 > 1e-8f && l2 > 1e-8f) {
                float dot = (e1.x*e2.x + e1.y*e2.y + e1.z*e2.z) / (l1 * l2);
                angleSum += acos(std::max(-1.0f, std::min(1.0f, dot)));
            }
            areaSum += computeFaceArea(tidx);
        }
        if (areaSum > 1e-8f) res.gaussian[vidx] = (2.0f * (float)M_PI - angleSum) / (areaSum / 3.0f);

        Vec3 n0 = vNormals[vidx];
        float hSum = 0.0f;
        for (unsigned int nidx : neighbors) {
            Vec3 ni = vNormals[nidx];
            hSum += (1.0f - (n0.x*ni.x + n0.y*ni.y + n0.z*ni.z));
        }
        res.mean[vidx] = hSum / neighbors.size();
    });

    return res;
}
