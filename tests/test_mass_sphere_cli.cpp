#include "mesh_utils.h"
#include <embree4/rtcore.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iomanip>

int main() {
    Mesh sphere;
    createUVSphere(sphere, 64, 128, 1.0f); // Radius = 1.0
    
    // Expected Volume = 4/3 * PI * r^3 = 4.18879
    // Expected CoM = (0,0,0)
    // Expected Inertia Ixx = Iyy = Izz = 2/5 * M * r^2 = 0.4 * 4.18879 * 1 = 1.675516

    RTCDevice device = rtcNewDevice(nullptr);
    RTCScene scene = rtcNewScene(device);
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), sphere.vertices.size());
    for(size_t i=0; i<sphere.vertices.size(); ++i) vb[i] = sphere.vertices[i];
    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), sphere.triangles.size());
    for(size_t i=0; i<sphere.triangles.size(); ++i) ib[i] = sphere.triangles[i];
    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
    rtcCommitScene(scene);

    int res = 256;
    double dx = 2.2 / res; // Bbox is [-1,1]
    double dy = 2.2 / res;
    double volume = 0, comZ = 0, Izz = 0;

    for (int i = 0; i < res; ++i) {
        for (int j = 0; j < res; ++j) {
            double x = -1.1 + (i + 0.5) * dx;
            double y = -1.1 + (j + 0.5) * dy;
            if (x*x + y*y > 1.1) continue;

            RTCRayHit rh;
            rh.ray.org_x = (float)x; rh.ray.org_y = (float)y; rh.ray.org_z = -1.5f;
            rh.ray.dir_x = 0; rh.ray.dir_y = 0; rh.ray.dir_z = 1.0f;
            rh.ray.tnear = 0.0f; rh.ray.tfar = 3.0f; rh.ray.mask = -1;
            rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            RTCIntersectArguments args; rtcInitIntersectArguments(&args);

            std::vector<float> hits;
            while(true) {
                rtcIntersect1(scene, &rh, &args);
                if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) break;
                hits.push_back(rh.ray.tfar);
                rh.ray.tnear = rh.ray.tfar + 1e-5f;
                rh.ray.tfar = 3.0f;
                rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            }
            if (hits.size() >= 2) {
                std::sort(hits.begin(), hits.end());
                for(size_t k=0; k+1 < hits.size(); k+=2) {
                    double zStart = -1.5 + hits[k];
                    double zEnd = -1.5 + hits[k+1];
                    double len = zEnd - zStart;
                    volume += len * dx * dy;
                    Izz += (x*x + y*y) * len * dx * dy;
                }
            }
        }
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Sphere Test (R=1.0):" << std::endl;
    std::cout << "Calculated Volume: " << volume << " (Expected: 4.188790)" << std::endl;
    std::cout << "Calculated Izz:    " << Izz << " (Expected: 1.675516)" << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return (std::abs(volume - 4.18879) < 0.05) ? 0 : 1;
}
