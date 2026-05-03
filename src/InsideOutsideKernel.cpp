#include "InsideOutsideKernel.h"
#include <tbb/parallel_for.h>
#include <algorithm>
#include <cmath>

InsideOutsideKernel::InsideOutsideKernel(const Mesh& mesh) : m_mesh(mesh) {
    m_scene.addSharedMesh(m_mesh);
    m_scene.commit();
    for (const auto& v : m_mesh.vertices) m_bbox.expand(v);
    
    float maxDim = m_bbox.size().length();
    // Senior Engineer standard: adaptive epsilon for 32-bit float precision
    m_epsilon = std::max(1e-5f, maxDim * 1e-6f);
}

int InsideOutsideKernel::classify(const Vec3& p) const {
    // 1. Fast-path: strictly outside BBox
    if (p.x < m_bbox.min.x - m_epsilon || p.x > m_bbox.max.x + m_epsilon ||
        p.y < m_bbox.min.y - m_epsilon || p.y > m_bbox.max.y + m_epsilon ||
        p.z < m_bbox.min.z - m_epsilon || p.z > m_bbox.max.z + m_epsilon) {
        return 1;
    }

    // 2. High-Reliability Surface Detection
    // Use 7 rays (center + 6 axial displacements) to probe the immediate vicinity.
    // If ANY ray from p - eps hit within 2*eps, we are on the surface.
    Vec3 probeDirs[3] = {{1,0,0}, {0,1,0}, {0,0,1}};
    for (int i = 0; i < 3; ++i) {
        for (float side : {-1.0f, 1.0f}) {
            Ray ray;
            Vec3 dir = { probeDirs[i].x * side, probeDirs[i].y * side, probeDirs[i].z * side };
            ray.org = p;
            ray.dir = dir;
            ray.tnear = 0.0f;
            ray.tfar = m_epsilon; 
            if (RayTracer::intersect(m_scene, ray).hit) return 0;
            
            // Back-check: shoot through p
            ray.org = { p.x - dir.x * m_epsilon, p.y - dir.y * m_epsilon, p.z - dir.z * m_epsilon };
            ray.tfar = m_epsilon * 2.0f;
            Hit h = RayTracer::intersect(m_scene, ray);
            if (h.hit && std::abs(h.t - m_epsilon) < m_epsilon * 1.5f) return 0;
        }
    }

    // 3. Foolproof Parity Counting
    // We use 3 directions based on irrational numbers (square roots) 
    // to avoid hitting edges/vertices exactly.
    auto checkDirection = [&](Vec3 dir) {
        float dLen = std::sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
        dir.x /= dLen; dir.y /= dLen; dir.z /= dLen;

        float maxDim = m_bbox.size().length();
        float t_start = maxDim * 4.0f;
        Vec3 org = { p.x - dir.x * t_start, p.y - dir.y * t_start, p.z - dir.z * t_start };

        // Parity of hits before reaching p
        int hits = m_scene.countIntersections(org, dir, t_start - m_epsilon);
        return (hits % 2 != 0);
    };

    int insideVotes = 0;
    // Directions derived from sqrt(2), sqrt(3), sqrt(5) to be "random" yet stable
    if (checkDirection({0.414213f, 0.732050f, 0.236067f})) insideVotes++;
    if (checkDirection({-0.236067f, 0.414213f, 0.732050f})) insideVotes++;
    if (checkDirection({0.732050f, -0.236067f, 0.414213f})) insideVotes++;

    return (insideVotes >= 2) ? -1 : 1;
}

std::vector<int> InsideOutsideKernel::classify(const std::vector<Vec3>& points) const {
    std::vector<int> results(points.size());
    tbb::parallel_for(tbb::blocked_range<size_t>(0, points.size()), [&](const tbb::blocked_range<size_t>& r) {
        for (size_t i = r.begin(); i != r.end(); ++i) {
            results[i] = classify(points[i]);
        }
    });
    return results;
}
