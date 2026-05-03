#include <iostream>
#include <vector>
#include <string>
#include "MeshIO.h"
#include "SampleSurface.h"
#include "argparse/argparse.h"

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("sample_near_surface", 
        "Generate sample points near the mesh surface (inside, outside, or on surface).");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_positional("samples", "Number of sample points to generate");
    parser.add_argument("-s", "Sample location: +1=outside, -1=inside, 0=on surface (default: 0)", "0");
    parser.add_argument("-o", "Output mesh file (default: surface_samples.off)", "surface_samples.off");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    int numSamples = parser.get_int("samples");
    int side = parser.get_int("side", 0);
    std::string outputFile = parser.get("o");

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) {
        std::cerr << "Failed to load mesh: " << inputFile << std::endl;
        return 1;
    }

    try {
        SampleSurface sampler(mesh);
        std::string sideStr = (side > 0) ? "outside" : (side < 0) ? "inside" : "on";
        std::cout << "Sampling " << numSamples << " points " << sideStr << " surface..." << std::endl;
        
        std::vector<SampledPoint> samples = sampler.sample(numSamples, side);

        Mesh outMesh;
        for (const auto& s : samples) {
            outMesh.vertices.push_back({s.p.x, s.p.y, s.p.z});
            outMesh.vertexNormals.push_back(s.n);
        }
        MeshIO::save(outputFile, outMesh);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}