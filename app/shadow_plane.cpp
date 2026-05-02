#include "RayTracer.h"
#include "MeshIO.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "mesh_utils.h"

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "shadow.ppm";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene scene = rtcNewScene(device);

    // Mesh
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Scene scene;
    scene.addMesh(mesh);

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 size = bbox.size();
    float diag = size.length();

    Mesh plane;
    float pSize = diag * 5.0f;
    plane.vertices = {
        {bbox.min.x - pSize, bbox.min.y - pSize, bbox.min.z - 0.01f * diag},
        {bbox.max.x + pSize, bbox.min.y - pSize, bbox.min.z - 0.01f * diag},
        {bbox.max.x + pSize, bbox.max.y + pSize, bbox.min.z - 0.01f * diag},
        {bbox.min.x - pSize, bbox.max.y + pSize, bbox.min.z - 0.01f * diag}
    };
    plane.triangles = {{0, 1, 2}, {0, 2, 3}};
    scene.addMesh(plane);
    scene.commit();


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
        for (int x = 0; x < width; ++x) {
            Ray ray;
            ray.org = {center.x, center.y, center.z + diag * 2.0f};
            ray.dir = {(x / (float)width - 0.5f) * 1.5f, (0.5f - y / (float)height) * 1.5f, -1.0f};
            Hit hit = RayTracer::intersect(scene, ray);

            if (hit.hit) {
                Vec3 hitP = {ray.org.x + ray.dir.x * hit.t, ray.org.y + ray.dir.y * hit.t, ray.org.z + ray.dir.z * hit.t};
                Ray sray;
                sray.org = {hitP.x + hit.normal.x * 1e-4f, hitP.y + hit.normal.y * 1e-4f, hitP.z + hit.normal.z * 1e-4f};
                sray.dir = lightDir;
                sray.tnear = 0.0f; sray.tfar = 1e10f;
                
                float shadow = RayTracer::occluded(scene, sray) ? 0.3f : 1.0f;
                float dot = std::max(0.2f, (hit.normal.x * lightDir.x + hit.normal.y * lightDir.y + hit.normal.z * lightDir.z));
                image[y * width + x] = {dot * shadow, dot * shadow, dot * shadow};

    }

    MeshIO::savePPM(outputFile, width, height, image);
    std::cout << "Saved to " << outputFile << std::endl;

    rtcReleaseScene(scene); rtcReleaseDevice(device);
    return 0;
}
