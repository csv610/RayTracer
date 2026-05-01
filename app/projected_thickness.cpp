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
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [num_rays_per_vtx] [output.off]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    int numRays = (argc >= 3) ? std::atoi(argv[2]) : 64;
    std::string outputFile = (argc >= 4) ? argv[3] : "projected_thickness.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

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

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    float diag = sqrt(pow(bbox.max.x-bbox.min.x,2) + pow(bbox.max.y-bbox.min.y,2) + pow(bbox.max.z-bbox.min.z,2));
    float epsilon = diag * 1e-4f;

    std::vector<float> thickness(mesh.vertices.size(), 0.0f);
    std::vector<Vec3> vtxNormals(mesh.vertices.size(), {0,0,0});
    for(const auto& tri : mesh.triangles) {
        Vec3 n = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
        vtxNormals[tri.v0].x += n.x; vtxNormals[tri.v0].y += n.y; vtxNormals[tri.v0].z += n.z;
        vtxNormals[tri.v1].x += n.x; vtxNormals[tri.v1].y += n.y; vtxNormals[tri.v1].z += n.z;
        vtxNormals[tri.v2].x += n.x; vtxNormals[tri.v2].y += n.y; vtxNormals[tri.v2].z += n.z;
    }
    for(auto& n : vtxNormals) {
        float l = sqrt(n.x*n.x + n.y*n.y + n.z*n.z);
        if(l > 0) { n.x /= l; n.y /= l; n.z /= l; }
    }

    std::cout << "Computing projected thickness for " << mesh.vertices.size() << " vertices..." << std::endl;

    tbb::parallel_for(size_t(0), mesh.vertices.size(), [&](size_t i) {
        const auto& p = mesh.vertices[i];
        const auto& n = vtxNormals[i];
        
        // Cast ray inward
        RTCRayHit rh;
        rh.ray.org_x = p.x - n.x * epsilon;
        rh.ray.org_y = p.y - n.y * epsilon;
        rh.ray.org_z = p.z - n.z * epsilon;
        rh.ray.dir_x = -n.x;
        rh.ray.dir_y = -n.y;
        rh.ray.dir_z = -n.z;
        rh.ray.tnear = 0.0f;
        rh.ray.tfar = diag;
        rh.ray.mask = -1;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        
        RTCIntersectArguments args; rtcInitIntersectArguments(&args);
        rtcIntersect1(scene, &rh, &args);
        
        if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
            thickness[i] = rh.ray.tfar;
        } else {
            thickness[i] = 0.0f;
    mesh.vertexColors.resize(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        float t = (maxT > 0) ? thickness[i] / maxT : 0;
        mesh.vertexColors[i] = getJetColor(t);
    }
    MeshIO::save(outputFile, mesh);

        float t = (maxT > 0) ? thickness[i] / maxT : 0;
        Vec3 c = getJetColor(t);
        out << mesh.vertices[i].x << " " << mesh.vertices[i].y << " " << mesh.vertices[i].z << " " << c.x << " " << c.y << " " << c.z << std::endl;
    }
    for (const auto& t : mesh.triangles) out << "3 " << t.v0 << " " << t.v1 << " " << t.v2 << std::endl;
    
    std::cout << "Thickness analysis saved to " << outputFile << std::endl;

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}
