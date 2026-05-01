#include <iostream>
#include <vector>
#include <string>
#include "MeshIO.h"
#include "SampleSurface.h"

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <input_mesh> <num_samples> <side> [output.off]" << std::endl;
        std::cerr << "  side = +1: outside surface" << std::endl;
        std::cerr << "  side = -1: inside surface" << std::endl;
        std::cerr << "  side =  0: on surface" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    int numSamples = std::stoi(argv[2]);
    int side = std::stoi(argv[3]);
    std::string outputFile = (argc >= 5) ? argv[4] : "surface_samples.off";

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

        // Save as extended OFF with normals (NOFF)
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
