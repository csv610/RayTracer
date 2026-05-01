#include <embree4/rtcore.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "mesh_utils.h"
#include "MeshIO.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.ppm]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "shadow.ppm";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene scene = rtcNewScene(device);

    // Mesh
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
    memcpy(vb, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
    memcpy(ib, mesh.triangles.data(), mesh.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 size = bbox.size();
    float diag = size.length();

    // Plane
    RTCGeometry plane = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* pvb = (Vertex*)rtcSetNewGeometryBuffer(plane, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), 4);
    float pSize = diag * 5.0f;
    pvb[0] = {bbox.min.x - pSize, bbox.min.y - pSize, bbox.min.z - 0.01f * diag};
    pvb[1] = {bbox.max.x + pSize, bbox.min.y - pSize, bbox.min.z - 0.01f * diag};
    pvb[2] = {bbox.max.x + pSize, bbox.max.y + pSize, bbox.min.z - 0.01f * diag};
    pvb[3] = {bbox.min.x - pSize, bbox.max.y + pSize, bbox.min.z - 0.01f * diag};
    Triangle* pib = (Triangle*)rtcSetNewGeometryBuffer(plane, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), 2);
    pib[0] = {0, 1, 2}; pib[1] = {0, 2, 3};
    rtcCommitGeometry(plane);
    rtcAttachGeometry(scene, plane);
    rtcReleaseGeometry(plane);

    rtcCommitScene(scene);

    const int width = 800, height = 600;
    std::vector<Vec3> image(width * height);
    Vec3 lightDir = {0.5f, 0.5f, 1.0f};
    float lLen = lightDir.length();
    lightDir.x /= lLen; lightDir.y /= lLen; lightDir.z /= lLen;

    Vec3 center = {(bbox.min.x + bbox.max.x) * 0.5f, (bbox.min.y + bbox.max.y) * 0.5f, (bbox.min.z + bbox.max.z) * 0.5f};

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            RTCRayHit rh;
            rh.ray.org_x = center.x; rh.ray.org_y = center.y; rh.ray.org_z = center.z + diag * 2.0f;
            rh.ray.dir_x = (x / (float)width - 0.5f) * 1.5f;
            rh.ray.dir_y = (0.5f - y / (float)height) * 1.5f;
            rh.ray.dir_z = -1.0f;
            rh.ray.tnear = 0.0f; rh.ray.tfar = 1e10f; rh.ray.mask = -1;
            rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            RTCIntersectArguments args; rtcInitIntersectArguments(&args);
            rtcIntersect1(scene, &rh, &args);

            if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                Vec3 hitP = {rh.ray.org_x + rh.ray.dir_x * rh.ray.tfar, rh.ray.org_y + rh.ray.dir_y * rh.ray.tfar, rh.ray.org_z + rh.ray.dir_z * rh.ray.tfar};
                RTCRay sray;
                sray.org_x = hitP.x + rh.hit.Ng_x * 1e-4f; sray.org_y = hitP.y + rh.hit.Ng_y * 1e-4f; sray.org_z = hitP.z + rh.hit.Ng_z * 1e-4f;
                sray.dir_x = lightDir.x; sray.dir_y = lightDir.y; sray.dir_z = lightDir.z;
                sray.tnear = 0.0f; sray.tfar = 1e10f; sray.mask = -1;
                RTCOccludedArguments sargs; rtcInitOccludedArguments(&sargs);
                rtcOccluded1(scene, &sray, &sargs);

                float shadow = (sray.tfar < 0) ? 0.3f : 1.0f;
                float dot = std::max(0.2f, (rh.hit.Ng_x * lightDir.x + rh.hit.Ng_y * lightDir.y + rh.hit.Ng_z * lightDir.z));
                image[y * width + x] = {dot * shadow, dot * shadow, dot * shadow};
            } else {
                image[y * width + x] = {0.1f, 0.1f, 0.2f};
            }
        }
    }

    MeshIO::savePPM(outputFile, width, height, image);
    std::cout << "Saved to " << outputFile << std::endl;

    rtcReleaseScene(scene); rtcReleaseDevice(device);
    return 0;
}
