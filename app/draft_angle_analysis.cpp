#include "ManufacturingAnalyzer.h"
#include "MeshIO.h"
#include <iostream>
#include <string>

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
        float l = pullDir.length();
        if(l > 0) { pullDir.x /= l; pullDir.y /= l; pullDir.z /= l; }
        outIdx = 5;
    }
    std::string outputFile = (argc > outIdx) ? argv[outIdx] : "draft_angles.off";

    ManufacturingAnalyzer analyzer(mesh);
    auto res = analyzer.analyzeDraftAngles(pullDir);
    Mesh coloredMesh = res.getColoredMesh(mesh);
    MeshIO::save(outputFile, coloredMesh);

    std::cout << "Draft angle analysis saved to " << outputFile << std::endl;

    return 0;
}
