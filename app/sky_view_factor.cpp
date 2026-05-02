#include "GeometryAnalyzer.h"
#include "MeshIO.h"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [samples] [output.off]" << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!MeshIO::load(argv[1], mesh)) return 1;

    int samples = (argc >= 3) ? std::atoi(argv[2]) : 128;
    GeometryAnalyzer analyzer(mesh);
    auto skyView = analyzer.computeSkyViewFactor(samples);

    std::string outF = (argc >= 4) ? argv[3] : "sky_view.off";
    mesh.faceColors.resize(skyView.size());
    for(size_t i = 0; i < skyView.size(); ++i) mesh.faceColors[i] = {skyView[i], skyView[i], skyView[i]};
    
    MeshIO::save(outF, mesh);
    std::cout << "Sky View analysis complete. Saved to " << outF << std::endl;
    return 0;
}
