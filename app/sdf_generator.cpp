#include <embree4/rtcore.h>
#include <tbb/parallel_for.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "mesh_utils.h"
#include "MeshIO.h"

// Robust insideness test using counting method
bool isInside(RTCScene scene, const Vec3& p, float epsilon) {
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
        rh.ray.org_x += rh.ray.dir_x * (rh.ray.tfar + 1e-4f);
        rh.ray.tnear = 0.0f; rh.ray.tfar = 1e10f;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    }
    return (intersections % 2 != 0);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_mesh> [res] [output.off]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    int res = (argc >= 3) ? std::stoi(argv[2]) : 32;
    std::string outputFile = (argc >= 4) ? argv[3] : "sdf_grid.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    AABB box;
    for (const auto& v : mesh.vertices) box.expand(v);
    box.pad(0.1f);
    Vec3 size = box.size();
    float diag = sqrt(size.x*size.x + size.y*size.y + size.z*size.z);

    // Initialize Embree
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

    // Spatial Hashing for fast distance queries
    int gridRes = std::max(res, 32);
    std::vector<std::vector<Vertex>> buckets(gridRes * gridRes * gridRes);
    auto getBucketIdx = [&](const Vertex& v) {
        int ix = std::clamp((int)((v.x - box.min.x) / size.x * gridRes), 0, gridRes - 1);
        int iy = std::clamp((int)((v.y - box.min.y) / size.y * gridRes), 0, gridRes - 1);
        int iz = std::clamp((int)((v.z - box.min.z) / size.z * gridRes), 0, gridRes - 1);
        return ix * gridRes * gridRes + iy * gridRes + iz;
    };

    std::cout << "Bucketing surface samples..." << std::endl;
    for (const auto& t : mesh.triangles) {
        buckets[getBucketIdx(mesh.vertices[t.v0])].push_back(mesh.vertices[t.v0]);
        buckets[getBucketIdx(mesh.vertices[t.v1])].push_back(mesh.vertices[t.v1]);
        buckets[getBucketIdx(mesh.vertices[t.v2])].push_back(mesh.vertices[t.v2]);
        Vertex center = {(mesh.vertices[t.v0].x + mesh.vertices[t.v1].x + mesh.vertices[t.v2].x)/3.0f,
                         (mesh.vertices[t.v0].y + mesh.vertices[t.v1].y + mesh.vertices[t.v2].y)/3.0f,
                         (mesh.vertices[t.v0].z + mesh.vertices[t.v1].z + mesh.vertices[t.v2].z)/3.0f};
        buckets[getBucketIdx(center)].push_back(center);
    }

    struct SDFPoint { Vec3 p; float dist; };
    std::vector<SDFPoint> grid(res * res * res);

    std::cout << "Generating SDF grid " << res << "^3..." << std::endl;
    tbb::parallel_for(0, res * res * res, [&](int idx) {
        int i = idx / (res * res);
        int j = (idx / res) % res;
        int k = idx % res;

        Vec3 p = {
            box.min.x + (i + 0.5f) * (size.x / res),
            box.min.y + (j + 0.5f) * (size.y / res),
            box.min.z + (k + 0.5f) * (size.z / res)
        };

        // Search neighboring buckets for the closest point
        float minDistSq = 1e30f;
        int bx = std::clamp((int)((p.x - box.min.x) / size.x * gridRes), 0, gridRes - 1);
        int by = std::clamp((int)((p.y - box.min.y) / size.y * gridRes), 0, gridRes - 1);
        int bz = std::clamp((int)((p.z - box.min.z) / size.z * gridRes), 0, gridRes - 1);

        int searchRadius = 2; // Start with 1, expand if necessary
        bool found = false;
        while (!found && searchRadius < gridRes) {
            for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
                for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
                    for (int dz = -searchRadius; dz <= searchRadius; ++dz) {
                        int nx = bx + dx, ny = by + dy, nz = bz + dz;
    Mesh outMesh;
    for (const auto& sp : grid) {
        outMesh.vertices.push_back({sp.p.x, sp.p.y, sp.p.z});
        float val = std::clamp((sp.dist / (diag * 0.15f) + 1.0f) * 0.5f, 0.0f, 1.0f);
        outMesh.vertexColors.push_back(getJetColor(val));
    }
    MeshIO::save(outputFile, outMesh);

            if (!found) searchRadius++;
        }

        float d = sqrt(minDistSq);
        if (isInside(scene, p, diag * 0.0001f)) d = -d;
        grid[idx] = {p, d};
    });

    Mesh outMesh;
    for (const auto& sp : grid) {
        outMesh.vertices.push_back({sp.p.x, sp.p.y, sp.p.z});
        float val = std::clamp((sp.dist / (diag * 0.15f) + 1.0f) * 0.5f, 0.0f, 1.0f);
        outMesh.vertexColors.push_back(getJetColor(val));
    }
    MeshIO::save(outputFile, outMesh);


    std::cout << "SDF grid saved to " << outputFile << std::endl;
    rtcReleaseScene(scene); rtcReleaseDevice(device);
    return 0;
}
