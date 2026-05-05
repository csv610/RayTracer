#include "SampleSurfacePoints.h"
#include "MeshGeometry.h"
#include <tbb/parallel_for.h>
#include <random>
#include <algorithm>
#include <iostream>

SampleSurfacePoints::SampleSurfacePoints(const Mesh& mesh) : m_mesh(mesh), m_totalArea(0.0f) {
    m_scene.addSharedMesh(m_mesh);
    m_scene.commit();
    
    MeshGeometry geom(m_mesh);
    m_epsilon = geom.computeAABB().size().length() * 1e-4f;
    if (m_epsilon < 1e-6f) m_epsilon = 1e-6f;

    buildCDF();
    precomputeOrientations();
}

void SampleSurfacePoints::buildCDF() {
    MeshGeometry geom(m_mesh);
    std::vector<float> areas = geom.computeAllFaceAreas();
    m_triangleCDF.resize(areas.size());
    m_totalArea = 0.0f;
    for (size_t i = 0; i < areas.size(); ++i) {
        m_totalArea += areas[i];
        m_triangleCDF[i] = m_totalArea;
    }
}

void SampleSurfacePoints::precomputeOrientations() {
    std::cout << "Precomputing face orientations..." << std::endl;
    m_isNormalOutward.resize(m_mesh.triangles.size());
    MeshGeometry geom(m_mesh);
    std::vector<Vec3> faceNormals = geom.computeAllFaceNormals();
    std::vector<Vec3> faceCenters = geom.computeAllFaceCenters();

    tbb::parallel_for(size_t(0), m_mesh.triangles.size(), [&](size_t i) {
        const Vec3& center = faceCenters[i];
        const Vec3& normal = faceNormals[i];
        
        Vec3 p_test = { center.x + normal.x * m_epsilon, 
                        center.y + normal.y * m_epsilon, 
                        center.z + normal.z * m_epsilon };
        
        m_isNormalOutward[i] = m_scene.isInside(p_test) ? 0 : 1;
    });
}

std::vector<SampledPoint> SampleSurfacePoints::sample(int numSamples, int side) const {
    size_t numTris = m_mesh.triangles.size();
    size_t totalToSample = std::max((size_t)numSamples, numTris);
    std::vector<SampledPoint> points(totalToSample);

    std::cout << "Generating " << totalToSample << " surface samples..." << std::endl;
    tbb::parallel_for(size_t(0), totalToSample, [&](size_t i) {
        thread_local std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dis(0.0f, 1.0f);

        size_t triIdx = (i < numTris) ? i : (std::distance(m_triangleCDF.begin(), std::lower_bound(m_triangleCDF.begin(), m_triangleCDF.end(), dis(gen) * m_totalArea)));
        if (triIdx >= numTris) triIdx = numTris - 1;

        const Triangle& t = m_mesh.triangles[triIdx];
        const Node& v0 = m_mesh.nodes[t.v0]; const Node& v1 = m_mesh.nodes[t.v1]; const Node& v2 = m_mesh.nodes[t.v2];

        float r1 = std::sqrt(dis(gen)), r2 = dis(gen);
        float u = 1.0f - r1, v = r2 * r1, w = 1.0f - u - v;
        
        Vec3 p = { u*v0.x + v*v1.x + w*v2.x, u*v0.y + v*v1.y + w*v2.y, u*v0.z + v*v1.z + w*v2.z };
        Vec3 normal = MeshGeometry::computeFaceNormal(v0, v1, v2);

        // Ensure normal points outward
        if (!m_isNormalOutward[triIdx]) {
            normal.x = -normal.x; normal.y = -normal.y; normal.z = -normal.z;
        }

        if (side != 0) {
            float s = (float)side;
            p.x += normal.x * s * m_epsilon;
            p.y += normal.y * s * m_epsilon;
            p.z += normal.z * s * normal.z;
        }
        
        points[i] = {p, normal};
    });
    return points;
}
