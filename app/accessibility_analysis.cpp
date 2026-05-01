#include "AccessibilityAnalysis.h"
#include <embree4/rtcore.h>
#include <tbb/parallel_for.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <fstream>
#include "mesh_utils.h"
#include "MeshIO.h"

    }
    std::string inputFile = argv[1];
    float toolRadius = (argc >= 3) ? (float)atof(argv[2]) : 2.0f; // Default 2mm tool
    std::string outputFile = (argc >= 4) ? argv[3] : "accessibility.off";

    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) return 1;

    std::cout << "Analyzing 3-axis CNC accessibility with tool radius: " << toolRadius << "..." << std::endl;

    AccessibilityAnalysis analysis(mesh);
    analysis.analyze(toolRadius);

    const auto& triColors = analysis.getTriColors();

    // Save as OFF
    mesh.faceColors = triColors;
    MeshIO::save(outputFile, mesh);

    }

    std::cout << "Analysis complete: " << analysis.getInaccessibleCount() << " triangles are inaccessible." << std::endl;
    std::cout << "Results saved to " << outputFile << std::endl;

    return 0;
}
