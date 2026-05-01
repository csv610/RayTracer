#include <iostream>
#include <vector>
#include <string>
#include "mesh_utils.h"
#include "MeshIO.h"
#include "SampleInterior.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_mesh> <num_samples> [output.off]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    int numSamples = std::stoi(argv[2]);
    std::string outputFile = (argc >= 4) ? argv[3] : "interior_samples.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) {
        std::cerr << "Failed to load mesh: " << inputFile << std::endl;
        return 1;
    }

    try {
        SampleInterior sampler(mesh);
        std::vector<Vec3> points = sampler.sample(numSamples);
        Mesh outMesh;
        for (const auto& p : points) outMesh.vertices.push_back({p.x, p.y, p.z});
        MeshIO::save(outputFile, outMesh);


        std::cout << "Successfully saved " << points.size() << " samples to: " << outputFile << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
