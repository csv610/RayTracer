#include "ManufacturingAnalyzer.h"
#include "MeshIO.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.off] [pull_x pull_y pull_z]" << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!MeshIO::load(argv[1], mesh)) return 1;

    Vec3 pullDir = {0, 0, 1};
    if (argc >= 6) {
        pullDir = {(float)atof(argv[3]), (float)atof(argv[4]), (float)atof(argv[5])};
        float l = sqrt(pullDir.x*pullDir.x + pullDir.y*pullDir.y + pullDir.z*pullDir.z);
        if(l>0) { pullDir.x/=l; pullDir.y/=l; pullDir.z/=l; }
    }

    ManufacturingAnalyzer analyzer(mesh);
    auto res = analyzer.analyzeUndercuts(pullDir);

    std::string outputFile = (argc >= 3) ? argv[2] : "undercuts.off";
    Mesh coloredMesh = res.getColoredMesh(mesh);
    MeshIO::save(outputFile, coloredMesh);

            << " " << res.colors[i].x << " " << res.colors[i].y << " " << res.colors[i].z << "\n";
    }
    std::cout << "Detected " << res.count << " undercut triangles. Saved to " << outputFile << std::endl;

    return 0;
}
