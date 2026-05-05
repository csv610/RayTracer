#include "MeshVoxelizer.h"
#include "MeshGeometry.h"
#include <tbb/parallel_for.h>
#include <cmath>
#include <vector>

MeshVoxelizer::MeshVoxelizer(const Mesh& mesh) : m_mesh(mesh) {
    m_scene.addSharedMesh(m_mesh);
    m_scene.commit();
}

Mesh MeshVoxelizer::generateVoxelMesh(float vSize) const {
    MeshGeometry geom(m_mesh);
    AABB bbox = geom.computeAABB();
    Vec3 size = bbox.size();
    
    int nx = (int)std::ceil(size.x / vSize) + 2;
    int ny = (int)std::ceil(size.y / vSize) + 2;
    int nz = (int)std::ceil(size.z / vSize) + 2;
    
    Vec3 minB = {bbox.min.x - vSize, bbox.min.y - vSize, bbox.min.z - vSize};
    std::vector<uint8_t> grid(nx * ny * nz, 0);

    tbb::parallel_for(0, nz, [&](int z) {
        for (int y = 0; y < ny; ++y) {
            for (int x = 0; x < nx; ++x) {
                Vec3 p = {minB.x + (x + 0.5f) * vSize, minB.y + (y + 0.5f) * vSize, minB.z + (z + 0.5f) * vSize};
                if (m_scene.isInside(p)) {
                    grid[(z * ny + y) * nx + x] = 1;
                }
            }
        }
    });

    Mesh resMesh;
    auto addCube = [&](Vec3 p, float s) {
        unsigned int start = (unsigned int)resMesh.nodes.size();
        float h = s * 0.5f;
        resMesh.nodes.push_back({p.x - h, p.y - h, p.z - h});
        resMesh.nodes.push_back({p.x + h, p.y - h, p.z - h});
        resMesh.nodes.push_back({p.x + h, p.y + h, p.z - h});
        resMesh.nodes.push_back({p.x - h, p.y + h, p.z - h});
        resMesh.nodes.push_back({p.x - h, p.y - h, p.z + h});
        resMesh.nodes.push_back({p.x + h, p.y - h, p.z + h});
        resMesh.nodes.push_back({p.x + h, p.y + h, p.z + h});
        resMesh.nodes.push_back({p.x - h, p.y + h, p.z + h});
        
        resMesh.triangles.push_back({start + 0, start + 2, start + 1});
        resMesh.triangles.push_back({start + 0, start + 3, start + 2});
        resMesh.triangles.push_back({start + 4, start + 5, start + 6});
        resMesh.triangles.push_back({start + 4, start + 6, start + 7});
        resMesh.triangles.push_back({start + 0, start + 1, start + 5});
        resMesh.triangles.push_back({start + 0, start + 5, start + 4});
        resMesh.triangles.push_back({start + 1, start + 2, start + 6});
        resMesh.triangles.push_back({start + 1, start + 6, start + 5});
        resMesh.triangles.push_back({start + 2, start + 3, start + 7});
        resMesh.triangles.push_back({start + 2, start + 7, start + 6});
        resMesh.triangles.push_back({start + 3, start + 0, start + 4});
        resMesh.triangles.push_back({start + 3, start + 4, start + 7});
    };

    for (int z = 0; z < nz; ++z) {
        for (int y = 0; y < ny; ++y) {
            for (int x = 0; x < nx; ++x) {
                if (grid[(z * ny + y) * nx + x]) {
                    addCube({minB.x + (x + 0.5f) * vSize, minB.y + (y + 0.5f) * vSize, minB.z + (z + 0.5f) * vSize}, vSize * 0.95f);
                }
            }
        }
    }
    return resMesh;
}
