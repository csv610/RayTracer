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
    std::string outputFile = (argc >= 3) ? argv[2] : "shadow.ppm";

    Mesh mesh;
    if (!readOFF(inputFile, mesh)) {
        std::cerr << "Failed to load mesh: " << inputFile << std::endl;
        return 1;
    }

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene scene = rtcNewScene(device);

    // Add Mesh
    RTCGeometry meshGeom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(meshGeom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
    for(size_t i=0; i<mesh.vertices.size(); ++i) vb[i] = mesh.vertices[i];
    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(meshGeom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
    for(size_t i=0; i<mesh.triangles.size(); ++i) ib[i] = mesh.triangles[i];
    rtcCommitGeometry(meshGeom);
    unsigned int meshID = rtcAttachGeometry(scene, meshGeom);
    rtcReleaseGeometry(meshGeom);

    // Compute bounding sphere to place plane and camera
    Vertex center;
    float radius;
    computeBoundingSphere(mesh, center, radius);

    // Add Ground Plane (a large square)
    RTCGeometry planeGeom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* pvb = (Vertex*)rtcSetNewGeometryBuffer(planeGeom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), 4);
    float planeSize = radius * 10.0f;
    float planeY = center.y - radius; // Plane at the bottom
    pvb[0] = {center.x - planeSize, planeY, center.z - planeSize};
    pvb[1] = {center.x + planeSize, planeY, center.z - planeSize};
    pvb[2] = {center.x + planeSize, planeY, center.z + planeSize};
    pvb[3] = {center.x - planeSize, planeY, center.z + planeSize};

    Triangle* pib = (Triangle*)rtcSetNewGeometryBuffer(planeGeom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), 2);
    pib[0] = {0, 1, 2};
    pib[1] = {0, 2, 3};
    rtcCommitGeometry(planeGeom);
    unsigned int planeID = rtcAttachGeometry(scene, planeGeom);
    rtcReleaseGeometry(planeGeom);

    rtcCommitScene(scene);

    int width = 800;
    int height = 600;
    std::vector<Vec3> image(width * height);

    Vec3 lightPos = {center.x + radius * 3.0f, center.y + radius * 5.0f, center.z + radius * 2.0f};
    Vec3 camPos = {center.x + radius * 2.0f, center.y + radius * 2.0f, center.z + radius * 5.0f};

    std::cout << "Rendering shadow application..." << std::endl;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float aspect = (float)width / height;
            float u = (2.0f * (x + 0.5f) / width - 1.0f) * aspect;
            float v = (1.0f - 2.0f * (y + 0.5f) / height);

            RTCRayHit rayhit;
            rayhit.ray.org_x = camPos.x;
            rayhit.ray.org_y = camPos.y;
            rayhit.ray.org_z = camPos.z;
            
            // Look towards center
            Vec3 dir = {center.x + u*radius - camPos.x, center.y + v*radius - camPos.y, center.z - camPos.z};
            float dlen = sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
            rayhit.ray.dir_x = dir.x/dlen; rayhit.ray.dir_y = dir.y/dlen; rayhit.ray.dir_z = dir.z/dlen;

            rayhit.ray.tnear = 0.0f;
            rayhit.ray.tfar = INFINITY;
            rayhit.ray.mask = -1;
            rayhit.ray.flags = 0;
            rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

            RTCIntersectArguments args;
            rtcInitIntersectArguments(&args);
            rtcIntersect1(scene, &rayhit, &args);

            if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                Vec3 hitPoint = {
                    rayhit.ray.org_x + rayhit.ray.tfar * rayhit.ray.dir_x,
                    rayhit.ray.org_y + rayhit.ray.tfar * rayhit.ray.dir_y,
                    rayhit.ray.org_z + rayhit.ray.tfar * rayhit.ray.dir_z
                };

                // Shoot shadow ray towards light
                RTCRay shadowRay;
                Vec3 lightDir = {lightPos.x - hitPoint.x, lightPos.y - hitPoint.y, lightPos.z - hitPoint.z};
                float distToLight = sqrt(lightDir.x*lightDir.x + lightDir.y*lightDir.y + lightDir.z*lightDir.z);
                
                shadowRay.org_x = hitPoint.x;
                shadowRay.org_y = hitPoint.y;
                shadowRay.org_z = hitPoint.z;
                shadowRay.dir_x = lightDir.x / distToLight;
                shadowRay.dir_y = lightDir.y / distToLight;
                shadowRay.dir_z = lightDir.z / distToLight;
                shadowRay.tnear = 0.001f; // Avoid self-intersection
                shadowRay.tfar = distToLight;
                shadowRay.mask = -1;
                shadowRay.flags = 0;

                RTCOccludedArguments sargs;
                rtcInitOccludedArguments(&sargs);
                rtcOccluded1(scene, &shadowRay, &sargs);

                // If tfar is -inf, it means it's occluded
                float shadow = (shadowRay.tfar < 0.0f) ? 0.3f : 1.0f;
                
                Vec3 color;
                if (rayhit.hit.geomID == meshID) color = {0.9f, 0.3f, 0.3f}; // Mesh is red
                else color = {0.7f, 0.7f, 0.7f}; // Plane is grey

                image[y * width + x] = {color.x * shadow, color.y * shadow, color.z * shadow};
            } else {
                image[y * width + x] = {0.2f, 0.3f, 0.5f}; // Sky blue background
            }
        }
    }

    writePPM(outputFile, width, height, image);
    std::cout << "Rendered shadow image saved to " << outputFile << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
