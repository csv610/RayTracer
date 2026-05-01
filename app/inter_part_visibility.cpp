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
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <target_part.off> <occluding_env.off> <viewer_x viewer_y viewer_z> [output.off]" << std::endl;
        return 1;
    }

    std::string partFile = argv[1];
    std::string envFile = argv[2];
    Vec3 viewerPos = {(float)atof(argv[3]), (float)atof(argv[4]), (float)atof(argv[5])};
    std::string outputFile = (argc >= 7) ? argv[6] : "visibility_analysis.off";

    Mesh part, env;
    if (!MeshIO::load(partFile, part)) return 1;
    if (!MeshIO::load(envFile, env)) return 1;

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene scene = rtcNewScene(device);
    
    // Add both to scene. Part = GeomID 0, Env = GeomID 1
    RTCGeometry geomP = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vbP = (Vertex*)rtcSetNewGeometryBuffer(geomP, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), part.vertices.size());
    memcpy(vbP, part.vertices.data(), part.vertices.size() * sizeof(Vertex));
    Triangle* ibP = (Triangle*)rtcSetNewGeometryBuffer(geomP, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), part.triangles.size());
    memcpy(ibP, part.triangles.data(), part.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(geomP);
    rtcAttachGeometry(scene, geomP);
    rtcReleaseGeometry(geomP);

    RTCGeometry geomE = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vbE = (Vertex*)rtcSetNewGeometryBuffer(geomE, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), env.vertices.size());
    memcpy(vbE, env.vertices.data(), env.vertices.size() * sizeof(Vertex));
    Triangle* ibE = (Triangle*)rtcSetNewGeometryBuffer(geomE, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), env.triangles.size());
    memcpy(ibE, env.triangles.data(), env.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(geomE);
    rtcAttachGeometry(scene, geomE);
    rtcReleaseGeometry(geomE);
    
    rtcCommitScene(scene);

    std::vector<Vec3> triColors(part.triangles.size());
    std::cout << "Analyzing visibility of target part from viewer at (" << viewerPos.x << ", " << viewerPos.y << ", " << viewerPos.z << ")..." << std::endl;

    tbb::parallel_for(size_t(0), part.triangles.size(), [&](size_t i) {
        const Triangle& tri = part.triangles[i];
        Vec3 faceCenter = computeFaceCenter(part.vertices[tri.v0], part.vertices[tri.v1], part.vertices[tri.v2]);
        Vec3 normal = computeFaceNormal(part.vertices[tri.v0], part.vertices[tri.v1], part.vertices[tri.v2]);

        // Ray from viewer to face center
        Vec3 dir = {faceCenter.x - viewerPos.x, faceCenter.y - viewerPos.y, faceCenter.z - viewerPos.z};
        float dist = sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
        if (dist > 0) { dir.x /= dist; dir.y /= dist; dir.z /= dist; }

        RTCRayHit rh;
        rh.ray.org_x = viewerPos.x; rh.ray.org_y = viewerPos.y; rh.ray.org_z = viewerPos.z;
        rh.ray.dir_x = dir.x; rh.ray.dir_y = dir.y; rh.ray.dir_z = dir.z;
        rh.ray.tnear = 0.0f; rh.ray.tfar = dist * 1.01f; rh.ray.mask = -1;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        RTCIntersectArguments args; rtcInitIntersectArguments(&args);
        
        rtcIntersect1(scene, &rh, &args);

        // If it hits the target part (GeomID 0) and NO occluder (GeomID 1) was closer
        if (rh.hit.geomID == 0 && std::abs(rh.ray.tfar - dist) < dist * 0.05f) {
            triColors[i] = {0.0f, 1.0f, 0.0f}; // Visible (Green)
        } else {
            triColors[i] = {1.0f, 0.0f, 0.0f}; // Occluded (Red)
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

    int visible = 0;
    for(const auto& c : triColors) if (c.y > 0.5f) visible++;
    std::cout << "Visibility analysis complete: " << visible << " triangles visible. Saved to " << outputFile << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
