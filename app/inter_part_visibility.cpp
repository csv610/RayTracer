#include "AssemblyAnalyzer.h"
#include "MeshIO.h"
#include <iostream>
#include <fstream>

int main(int argc, char** argv) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <target.off> <env.off> <vx vy vz> [out.off]" << std::endl;
        return 1;
    }

    Mesh target, env;
    if (!MeshIO::load(argv[1], target) || !MeshIO::load(argv[2], env)) return 1;

    Vec3 v = {(float)atof(argv[3]), (float)atof(argv[4]), (float)atof(argv[5])};
    std::string outF = (argc >= 7) ? argv[6] : "visibility.off";

    AssemblyAnalyzer analyzer(target, env);
    auto colors = analyzer.analyzeVisibility(v);
    target.faceColors = colors;
    MeshIO::save(outF, target);

            << " " << colors[i].x << " " << colors[i].y << " " << colors[i].z << "\n";
    }
    std::cout << "Visibility analysis saved to " << outF << std::endl;

    return 0;
}
