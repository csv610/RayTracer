#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include "MeshIO.h"
#include "StructuralCaliper.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_mesh> [min_thickness_threshold] [num_samples] [output.off]" << std::endl;
        std::cerr << "  If threshold is omitted, it defaults to 1% of the model diagonal." << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    // Calculate default threshold if not provided
    float threshold = -1.0f;
    if (argc >= 3) {
        threshold = std::stof(argv[2]);
    } else {
        AABB box;
        for (const auto& v : mesh.vertices) box.expand(v);
        Vec3 s = box.size();
        float diag = sqrt(s.x*s.x + s.y*s.y + s.z*s.z);
        threshold = diag * 0.01f; // 1% of diagonal
        std::cout << "Using default threshold (1% of diagonal): " << threshold << std::endl;
    }

    int numSamples = (argc >= 4) ? std::stoi(argv[3]) : 100000;
    std::string outputFile = (argc >= 5) ? argv[4] : "thickness_analysis.off";

    try {
        StructuralCaliper caliper(mesh);
        Mesh outMesh;
        for (const auto& res : results) {
            outMesh.vertices.push_back({res.p.x, res.p.y, res.p.z});
            outMesh.vertexColors.push_back(res.color);
        }
        MeshIO::save(outputFile, outMesh);

        fclose(out);

        std::cout << "Successfully saved structural analysis to: " << outputFile << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
