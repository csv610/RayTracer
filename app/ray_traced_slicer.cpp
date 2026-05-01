#include "SimulationSuite.h"
#include "MeshIO.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> <num_layers> [res]" << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!MeshIO::load(argv[1], mesh)) return 1;

    int layers = std::atoi(argv[2]);
    int res = (argc >= 4) ? std::atoi(argv[3]) : 128;

    SimulationSuite suite(mesh);
    suite.slice(layers, res, "slice");

    std::cout << "Slicing complete." << std::endl;
    return 0;
}
