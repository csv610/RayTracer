#include "AmbientOcclusion.h"
#include "MeshGeometry.h"
#include <tbb/parallel_for.h>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

AmbientOcclusion::AmbientOcclusion(const Mesh& mesh) : m_mesh(mesh) {
    m_scene.addSharedMesh(m_mesh);
    m_scene.commit();
    MeshGeometry geom(m_mesh);
    m_meshDiag = geom.computeAABB().size().length();
}

std::vector<float> AmbientOcclusion::compute(int samples) const {
    std::vector<float> results(m_mesh.triangles.size());
    float epsilon = m_meshDiag * 1e-4f;
    float rayLength = m_meshDiag * 2.0f;

    tbb::parallel_for(size_t(0), m_mesh.triangles.size(), [&](size_t i) {
        Vec3 n = MeshGeometry::computeFaceNormal(m_mesh.nodes[m_mesh.triangles[i].v0], 
                                               m_mesh.nodes[m_mesh.triangles[i].v1], 
                                               m_mesh.nodes[m_mesh.triangles[i].v2]);
        Vec3 p = MeshGeometry::computeFaceCenter(m_mesh.nodes[m_mesh.triangles[i].v0], 
                                               m_mesh.nodes[m_mesh.triangles[i].v1], 
                                               m_mesh.nodes[m_mesh.triangles[i].v2]);

        Vec3 up = (std::abs(n.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        Vec3 tangent = {n.y * up.z - n.z * up.y, n.z * up.x - n.x * up.z, n.x * up.y - n.y * up.x};
        float tLen = tangent.length();
        if (tLen > 0) { tangent.x /= tLen; tangent.y /= tLen; tangent.z /= tLen; }
        Vec3 bitangent = {n.y * tangent.z - n.z * tangent.y, n.z * tangent.x - n.x * tangent.z, n.x * tangent.y - n.y * tangent.x};

        int hits = 0;
        for (int s = 0; s < samples; ++s) {
            float u = (float)s / samples;
            unsigned int bits = (s << 16) | (s >> 16);
            bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
            bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
            bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
            bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
            float v = (float)bits * 2.3283064365386963e-10;

            float phi = 2.0f * (float)M_PI * u;
            float cosTheta = v;
            float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
            
            Vec3 localDir = {std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta};
            Vec3 rayDir = {
                tangent.x * localDir.x + bitangent.x * localDir.y + n.x * localDir.z,
                tangent.y * localDir.x + bitangent.y * localDir.y + n.y * localDir.z,
                tangent.z * localDir.x + bitangent.z * localDir.y + n.z * localDir.z
            };

            Ray ray;
            ray.org = {p.x + n.x * epsilon, p.y + n.y * epsilon, p.z + n.z * epsilon};
            ray.dir = rayDir;
            ray.tnear = 0.0f;
            ray.tfar = rayLength;

            if (RayTracer::occluded(m_scene, ray)) hits++;
        }
        results[i] = (float)hits / samples;
    });
    return results;
}
