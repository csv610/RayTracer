#include "SymmetryDetector.h"
#include "MeshIO.h"
#include <iostream>
#include <vector>
#include <iomanip>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mesh.off>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    Mesh mesh;
    if (!MeshIO::load(inputFile, mesh)) {
        std::cerr << "Failed to load mesh: " << inputFile << std::endl;
        return 1;
    }

    std::cout << "Mesh loaded: " << mesh.vertices.size() << " vertices, " << mesh.triangles.size() << " triangles." << std::endl;

    SymmetryDetector detector(mesh);
    std::cout << "Finding candidate symmetry planes..." << std::endl;
    std::vector<Plane> candidates = detector.findCandidatePlanes();

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "\nCandidate Planes (based on Principal Component Analysis):" << std::endl;
    std::cout << "---------------------------------------------------------" << std::endl;
    
    float bestScore = 1e10f;
    Plane bestPlane;

    for (size_t i = 0; i < candidates.size(); ++i) {
        const auto& p = candidates[i];
        std::cout << "Plane " << i << ": Normal(" << p.normal.x << ", " << p.normal.y << ", " << p.normal.z << "), Dist: " << p.distance << std::endl;
        
        float score = detector.checkSymmetry(p);
        std::cout << "  Symmetry Score: " << score << " (lower is better)" << std::endl;

        if (score < bestScore) {
            bestScore = score;
            bestPlane = p;
        }
    }

    std::cout << "\nBest symmetry plane found:" << std::endl;
    std::cout << "Normal: (" << bestPlane.normal.x << ", " << bestPlane.normal.y << ", " << bestPlane.normal.z << ")" << std::endl;
    std::cout << "Distance: " << bestPlane.distance << std::endl;
    std::cout << "Score: " << bestScore << std::endl;

    if (bestScore < 0.005) {
        std::cout << "Result: The mesh exhibits STRONG symmetry with respect to this plane." << std::endl;
    } else if (bestScore < 0.02) {
        std::cout << "Result: The mesh exhibits MODERATE symmetry with respect to this plane." << std::endl;
    } else {
        std::cout << "Result: No strong global symmetry detected among principal axes." << std::endl;
    }

    return 0;
}
