#include "AccessibilityAnalysis.h"
#include "MeshIO.h"
#include "argparse/argparse.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    argparse::ArgumentParser parser("accessibility_analysis", 
        "Analyze 3-axis CNC machining accessibility considering tool radius.");

    parser.add_positional("input", "Input mesh file (OFF/PLY format)");
    parser.add_argument("-o", "Output mesh file (default: accessibility.off)", "accessibility.off");
    parser.add_argument("-r", "CNC tool radius (default: 2.0)", "2.0");

    try {
        parser.parse(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::string inputFile = parser.get("input");
    std::string outputFile = parser.get("o");
    float toolRadius = parser.get_float("tool-radius", 2.0f);

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