#include <iostream>
#include <vector>
#include <string>
#include "mesh_utils.h"
#include "MeshIO.h"
#include "SampleInterior.h"
#include "argparse/argparse.h"

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("sample_interior", 
        "Generate random sample points inside the mesh volume.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_positional("samples", "Number of sample points to generate");
    parser.add_argument("-o", "Output mesh file (default: interior_samples.off)", "interior_samples.off");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    int numSamples = parser.get_int("samples");
    std::string outputFile = parser.get("o");

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

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