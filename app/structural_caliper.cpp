#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include "MeshIO.h"
#include "StructuralCaliper.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_mesh> [min_thickness_threshold] [num_samples] [output.off]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    float threshold = -1.0f;
    if (argc >= 3) {
        threshold = std::stof(argv[2]);
    } else {
        AABB box;
        for (const auto& v : mesh.vertices) box.expand(v);
        Vec3 s = box.size();
        float diag = s.length();
        threshold = diag * 0.01f;
        std::cout << "Using default threshold: " << threshold << std::endl;
    }

    int numSamples = (argc >= 4) ? std::stoi(argv[3]) : 100000;
    std::string outputFile = (argc >= 5) ? argv[4] : "thickness_analysis.off";

    try {
        StructuralCaliper caliper(mesh);
        auto results = caliper.analyze(numSamples, threshold);

        Mesh outMesh;
        for (const auto& res : results) {
            outMesh.vertices.push_back({res.p.x, res.p.y, res.p.z});
            outMesh.vertexColors.push_back(res.color);
        }
        MeshIO::save(outputFile, outMesh);

        std::cout << "Successfully saved structural analysis to: " << outputFile << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
