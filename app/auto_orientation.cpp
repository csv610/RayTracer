#include "AutoOrientationOptimizer.h"
#include "MeshIO.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [num_samples]" << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!MeshIO::load(argv[1], mesh)) return 1;

    int samples = (argc >= 3) ? std::atoi(argv[2]) : 32;

    AutoOrientationOptimizer optimizer(mesh);
    auto results = optimizer.optimize(samples);

    std::cout << "\nOptimal Build Orientation Analysis:" << std::endl;
    for (int i = 0; i < std::min(5, (int)results.size()); ++i) {
        std::cout << "Rank " << i+1 << ": UpDirection(" << results[i].up.x << ", " << results[i].up.y << ", " << results[i].up.z 
                  << "), Support Volume: " << results[i].supportVolume << std::endl;
    }

    return 0;
}
