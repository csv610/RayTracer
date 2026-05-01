#include "ManufacturingAnalyzer.h"
#include "MeshIO.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [num_samples]" << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!MeshIO::load(argv[1], mesh)) return 1;

    int samples = (argc >= 3) ? std::atoi(argv[2]) : 64;

    ManufacturingAnalyzer analyzer(mesh);
    Vec3 best = analyzer.findOptimalPartingLine(samples);

    std::cout << "Optimal Pull Direction: (" << best.x << ", " << best.y << ", " << best.z << ")" << std::endl;

    return 0;
}
