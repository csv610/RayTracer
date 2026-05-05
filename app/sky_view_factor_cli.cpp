#include "GeometryAnalyzer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("sky_view_factor", 
        "Compute sky view factor for each face indicating exposure to external environment.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output mesh file (default: sky_view.off)", "sky_view.off");
    parser.add_argument("-n", "Number of ray samples per face (default: 128)", "128");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    std::string outputFile = parser.get("o");
    int samples = parser.get_int("n", 128);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    GeometryAnalyzer analyzer(mesh);
    auto skyView = analyzer.computeSkyViewFactor(samples);

    mesh.faceColors.resize(skyView.size());
    for(size_t i = 0; i < skyView.size(); ++i) {
        unsigned char c = (unsigned char)(skyView[i] * 255.0f);
        mesh.faceColors[i] = {0, 0, c, 255};
    }
    
    MeshIO::save(outputFile, mesh);
    std::cout << "Sky View analysis complete. Saved to " << outputFile << std::endl;
    return 0;
}
