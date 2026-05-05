#include "StructuralCaliper.h"
#include <tbb/parallel_for.h>
#include <iostream>
#include <algorithm>

StructuralCaliper::StructuralCaliper(const Mesh& mesh) : m_mesh(mesh) {
    m_scene.addSharedMesh(m_mesh);
    m_scene.commit();
}

std::vector<StructuralCaliper::AnalysisResult> StructuralCaliper::analyze(int numSamples, float threshold) const {
    SampleSurfacePoints sampler(m_mesh);
    std::vector<SampledPoint> surfacePoints = sampler.sample(numSamples, 0);

    std::vector<AnalysisResult> results(surfacePoints.size());
    std::cout << "Running structural analysis on " << surfacePoints.size() << " samples..." << std::endl;

    tbb::parallel_for(size_t(0), surfacePoints.size(), [&](size_t i) {
        const auto& sp = surfacePoints[i];
        
        Ray ray;
        float epsilon = 1e-4f;
        ray.org = {sp.p.x - sp.n.x * epsilon, sp.p.y - sp.n.y * epsilon, sp.p.z - sp.n.z * epsilon};
        ray.dir = {-sp.n.x, -sp.n.y, -sp.n.z};
        ray.tnear = 0.0f;
        ray.tfar = 1e10f;

        Hit hit = RayTracer::intersect(m_scene, ray);

        float thickness = hit.hit ? hit.t : 1e10f;
        
        Color4b color;
        if (thickness < threshold) {
            float t = std::clamp(thickness / threshold, 0.0f, 1.0f);
            color = {(unsigned char)(255.0f), (unsigned char)(t * 255.0f), 0, 255}; // Red (0) to Yellow (threshold)
        } else {
            color = {0, 255, 0, 255}; // Green (Safe)
        }
        
        results[i] = {sp.p, thickness, color};
    });

    return results;
}
