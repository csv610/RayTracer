#include <embree4/rtcore.h>
#include <tbb/parallel_for.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include "mesh_utils.h"
#include "MeshIO.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_mesh> [output.off] [samples_per_face]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "ao_output.off";
    int numSamples = (argc >= 4) ? std::stoi(argv[3]) : 64;

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) {
        std::cerr << "Failed to load mesh: " << inputFile << std::endl;
        return 1;
    }

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

    std::vector<float> aoValues(mesh.triangles.size());
    int numTheta = std::max(1, (int)sqrt(numSamples / 2));
    int numPhi = numSamples / numTheta;

    std::cout << "Baking Ambient Occlusion (" << numTheta * numPhi << " samples per face)..." << std::endl;

    tbb::parallel_for(size_t(0), mesh.triangles.size(), [&](size_t triIdx) {
        const Triangle& tri = mesh.triangles[triIdx];
        Vec3 normal = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
        Vec3 center = computeFaceCenter(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);

        // Create coordinate frame
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
        float rayLength = 1e10f; // Global AO. Could be limited for local AO.

        for (int i = 0; i < numTheta; ++i) {
            for (int j = 0; j < numPhi; ++j) {
                float theta = 1.5707f * (i + 0.5f) / numTheta; // 0 to PI/2
                float phi = 6.2831f * (j + 0.5f) / numPhi;    // 0 to 2PI

                float sinT = sin(theta);
                float cosT = cos(theta);
                float sinP = sin(phi);
                float cosP = cos(phi);

                Vec3 rayDir = {
                    (tangent.x * cosP + bitangent.x * sinP) * sinT + normal.x * cosT,
                    (tangent.y * cosP + bitangent.y * sinP) * sinT + normal.y * cosT,
                    (tangent.z * cosP + bitangent.z * sinP) * sinT + normal.z * cosT
                };

                RTCRayHit rh;
                float epsilon = 1e-4f;
                rh.ray.org_x = center.x + normal.x * epsilon;
                rh.ray.org_y = center.y + normal.y * epsilon;
                rh.ray.org_z = center.z + normal.z * epsilon;
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
        }
        aoValues[triIdx] = 1.0f - ((float)hits / (numTheta * numPhi));
    });

    // Save as OFF with face colors
    FILE* out = fopen(outputFile.c_str(), "w");
    fprintf(out, "OFF\n%zu %zu 0\n", mesh.vertices.size(), mesh.triangles.size());
    for (const auto& v : mesh.vertices) fprintf(out, "%f %f %f\n", v.x, v.y, v.z);
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        float ao = aoValues[i];
        // Convert AO to grayscale (1.0 = white/exposed, 0.0 = black/occluded)
        fprintf(out, "3 %u %u %u %f %f %f\n", 
            mesh.triangles[i].v0, mesh.triangles[i].v1, mesh.triangles[i].v2,
            ao, ao, ao);
    }
    fclose(out);

    std::cout << "AO bake complete. Output saved to: " << outputFile << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
