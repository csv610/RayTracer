#include "SimulationSuite.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("mesh_voxelizer", 
        "Convert mesh into a voxel grid representation.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-r", "Voxel grid resolution (default: 64)", "64");
    parser.add_argument("-o", "Output mesh file (default: voxels.off)", "voxels.off");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    std::string outputFile = parser.get("o");
    int res = parser.get_int("resolution", 64);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    SimulationSuite suite(mesh);
    auto vres = suite.voxelize(res);

    MeshIO::save(outputFile, vres);
    std::cout << "Voxelization saved to " << outputFile << std::endl;
    return 0;
}