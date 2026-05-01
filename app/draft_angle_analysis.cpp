#include "ManufacturingAnalyzer.h"
#include "MeshIO.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [pull_x pull_y pull_z] [output.off]" << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!MeshIO::load(argv[1], mesh)) return 1;

    Vec3 pullDir = {0, 0, 1};
    int outIdx = 2;
    if (argc >= 5) {
        pullDir = {(float)atof(argv[2]), (float)atof(argv[3]), (float)atof(argv[4])};
        outIdx = 5;
    }
    std::string outputFile = (argc > outIdx) ? argv[outIdx] : "draft_angles.off";

    ManufacturingAnalyzer analyzer(mesh);
    auto res = analyzer.analyzeDraftAngles(pullDir);

    std::ofstream out(outputFile);
    out << "OFF\n" << mesh.vertices.size() << " " << mesh.triangles.size() << " 0\n";
    for (const auto& v : mesh.vertices) out << v.x << " " << v.y << " " << v.z << "\n";
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        out << "3 " << mesh.triangles[i].v0 << " " << mesh.triangles[i].v1 << " " << mesh.triangles[i].v2 
            << " " << res.colors[i].x << " " << res.colors[i].y << " " << res.colors[i].z << "\n";
    }
    std::cout << "Draft angle analysis saved to " << outputFile << std::endl;

    return 0;
}
