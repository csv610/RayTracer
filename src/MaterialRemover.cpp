#include "MaterialRemover.h"
#include "MeshGeometry.h"
#include <tbb/parallel_for.h>
#include <cmath>
#include <algorithm>

MaterialRemover::MaterialRemover(const Mesh& mesh) : m_mesh(mesh) {
    m_scene.addSharedMesh(m_mesh);
    m_scene.commit();
}

Mesh MaterialRemover::simulateCnc(int res, float toolRadius) const {
    MeshGeometry geom(m_mesh);
    AABB bbox = geom.computeAABB();
    Vec3 size = bbox.size();
    
    // Target height map (Z-buffer style)
    std::vector<float> targetH(res * res, bbox.min.z);
    // Simulated height map after tool passes
    std::vector<float> simH(res * res, bbox.min.z);

    float dx = size.x / res;
    float dy = size.y / res;

    // Phase 1: Capture the target surface heights
    tbb::parallel_for(0, res, [&](int y) {
        for (int x = 0; x < res; ++x) {
            float px = bbox.min.x + (x + 0.5f) * dx;
            float py = bbox.min.y + (y + 0.5f) * dy;
            
            Ray ray;
            ray.org = {px, py, bbox.max.z + size.z * 0.1f};
            ray.dir = {0, 0, -1.0f};
            ray.tnear = 0.0f;
            ray.tfar = size.z * 1.5f;
            
            Hit hit = RayTracer::intersect(m_scene, ray);
            if (hit.hit) {
                targetH[y * res + x] = ray.org.z - hit.t;
            }
        }
    });

    // Phase 2: Dilate the heights using the tool geometry (Minkowski subtraction)
    int pR = (int)std::ceil(toolRadius / dx);
    tbb::parallel_for(0, res, [&](int y) {
        for (int x = 0; x < res; ++x) {
            float maxH = bbox.min.z;
            for (int dy_off = -pR; dy_off <= pR; ++dy_off) {
                for (int dx_off = -pR; dx_off <= pR; ++dx_off) {
                    int nx = x + dx_off;
                    int ny = y + dy_off;
                    if (nx >= 0 && nx < res && ny >= 0 && ny < res) {
                        float distSq = (dx_off * dx_off + dy_off * dy_off) * dx * dx;
                        if (distSq <= toolRadius * toolRadius) {
                            // Hemispherical tool bottom assumption
                            float toolOffset = std::sqrt(toolRadius * toolRadius - distSq) - toolRadius;
                            maxH = std::max(maxH, targetH[ny * res + nx] + toolOffset);
                        }
                    }
                }
            }
            simH[y * res + x] = maxH;
        }
    });

    // Phase 3: Construct the result mesh and color by deviation
    Mesh result;
    for (int y = 0; y < res; ++y) {
        for (int x = 0; x < res; ++x) {
            result.nodes.push_back({bbox.min.x + x * dx, bbox.min.y + y * dy, simH[y * res + x]});
            
            // L is the thickness of "uncut" material
            float L = simH[y * res + x] - targetH[y * res + x];
            // Normalize for visualization: 0 deviation = green, high deviation = red
            float deviation = std::clamp(L / (toolRadius * 0.2f), 0.0f, 1.0f);
            result.nodeColors.push_back(MeshGeometry::getJetColor(1.0f - deviation));
        }
    }

    // Grid to triangles
    for (int y = 0; y < res - 1; ++y) {
        for (int x = 0; x < res - 1; ++x) {
            unsigned int i0 = y * res + x;
            unsigned int i1 = y * res + x + 1;
            unsigned int i2 = (y + 1) * res + x + 1;
            unsigned int i3 = (y + 1) * res + x;
            result.triangles.push_back({i0, i1, i2});
            result.triangles.push_back({i0, i2, i3});
        }
    }

    return result;
}
