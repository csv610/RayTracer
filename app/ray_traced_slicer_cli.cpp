#include "SimulationSuite.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("ray_traced_slicer", 
        "Slice mesh into layers for 3D printing using ray tracing.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_positional("layers", "Number of layers to generate");
    parser.add_argument("-r", "Slicing resolution (default: 128)", "128");
    parser.add_argument("-o", "Output file prefix (default: slice)", "slice");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    int layers = parser.get_int("layers");
    int res = parser.get_int("resolution", 128);
    std::string prefix = parser.get("prefix");

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    SimulationSuite suite(mesh);
    suite.slice(layers, res, prefix);

    std::cout << "Slicing complete." << std::endl;
    return 0;
}
