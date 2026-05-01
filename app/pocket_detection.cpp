#include <embree4/rtcore.h>
#include <tbb/parallel_for.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include "mesh_utils.h"
#include "MeshIO.h"

struct PocketResult {
    float exposure; // 0.0 (fully trapped) to 1.0 (fully exposed)
    Vec3 color;
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.off] [samples_per_face]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "pockets.off";
    int numSamples = (argc >= 4) ? std::atoi(argv[3]) : 64;

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) {
        std::cerr << "Failed to load mesh: " << inputFile << std::endl;
        return 1;
    }

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

    Vertex center;
    float radius;
    computeBoundingSphere(mesh, center, radius);
    float rayLength = radius * 2.0f;

    std::vector<PocketResult> results(mesh.triangles.size());
    std::cout << "Detecting pockets on " << mesh.triangles.size() << " triangles using " << numSamples << " samples per face..." << std::endl;

    // Use a fixed set of directions for consistency or random for better sampling
    // Here we'll use a deterministic quasi-random sampling on the hemisphere
    auto getHemisphereDir = [](float u, float v) {
        float phi = 2.0f * M_PI * u;
        float cosTheta = v;
        float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
        return Vec3{cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta};
    };

    tbb::parallel_for(size_t(0), mesh.triangles.size(), [&](size_t i) {
        const Triangle& tri = mesh.triangles[i];
        Vec3 normal = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
        Vec3 faceCenter = computeFaceCenter(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);

        // Create local frame
        Vec3 up = (std::abs(normal.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{1, 0, 0};
        Vec3 tangent = {normal.y * up.z - normal.z * up.y,
                        normal.z * up.x - normal.x * up.z,
                        normal.x * up.y - normal.y * up.x};
        float tLen = sqrt(tangent.x*tangent.x + tangent.y*tangent.y + tangent.z*tangent.z);
        tangent.x /= tLen; tangent.y /= tLen; tangent.z /= tLen;
        Vec3 bitangent = {normal.y * tangent.z - normal.z * tangent.y,
                          normal.z * tangent.x - normal.x * tangent.z,
                          normal.x * tangent.y - normal.y * tangent.x};

        int hits = 0;
        float epsilon = 1e-4f * radius;

        for (int s = 0; s < numSamples; ++s) {
            // Hammersley sequence for better distribution
            float u = (float)s / numSamples;
            unsigned int bits = (s << 16) | (s >> 16);
            bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
            bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
            bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
            bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
            float v = (float)bits * 2.3283064365386963e-10;

            Vec3 localDir = getHemisphereDir(u, v);
            Vec3 rayDir = {
                tangent.x * localDir.x + bitangent.x * localDir.y + normal.x * localDir.z,
                tangent.y * localDir.x + bitangent.y * localDir.y + normal.y * localDir.z,
                tangent.z * localDir.x + bitangent.z * localDir.y + normal.z * localDir.z
            };

            RTCRayHit rh;
            rh.ray.org_x = faceCenter.x + normal.x * epsilon;
            rh.ray.org_y = faceCenter.y + normal.y * epsilon;
            rh.ray.org_z = faceCenter.z + normal.z * epsilon;
            rh.ray.dir_x = rayDir.x;
            rh.ray.dir_y = rayDir.y;
            rh.ray.dir_z = rayDir.z;
            rh.ray.tnear = 0.0f;
            rh.ray.tfar = rayLength;
            rh.ray.mask = -1;
            rh.ray.time = 0;
            rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

            RTCIntersectArguments args;
            rtcInitIntersectArguments(&args);
            rtcIntersect1(scene, &rh, &args);

            if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                hits++;
            }
        }

        float exposure = 1.0f - ((float)hits / numSamples);
        results[i].exposure = exposure;
        
        // Color mapping: 
        // Exposure 1.0 (exposed) -> White (1,1,1)
        // Exposure 0.0 (pocket) -> Red (1,0,0)
        results[i].color = {1.0f, exposure, exposure};
    });

    // Save as OFF with colors
    std::ofstream out(outputFile);
    out << "OFF" << std::endl;
    out << mesh.vertices.size() << " " << mesh.triangles.size() << " 0" << std::endl;
    for (const auto& v : mesh.vertices) {
        out << v.x << " " << v.y << " " << v.z << std::endl;
    }
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        const auto& t = mesh.triangles[i];
        const auto& c = results[i].color;
        out << "3 " << t.v0 << " " << t.v1 << " " << t.v2 << " " 
            << c.x << " " << c.y << " " << c.z << std::endl;
    }
    out.close();

    std::cout << "Pocket detection complete. Results saved to " << outputFile << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);

    return 0;
}
