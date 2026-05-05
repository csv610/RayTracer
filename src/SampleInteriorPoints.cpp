#include "SampleInteriorPoints.h"
#include "MeshGeometry.h"
#include <tbb/parallel_for.h>
#include <random>
#include <mutex>
#include <iostream>

SampleInteriorPoints::SampleInteriorPoints(const Mesh& mesh) : m_mesh(mesh) {
    m_scene.addSharedMesh(m_mesh);
    m_scene.commit();
    MeshGeometry geom(m_mesh);
    m_bbox = geom.computeAABB();
}

std::vector<Vec3> SampleInteriorPoints::sample(int numSamples) const {
    std::vector<Vec3> points;
    std::mutex mtx;
    int batchSize = 10000;

    while (points.size() < (size_t)numSamples) {
        tbb::parallel_for(0, batchSize, [&](int) {
            thread_local std::mt19937 gen(std::random_device{}());
            std::uniform_real_distribution<float> disX(m_bbox.min.x, m_bbox.max.x);
            std::uniform_real_distribution<float> disY(m_bbox.min.y, m_bbox.max.y);
            std::uniform_real_distribution<float> disZ(m_bbox.min.z, m_bbox.max.z);

            Vec3 p = {disX(gen), disY(gen), disZ(gen)};
            if (m_scene.isInside(p)) {
                std::lock_guard<std::mutex> lock(mtx);
                if (points.size() < (size_t)numSamples) {
                    points.push_back(p);
                }
            }
        });
        std::cout << "\rProgress: " << points.size() << " / " << numSamples << std::flush;
    }
    std::cout << std::endl;
    return points;
}
