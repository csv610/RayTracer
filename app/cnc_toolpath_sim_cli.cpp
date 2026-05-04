#include "SimulationSuite.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("cnc_toolpath_sim", 
        "Simulate CNC toolpath to detect collisions and accessibility issues.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-r", "Simulation resolution (default: 128)", "128");
    parser.add_argument("-t", "CNC tool radius (default: 2.0)", "2.0");
    parser.add_argument("-o", "Output mesh file (default: cnc_sim.off)", "cnc_sim.off");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    std::string outputFile = parser.get("o");
    int res = parser.get_int("r", 128);
    float toolR = parser.get_float("R", 2.0f);

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    SimulationSuite suite(mesh);
    auto cres = suite.simulateCnc(res, toolR);

    MeshIO::save(outputFile, cres);
    std::cout << "CNC simulation saved to " << outputFile << std::endl;
    return 0;
}