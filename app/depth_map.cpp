#include <embree4/rtcore.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include "mesh_utils.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.ppm]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "depth.ppm";

    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    if (!readOFF(inputFile, vertices, triangles)) {
        std::cerr << "Failed to load mesh: " << inputFile << std::endl;
        return 1;
    }

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene scene = rtcNewScene(device);
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), vertices.size());
    for(size_t i=0; i<vertices.size(); ++i) vb[i] = vertices[i];

    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), triangles.size());
    for(size_t i=0; i<triangles.size(); ++i) ib[i] = triangles[i];

    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
    rtcCommitScene(scene);

    Vertex center;
    float radius;
    computeBoundingSphere(vertices, center, radius);

    int width = 800;
    int height = 600;
    std::vector<float> depthBuffer(width * height, -1.0f);

    float aspect = (float)width / height;
    float camDist = radius * 2.5f;
    Vec3 camPos = {center.x, center.y, center.z + camDist};

    float minDepth = INFINITY;
    float maxDepth = -INFINITY;

    std::cout << "Rendering depth map..." << std::endl;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float u = (2.0f * (x + 0.5f) / width - 1.0f) * aspect;
            float v = (1.0f - 2.0f * (y + 0.5f) / height);

            RTCRayHit rayhit;
            rayhit.ray.org_x = camPos.x;
            rayhit.ray.org_y = camPos.y;
            rayhit.ray.org_z = camPos.z;
            rayhit.ray.dir_x = u;
            rayhit.ray.dir_y = v;
            rayhit.ray.dir_z = -1.0f;
            
            float len = sqrt(rayhit.ray.dir_x * rayhit.ray.dir_x + rayhit.ray.dir_y * rayhit.ray.dir_y + rayhit.ray.dir_z * rayhit.ray.dir_z);
            rayhit.ray.dir_x /= len; rayhit.ray.dir_y /= len; rayhit.ray.dir_z /= len;

            rayhit.ray.tnear = 0.0f;
            rayhit.ray.tfar = INFINITY;
            rayhit.ray.mask = -1;
            rayhit.ray.flags = 0;
            rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

            RTCIntersectArguments args;
            rtcInitIntersectArguments(&args);
            rtcIntersect1(scene, &rayhit, &args);

            if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                depthBuffer[y * width + x] = rayhit.ray.tfar;
                minDepth = std::min(minDepth, rayhit.ray.tfar);
                maxDepth = std::max(maxDepth, rayhit.ray.tfar);
            }
        }
    }

    // Convert depth to grayscale colors
    std::vector<Vec3> image(width * height);
    for (int i = 0; i < width * height; ++i) {
        if (depthBuffer[i] > 0) {
            // Normalize depth: 1.0 at minDepth (white), 0.0 at maxDepth (black)
            float norm = 1.0f - (depthBuffer[i] - minDepth) / (maxDepth - minDepth + 1e-6f);
            image[i] = {norm, norm, norm};
        } else {
            image[i] = {0.0f, 0.0f, 0.0f}; // Background is black
        }
    }

    writePPM(outputFile, width, height, image);
    std::cout << "Depth map saved to " << outputFile << std::endl;
    std::cout << "Depth range: [" << minDepth << ", " << maxDepth << "]" << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
