#include "RayTracer.h"
#include <tbb/parallel_for.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "mesh_utils.h"
#include "MeshIO.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [num_rays_per_vtx] [output.off]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 4) ? argv[3] : "projected_thickness.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    Scene scene;
    scene.addMesh(mesh);
    scene.commit();

    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    float diag = bbox.size().length();
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
        float l = n.length();
        if(l > 0) { n.x /= l; n.y /= l; n.z /= l; }
    }

    std::cout << "Computing projected thickness for " << mesh.vertices.size() << " vertices..." << std::endl;

    tbb::parallel_for(size_t(0), mesh.vertices.size(), [&](size_t i) {
        const auto& p = mesh.vertices[i];
        const auto& n = vtxNormals[i];
        
        Ray ray;
        ray.org = {p.x - n.x * epsilon, p.y - n.y * epsilon, p.z - n.z * epsilon};
        ray.dir = {-n.x, -n.y, -n.z};
        ray.tnear = 0.0f;
        ray.tfar = diag;
        
        Hit hit = RayTracer::intersect(scene, ray);
        if (hit.hit) {
            thickness[i] = hit.t;
        } else {
            thickness[i] = 0.0f;
        }
    });

    float maxT = *std::max_element(thickness.begin(), thickness.end());
    mesh.vertexColors.resize(mesh.vertices.size());
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        float t = (maxT > 0) ? thickness[i] / maxT : 0;
        mesh.vertexColors[i] = getJetColor(t);
    }
    MeshIO::save(outputFile, mesh);
    
    std::cout << "Thickness analysis saved to " << outputFile << std::endl;
    return 0;
}
