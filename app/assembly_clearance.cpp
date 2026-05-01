#include "AssemblyAnalyzer.h"
#include "MeshIO.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <partA.off> <partB.off> [clearance_threshold] [output.off]" << std::endl;
        return 1;
    }

    Mesh meshA, meshB;
    if (!MeshIO::load(argv[1], meshA) || !MeshIO::load(argv[2], meshB)) return 1;

    float threshold = (argc >= 4) ? (float)atof(argv[3]) : 1.0f;
    std::string outputFile = (argc >= 5) ? argv[4] : "clearance_results.off";

    AssemblyAnalyzer analyzer(meshA, meshB);
    auto res = analyzer.analyzeClearance(threshold);

    MeshIO::save(outputFile, res.getColoredMesh(meshA));

    }
    std::cout << "Analysis complete: " << res.collisions << " collisions, " << res.violations << " clearance violations." << std::endl;

    return 0;
}
