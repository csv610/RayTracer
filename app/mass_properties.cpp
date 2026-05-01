#include <embree4/rtcore.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include "mesh_utils.h"
#include "MeshIO.h"

struct MassAccumulator {
    double volume = 0;
    double comX = 0, comY = 0, comZ = 0;
    double Ixx = 0, Iyy = 0, Izz = 0;
    double Ixy = 0, Iyz = 0, Izx = 0;

    void operator+=(const MassAccumulator& other) {
        volume += other.volume;
        comX += other.comX; comY += other.comY; comZ += other.comZ;
        Ixx += other.Ixx; Iyy += other.Iyy; Izz += other.Izz;
        Ixy += other.Ixy; Iyz += other.Iyz; Izx += other.Izx;
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [resolution]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    int res = (argc >= 3) ? std::atoi(argv[2]) : 128;

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 size = bbox.size();
    float diag = sqrt(size.x*size.x + size.y*size.y + size.z*size.z);

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

    double dx = size.x / res;
    double dy = size.y / res;
    double areaStep = dx * dy;

    std::cout << "Calculating mass properties with " << res << "x" << res << " ray grid..." << std::endl;

    MassAccumulator total = tbb::parallel_reduce(
        tbb::blocked_range<int>(0, res),
        MassAccumulator(),
        [&](const tbb::blocked_range<int>& r, MassAccumulator init) -> MassAccumulator {
            for (int i = r.begin(); i != r.end(); ++i) {
                for (int j = 0; j < res; ++j) {
                    double x = bbox.min.x + (i + 0.5) * dx;
                    double y = bbox.min.y + (j + 0.5) * dy;

                    RTCRayHit rh;
                    rh.ray.org_x = (float)x; rh.ray.org_y = (float)y; rh.ray.org_z = bbox.min.z - diag * 0.1f;
                    rh.ray.dir_x = 0; rh.ray.dir_y = 0; rh.ray.dir_z = 1.0f;
                    rh.ray.tnear = 0.0f; rh.ray.tfar = diag * 1.2f; rh.ray.mask = -1;
                    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    RTCIntersectArguments args; rtcInitIntersectArguments(&args);

                    std::vector<float> hits;
                    while (true) {
                        rtcIntersect1(scene, &rh, &args);
                        if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) break;
                        hits.push_back(rh.ray.tfar);
                        rh.ray.tnear = rh.ray.tfar + diag * 1e-6f;
                        rh.ray.tfar = diag * 1.2f;
                        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    }

                    if (hits.size() >= 2) {
                        std::sort(hits.begin(), hits.end());
                        for (size_t k = 0; k + 1 < hits.size(); k += 2) {
                            double zStart = rh.ray.org_z + hits[k];
                            double zEnd = rh.ray.org_z + hits[k+1];
                            double length = zEnd - zStart;
                            double zMid = (zStart + zEnd) * 0.5;

                            double dVol = length * areaStep;
                            init.volume += dVol;
                            init.comX += x * dVol;
                            init.comY += y * dVol;
                            init.comZ += zMid * dVol;

                            // Inertia components (relative to origin, will shift to CoM later)
                            // Using integral of x^2, y^2, z^2 over the segment
                            init.Ixx += (y*y * length + (zEnd*zEnd*zEnd - zStart*zStart*zStart)/3.0) * areaStep;
                            init.Iyy += (x*x * length + (zEnd*zEnd*zEnd - zStart*zStart*zStart)/3.0) * areaStep;
                            init.Izz += (x*x + y*y) * length * areaStep;
                            init.Ixy += (x * y * length) * areaStep;
                            init.Iyz += (y * (zEnd*zEnd - zStart*zStart)*0.5) * areaStep;
                            init.Izx += (x * (zEnd*zEnd - zStart*zStart)*0.5) * areaStep;
                        }
                    }
                }
            }
            return init;
        },
        [](MassAccumulator a, MassAccumulator b) {
            a += b; return a;
        }
    );

    if (total.volume > 0) {
        double comX = total.comX / total.volume;
        double comY = total.comY / total.volume;
        double comZ = total.comZ / total.volume;

        // Shift Inertia Tensor to Center of Mass (Parallel Axis Theorem)
        double Ixx_com = total.Ixx - total.volume * (comY*comY + comZ*comZ);
        double Iyy_com = total.Iyy - total.volume * (comX*comX + comZ*comZ);
        double Izz_com = total.Izz - total.volume * (comX*comX + comY*comY);
        double Ixy_com = total.Ixy - total.volume * (comX * comY);
        double Iyz_com = total.Iyz - total.volume * (comY * comZ);
        double Izx_com = total.Izx - total.volume * (comZ * comX);

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "\nMass Properties (assuming density = 1.0):" << std::endl;
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "Volume:         " << total.volume << std::endl;
        std::cout << "Center of Mass: (" << comX << ", " << comY << ", " << comZ << ")" << std::endl;
        std::cout << "\nInertia Tensor (at CoM):" << std::endl;
        std::cout << "| " << std::setw(12) << Ixx_com << " " << std::setw(12) << -Ixy_com << " " << std::setw(12) << -Izx_com << " |" << std::endl;
        std::cout << "| " << std::setw(12) << -Ixy_com << " " << std::setw(12) << Iyy_com << " " << std::setw(12) << -Iyz_com << " |" << std::endl;
        std::cout << "| " << std::setw(12) << -Izx_com << " " << std::setw(12) << -Iyz_com << " " << std::setw(12) << Izz_com << " |" << std::endl;
    } else {
        std::cout << "Error: Could not calculate mass properties (Volume is 0). Ensure the mesh is manifold and closed." << std::endl;
    }

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
