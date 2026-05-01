#include <embree4/rtcore.h>
#include <iostream>
#include <vector>
#include <cmath>
#include "mesh_utils.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.ppm]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "render.ppm";

    Mesh mesh;
    if (!readOFF(inputFile, mesh)) {
        std::cerr << "Failed to load mesh: " << inputFile << std::endl;
        return 1;
    }

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene scene = rtcNewScene(device);
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
    for(size_t i=0; i<mesh.vertices.size(); ++i) vb[i] = mesh.vertices[i];

    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
    for(size_t i=0; i<mesh.triangles.size(); ++i) ib[i] = mesh.triangles[i];

    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
    rtcCommitScene(scene);

    Vertex center;
    float radius;
    computeBoundingSphere(mesh, center, radius);

    int width = 800;
    int height = 600;
    std::vector<Vec3> image(width * height);

    float aspect = (float)width / height;
    float camDist = radius * 2.5f;
    Vec3 camPos = {center.x, center.y, center.z + camDist};

    std::cout << "Rendering " << width << "x" << height << "..." << std::endl;

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
                // Simple shading based on normal
                float intensity = std::abs(rayhit.hit.Ng_z); // Face normal Z component
                image[y * width + x] = {intensity, intensity, intensity};
            } else {
                image[y * width + x] = {0.2f, 0.2f, 0.2f}; // Background
            }
        }
    }

    writePPM(outputFile, width, height, image);
    std::cout << "Saved to " << outputFile << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
