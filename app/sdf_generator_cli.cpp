#include "RayTracer.h"
#include <tbb/parallel_for.h>
#include "argparse/argparse.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "mesh_utils.h"
#include "MeshIO.h"

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("sdf_generator", 
        "Generate signed distance field (SDF) grid around the mesh.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-r", "Grid resolution (default: 32)", "32");
    parser.add_argument("-o", "Output mesh file (default: sdf_grid.off)", "sdf_grid.off");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    int res = parser.get_int("r", 32);
    std::string outputFile = parser.get("o");

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    AABB box;
    for (const auto& v : mesh.vertices) box.expand(v);
    box.pad(0.1f);
    Vec3 size = box.size();
    float diag = size.length();

    Scene scene;
    scene.addMesh(mesh);
    scene.commit();

    int gridRes = std::max(res, 32);
    std::vector<std::vector<Vertex>> buckets(gridRes * gridRes * gridRes);
    auto getBucketIdx = [&](const Vertex& v) {
        int ix = std::clamp((int)((v.x - box.min.x) / size.x * gridRes), 0, gridRes - 1);
        int iy = std::clamp((int)((v.y - box.min.y) / size.y * gridRes), 0, gridRes - 1);
        int iz = std::clamp((int)((v.z - box.min.z) / size.z * gridRes), 0, gridRes - 1);
        return ix * gridRes * gridRes + iy * gridRes + iz;
    };

    for (const auto& t : mesh.triangles) {
        buckets[getBucketIdx(mesh.vertices[t.v0])].push_back(mesh.vertices[t.v0]);
        buckets[getBucketIdx(mesh.vertices[t.v1])].push_back(mesh.vertices[t.v1]);
        buckets[getBucketIdx(mesh.vertices[t.v2])].push_back(mesh.vertices[t.v2]);
        Vertex center = {(mesh.vertices[t.v0].x + mesh.vertices[t.v1].x + mesh.vertices[t.v2].x)/3.0f,
                         (mesh.vertices[t.v0].y + mesh.vertices[t.v1].y + mesh.vertices[t.v2].y)/3.0f,
                         (mesh.vertices[t.v0].z + mesh.vertices[t.v1].z + mesh.vertices[t.v2].z)/3.0f};
        buckets[getBucketIdx(center)].push_back(center);
    }

    struct SDFPoint { Vec3 p; float dist; };
    std::vector<SDFPoint> grid(res * res * res);

    tbb::parallel_for(0, res * res * res, [&](int idx) {
        int i = idx / (res * res), j = (idx / res) % res, k = idx % res;
        Vec3 p = {box.min.x + (i + 0.5f) * (size.x / res), box.min.y + (j + 0.5f) * (size.y / res), box.min.z + (k + 0.5f) * (size.z / res)};

        float minDistSq = 1e30f;
        int bx = std::clamp((int)((p.x - box.min.x) / size.x * gridRes), 0, gridRes - 1);
        int by = std::clamp((int)((p.y - box.min.y) / size.y * gridRes), 0, gridRes - 1);
        int bz = std::clamp((int)((p.z - box.min.z) / size.z * gridRes), 0, gridRes - 1);

        int searchRadius = 2;
        bool found = false;
        while (!found && searchRadius < gridRes) {
            for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
                for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
                    for (int dz = -searchRadius; dz <= searchRadius; ++dz) {
                        int nx = bx + dx, ny = by + dy, nz = bz + dz;
                        if (nx < 0 || nx >= gridRes || ny < 0 || ny >= gridRes || nz < 0 || nz >= gridRes) continue;
                        for (const auto& s : buckets[nx * gridRes * gridRes + ny * gridRes + nz]) {
                            float dist2 = (s.x-p.x)*(s.x-p.x) + (s.y-p.y)*(s.y-p.y) + (s.z-p.z)*(s.z-p.z);
                            if (dist2 < minDistSq) { minDistSq = dist2; found = true; }
                        }
                    }
                }
            }
            if (!found) searchRadius++;
        }
        float d = sqrt(minDistSq);
        if (scene.isInside(p)) d = -d;
        grid[idx] = {p, d};
    });

    Mesh outMesh;
    for (const auto& sp : grid) {
        outMesh.vertices.push_back({sp.p.x, sp.p.y, sp.p.z});
        float val = std::clamp((sp.dist / (diag * 0.15f) + 1.0f) * 0.5f, 0.0f, 1.0f);
        outMesh.vertexColors.push_back(getJetColor(val));
    }
    MeshIO::save(outputFile, outMesh);
    std::cout << "SDF grid saved to " << outputFile << std::endl;
    return 0;
}