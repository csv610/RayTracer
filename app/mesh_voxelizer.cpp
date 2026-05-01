#include <embree4/rtcore.h>
#include <tbb/parallel_for.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <fstream>
#include "mesh_utils.h"
#include "MeshIO.h"

struct VoxelGrid {
    int nx, ny, nz;
    Vec3 minBound;
    float voxelSize;
    std::vector<uint8_t> data;

    VoxelGrid(int res, const AABB& bbox) {
        Vec3 size = bbox.size();
        float maxDim = std::max({size.x, size.y, size.z});
        voxelSize = maxDim / res;
        nx = (int)ceil(size.x / voxelSize) + 2;
        ny = (int)ceil(size.y / voxelSize) + 2;
        nz = (int)ceil(size.z / voxelSize) + 2;
        minBound = {bbox.min.x - voxelSize, bbox.min.y - voxelSize, bbox.min.z - voxelSize};
        data.resize(nx * ny * nz, 0);
    }

    void set(int x, int y, int z) { data[(z * ny + y) * nx + x] = 1; }
    bool get(int x, int y, int z) const { return data[(z * ny + y) * nx + x] != 0; }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [resolution] [output_voxels.off]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    int resolution = (argc >= 3) ? std::stoi(argv[2]) : 64;
    std::string outputFile = (argc >= 4) ? argv[3] : "voxels.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    VoxelGrid grid(resolution, bbox);

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene scene = rtcNewScene(device);
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
    memcpy(vb, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
    memcpy(ib, mesh.triangles.data(), mesh.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
    rtcCommitScene(scene);

    std::cout << "Voxelizing mesh to grid of size " << grid.nx << "x" << grid.ny << "x" << grid.nz << "..." << std::endl;

    tbb::parallel_for(0, grid.nz, [&](int z) {
        for (int y = 0; y < grid.ny; ++y) {
            for (int x = 0; x < grid.nx; ++x) {
                Vec3 p = {
                    grid.minBound.x + (x + 0.5f) * grid.voxelSize,
                    grid.minBound.y + (y + 0.5f) * grid.voxelSize,
                    grid.minBound.z + (z + 0.5f) * grid.voxelSize
                };

                // Parity test to check if point is inside
                RTCRayHit rh;
                rh.ray.org_x = p.x; rh.ray.org_y = p.y; rh.ray.org_z = p.z;
                rh.ray.dir_x = 0.314f; rh.ray.dir_y = 0.718f; rh.ray.dir_z = 0.941f;
                float len = sqrt(rh.ray.dir_x*rh.ray.dir_x + rh.ray.dir_y*rh.ray.dir_y + rh.ray.dir_z*rh.ray.dir_z);
                rh.ray.dir_x /= len; rh.ray.dir_y /= len; rh.ray.dir_z /= len;
                rh.ray.tnear = 0.0f; rh.ray.tfar = 1e10f; rh.ray.mask = -1; rh.ray.time = 0;
                rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                RTCIntersectArguments args; rtcInitIntersectArguments(&args);

                int intersections = 0;
                while (true) {
                    rtcIntersect1(scene, &rh, &args);
                    if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) break;
                    intersections++;
                    rh.ray.tnear = rh.ray.tfar + 1e-4f;
                    rh.ray.tfar = 1e10f;
                    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                }

                if (intersections % 2 != 0) {
                    grid.set(x, y, z);
                }
            }
        }
    });

    // Export occupied voxels as a cube mesh for visualization
    std::cout << "Exporting voxel mesh..." << std::endl;
    std::vector<Vertex> vMesh;
    std::vector<Triangle> tMesh;
    auto addCube = [&](const Vec3& p, float s) {
        unsigned int start = vMesh.size();
        float h = s * 0.5f;
        vMesh.push_back({p.x-h, p.y-h, p.z-h}); vMesh.push_back({p.x+h, p.y-h, p.z-h});
        vMesh.push_back({p.x+h, p.y+h, p.z-h}); vMesh.push_back({p.x-h, p.y+h, p.z-h});
        vMesh.push_back({p.x-h, p.y-h, p.z+h}); vMesh.push_back({p.x+h, p.y-h, p.z+h});
        vMesh.push_back({p.x+h, p.y+h, p.z+h}); vMesh.push_back({p.x-h, p.y+h, p.z+h});
        // 12 triangles
        tMesh.push_back({start+0, start+2, start+1}); tMesh.push_back({start+0, start+3, start+2});
        tMesh.push_back({start+4, start+5, start+6}); tMesh.push_back({start+4, start+6, start+7});
        tMesh.push_back({start+0, start+1, start+5}); tMesh.push_back({start+0, start+5, start+4});
        tMesh.push_back({start+1, start+2, start+6}); tMesh.push_back({start+1, start+6, start+5});
        tMesh.push_back({start+2, start+3, start+7}); tMesh.push_back({start+2, start+7, start+6});
        tMesh.push_back({start+3, start+0, start+4}); tMesh.push_back({start+3, start+4, start+7});
    };

    for (int z = 0; z < grid.nz; ++z) {
        for (int y = 0; y < grid.ny; ++y) {
            for (int x = 0; x < grid.nx; ++x) {
                if (grid.get(x, y, z)) {
                    addCube({grid.minBound.x + (x+0.5f)*grid.voxelSize, grid.minBound.y + (y+0.5f)*grid.voxelSize, grid.minBound.z + (z+0.5f)*grid.voxelSize}, grid.voxelSize * 0.95f);
                }
            }
        }
    }

    std::ofstream out(outputFile);
    out << "OFF" << std::endl;
    out << vMesh.size() << " " << tMesh.size() << " 0" << std::endl;
    for (const auto& v : vMesh) out << v.x << " " << v.y << " " << v.z << std::endl;
    for (const auto& t : tMesh) out << "3 " << t.v0 << " " << t.v1 << " " << t.v2 << std::endl;
    out.close();

    std::cout << "Voxel mesh saved to " << outputFile << " (" << tMesh.size()/12 << " voxels)" << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
