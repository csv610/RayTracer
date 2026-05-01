#include "SymmetryDetector.h"
#include <iostream>
#include <cassert>
#include <iomanip>

int main() {
    Mesh sphere;
    int stacks = 32;
    int slices = 64;
    float radius = 1.0f;
    createUVSphere(sphere, stacks, slices, radius);
    std::cout << "Generated UV Sphere: " << sphere.vertices.size() << " vertices, " << sphere.triangles.size() << " triangles." << std::endl;

    SymmetryDetector detector(sphere);
    std::vector<Plane> candidates = detector.findCandidatePlanes();

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Testing symmetry on UV Sphere..." << std::endl;
    
    bool foundGoodSymmetry = false;
    for (size_t i = 0; i < candidates.size(); ++i) {
        float score = detector.checkSymmetry(candidates[i]);
        std::cout << "Plane " << i << " score: " << score << std::endl;
        // For a sphere, the PCA-aligned planes should be highly symmetric
        if (score < 0.001f) {
            foundGoodSymmetry = true;
        }
    }

    if (foundGoodSymmetry) {
        std::cout << "SUCCESS: High symmetry detected for sphere." << std::endl;
        return 0;
    } else {
        std::cout << "FAILURE: Could not detect high symmetry for sphere." << std::endl;
        return 1;
    }
}
