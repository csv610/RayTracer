#include "GeometryAnalyzer.h"
#include "MeshIO.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [output.off] [samples]" << std::endl;
        return 1;
    }

    Mesh mesh;
    if (!MeshIO::load(argv[1], mesh)) return 1;

    int samples = (argc >= 4) ? std::atoi(argv[3]) : 64;
    GeometryAnalyzer analyzer(mesh);
    auto ao = analyzer.computeAmbientOcclusion(samples);

    std::string outF = (argc >= 3) ? argv[2] : "ao_biked.off";
    std::string outF = (argc >= 3) ? argv[2] : "ao_biked.off";
    mesh.faceColors.resize(ao.size());
    for(size_t i=0; i<ao.size(); ++i) mesh.faceColors[i] = {ao[i], ao[i], ao[i]};
    MeshIO::save(outF, mesh);

    }
    std::cout << "AO baking complete. Saved to " << outF << std::endl;
    return 0;
}
