#include "AccessibilityAnalysis.h"
#include "MeshIO.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off> [tool_radius] [output.off]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    float toolRadius = (argc >= 3) ? (float)atof(argv[2]) : 2.0f;
    std::string outputFile = (argc >= 4) ? argv[3] : "accessibility.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    std::cout << "Analyzing 3-axis CNC accessibility with tool radius: " << toolRadius << "..." << std::endl;

    AccessibilityAnalysis analysis(mesh);
    analysis.analyze(toolRadius);

    mesh.faceColors = analysis.getTriColors();
    MeshIO::save(outputFile, mesh);

    std::cout << "Analysis complete: " << analysis.getInaccessibleCount() << " triangles are inaccessible." << std::endl;
    std::cout << "Results saved to " << outputFile << std::endl;

    return 0;
}
