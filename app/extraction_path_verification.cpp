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

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <part.off> <environment.off> [dir_x dir_y dir_z] [distance] [output.off]" << std::endl;
        return 1;
    }

    std::string partFile = argv[1];
    std::string envFile = argv[2];
    Vec3 moveDir = {(float)atof(argv[3]), (float)atof(argv[4]), (float)atof(argv[5])};
    float len = sqrt(moveDir.x*moveDir.x + moveDir.y*moveDir.y + moveDir.z*moveDir.z);
    if (len > 0) { moveDir.x /= len; moveDir.y /= len; moveDir.z /= len; }
    
    float moveDist = (argc >= 7) ? (float)atof(argv[6]) : 100.0f;
    std::string outputFile = (argc >= 8) ? argv[7] : "extraction_path.off";

    Mesh part, env;
    if (!MeshIO::load(partFile, part)) return 1;
    if (!MeshIO::load(envFile, env)) return 1;

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene sceneEnv = rtcNewScene(device);
    RTCGeometry geomEnv = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vbE = (Vertex*)rtcSetNewGeometryBuffer(geomEnv, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), env.vertices.size());
    memcpy(vbE, env.vertices.data(), env.vertices.size() * sizeof(Vertex));
    Triangle* ibE = (Triangle*)rtcSetNewGeometryBuffer(geomEnv, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), env.triangles.size());
    memcpy(ibE, env.triangles.data(), env.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(geomEnv);
    rtcAttachGeometry(sceneEnv, geomEnv);
    rtcReleaseGeometry(geomEnv);
    rtcCommitScene(sceneEnv);

    std::vector<Vec3> triColors(part.triangles.size());
    std::cout << "Verifying extraction path for " << part.triangles.size() << " triangles..." << std::endl;

    const int numSteps = 20; // Check 20 discrete steps along the path

    tbb::parallel_for(size_t(0), part.triangles.size(), [&](size_t i) {
        const Triangle& tri = part.triangles[i];
        Vec3 v0 = {part.vertices[tri.v0].x, part.vertices[tri.v0].y, part.vertices[tri.v0].z};
        Vec3 v1 = {part.vertices[tri.v1].x, part.vertices[tri.v1].y, part.vertices[tri.v1].z};
        Vec3 v2 = {part.vertices[tri.v2].x, part.vertices[tri.v2].y, part.vertices[tri.v2].z};
        Vec3 center = {(v0.x + v1.x + v2.x)/3.0f, (v0.y + v1.y + v2.y)/3.0f, (v0.z + v1.z + v2.z)/3.0f};

        bool collision = false;
        
        // Optimized check: Cast a single ray of length moveDist from the center
        RTCRayHit rh;
        rh.ray.org_x = center.x; rh.ray.org_y = center.y; rh.ray.org_z = center.z;
        rh.ray.dir_x = moveDir.x; rh.ray.dir_y = moveDir.y; rh.ray.dir_z = moveDir.z;
        rh.ray.tnear = 0.0f; rh.ray.tfar = moveDist; rh.ray.mask = -1;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        
        RTCIntersectArguments args; rtcInitIntersectArguments(&args);
        rtcIntersect1(sceneEnv, &rh, &args);
        
        if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
            collision = true;
        } else {
            // Check corners too for better robustness
            Vec3 corners[3] = {v0, v1, v2};
            for(int c=0; c<3; ++c) {
                rh.ray.org_x = corners[c].x; rh.ray.org_y = corners[c].y; rh.ray.org_z = corners[c].z;
                rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                rtcIntersect1(sceneEnv, &rh, &args);
                if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                    collision = true;
                    break;
                }
            }
        }

        if (collision) {
            triColors[i] = {1.0f, 0.0f, 0.0f}; // Red (Collides)
        } else {
            triColors[i] = {0.0f, 1.0f, 0.0f}; // Green (Clear)
        }
    });

    std::ofstream out(outputFile);
    out << "OFF" << std::endl;
    out << part.vertices.size() << " " << part.triangles.size() << " 0" << std::endl;
    for (const auto& v : part.vertices) out << v.x << " " << v.y << " " << v.z << std::endl;
    for (size_t i = 0; i < part.triangles.size(); ++i) {
        const auto& t = part.triangles[i];
        const auto& c = triColors[i];
        out << "3 " << t.v0 << " " << t.v1 << " " << t.v2 << " " << c.x << " " << c.y << " " << c.z << std::endl;
    }
    out.close();

    int blocked = 0;
    for(const auto& c : triColors) if (c.x > 0.5f) blocked++;
    std::cout << "Path verification complete: " << blocked << " triangles hit obstacles. Saved to " << outputFile << std::endl;

    rtcReleaseScene(sceneEnv);
    rtcReleaseDevice(device);
    return 0;
}
