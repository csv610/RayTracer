#include "GeometryAnalyzer.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("ambient_occlusion_baker", 
        "Compute ambient occlusion values for each face using ray-based sampling.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output mesh file (default: ao_baked.off)", "ao_baked.off");
    parser.add_argument("-n", "Number of ray samples per face (default: 64)", "64");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    std::string outputFile = parser.get("o");
    int samples = parser.get_int("samples", 64);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    GeometryAnalyzer analyzer(mesh);
    auto ao = analyzer.computeAmbientOcclusion(samples);

    mesh.faceColors.resize(ao.size());
    for(size_t i = 0; i < ao.size(); ++i) {
        unsigned char c = (unsigned char)(ao[i] * 255.0f);
        mesh.faceColors[i] = {c, c, c, 255};
    }
    
    MeshIO::save(outputFile, mesh);
    std::cout << "AO baking complete. Saved to " << outputFile << std::endl;
    return 0;
}