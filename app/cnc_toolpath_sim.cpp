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
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [resolution] [tool_radius] [output.off]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    int res = (argc >= 3) ? std::atoi(argv[2]) : 128;
    float toolRadius = (argc >= 4) ? (float)atof(argv[3]) : 2.0f;
    std::string outputFile = (argc >= 5) ? argv[4] : "cnc_simulation.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 size = bbox.size();

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

    std::cout << "Simulating CNC toolpath (Ball-end tool) at resolution " << res << "x" << res << "..." << std::endl;

    std::vector<float> simulatedHeights(res * res, bbox.min.z);
    std::vector<float> targetHeights(res * res, bbox.min.z);

    tbb::parallel_for(0, res, [&](int y) {
        for (int x = 0; x < res; ++x) {
            float px = bbox.min.x + (x + 0.5f) * (size.x / res);
            float py = bbox.min.y + (y + 0.5f) * (size.y / res);

            // 1. Find the actual target height of the mesh at this point
            RTCRayHit rh;
            rh.ray.org_x = px; rh.ray.org_y = py; rh.ray.org_z = bbox.max.z + size.z * 0.1f;
            rh.ray.dir_x = 0; rh.ray.dir_y = 0; rh.ray.dir_z = -1.0f;
            rh.ray.tnear = 0.0f; rh.ray.tfar = size.z * 1.5f; rh.ray.mask = -1;
            rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            RTCIntersectArguments args; rtcInitIntersectArguments(&args);
            rtcIntersect1(scene, &rh, &args);

            if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                targetHeights[y * res + x] = rh.ray.org_z - rh.ray.tfar;
            }
        }
    });

    // 2. Simulate tool scraping the surface (incorporating tool radius)
    // For each point, the tool height is constrained by neighbors within toolRadius
    int pixelRadius = (int)ceil(toolRadius / (size.x / res));

    tbb::parallel_for(0, res, [&](int y) {
        for (int x = 0; x < res; ++x) {
            float maxHeight = bbox.min.z;
            for (int dy = -pixelRadius; dy <= pixelRadius; ++dy) {
                for (int dx = -pixelRadius; dx <= pixelRadius; ++dx) {
                    int nx = x + dx;
                    int ny = y + dy;
                    if (nx >= 0 && nx < res && ny >= 0 && ny < res) {
                        float distSq = (dx*dx + dy*dy) * pow(size.x / res, 2);
                        if (distSq <= toolRadius * toolRadius) {
                            // Circular tool profile (ball-end)
                            float offset = sqrt(toolRadius * toolRadius - distSq) - toolRadius;
                            maxHeight = std::max(maxHeight, targetHeights[ny * res + nx] + offset);
                        }
                    }
                }
            }
            simulatedHeights[y * res + x] = maxHeight;
        }
    });

    // Export a heightmap mesh where color represents "Leftover Material" (Scallop)
    std::cout << "Exporting simulation results..." << std::endl;
    std::vector<Vertex> vOut;
    std::vector<Triangle> tOut;
    for (int y = 0; y < res; ++y) {
        for (int x = 0; x < res; ++x) {
            float px = bbox.min.x + x * (size.x / res);
            float py = bbox.min.y + y * (size.y / res);
            float leftover = simulatedHeights[y * res + x] - targetHeights[y * res + x];
            
            // Map leftover to color (Red for high leftover/scallop, Blue for perfect match)
            float t = std::clamp(leftover / (toolRadius * 0.5f), 0.0f, 1.0f);
            Vec3 c = getJetColor(1.0f - t);
            
            unsigned int idx = vOut.size();
            vOut.push_back({px, py, simulatedHeights[y * res + x]});
            // We need to store color in the OFF file, we'll use per-vertex colors
            // Note: OFF usually expects colors after vertices or on faces.
        }
    }

    std::ofstream out(outputFile);
    out << "OFF" << std::endl;
    out << vOut.size() << " " << (res-1)*(res-1)*2 << " 0" << std::endl;
    for (size_t i = 0; i < vOut.size(); ++i) {
        int x = i % res;
        int y = i / res;
        float leftover = simulatedHeights[y * res + x] - targetHeights[y * res + x];
        float t = std::clamp(leftover / (toolRadius * 0.1f), 0.0f, 1.0f);
        Vec3 c = getJetColor(1.0f - t);
        out << vOut[i].x << " " << vOut[i].y << " " << vOut[i].z << " " << c.x << " " << c.y << " " << c.z << std::endl;
    }
    for (int y = 0; y < res - 1; ++y) {
        for (int x = 0; x < res - 1; ++x) {
            unsigned int i0 = y * res + x;
            unsigned int i1 = y * res + (x + 1);
            unsigned int i2 = (y + 1) * res + (x + 1);
            unsigned int i3 = (y + 1) * res + x;
            out << "3 " << i0 << " " << i1 << " " << i2 << std::endl;
            out << "3 " << i0 << " " << i2 << " " << i3 << std::endl;
        }
    }
    out.close();

    std::cout << "CNC Simulation saved to " << outputFile << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
