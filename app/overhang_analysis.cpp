#include "ManufacturingAnalyzer.h"
#include "MeshIO.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.off] [threshold_deg]" << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!MeshIO::load(argv[1], mesh)) return 1;

    float threshold = (argc >= 4) ? (float)atof(argv[3]) : 45.0f;

    ManufacturingAnalyzer analyzer(mesh);
    auto res = analyzer.analyzeOverhangs(threshold);

    std::string outputFile = (argc >= 3) ? argv[2] : "overhangs.off";
    Mesh coloredMesh = res.getColoredMesh(mesh);
    MeshIO::save(outputFile, coloredMesh);

    std::cout << "Detected " << res.count << " overhang triangles. Saved to " << outputFile << std::endl;

    return 0;
}
