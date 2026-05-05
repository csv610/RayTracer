#include "GeometryAnalyzer.h"
#include "Mesh.h"
#include "MeshGeometry.h"
#include <iostream>
#include <cassert>
#include <vector>

void test_GeometryAnalyzer_Sphere() {
    std::cout << "Testing GeometryAnalyzer with a sphere..." << std::endl;
    Mesh mesh;
    MeshGeometry::createUVSphere(mesh, 20, 20, 1.0f);
    
    GeometryAnalyzer analyzer(mesh);
    
    // For a sphere, AO should be relatively low on the exterior
    auto ao = analyzer.computeAmbientOcclusion(32);
    assert(ao.size() == mesh.triangles.size());
    
    float avgAO = 0;
    for (float v : ao) avgAO += v;
    avgAO /= ao.size();
    std::cout << "Average AO for Sphere: " << avgAO << std::endl;
    // On a convex surface, most rays should escape
    assert(avgAO < 0.5f);

    // SVF should be high for a convex sphere
    auto svf = analyzer.computeSkyViewFactor(32);
    assert(svf.size() == mesh.triangles.size());
    
    float avgSVF = 0;
    for (float v : svf) avgSVF += v;
    avgSVF /= svf.size();
    std::cout << "Average SVF for Sphere: " << avgSVF << std::endl;
    assert(avgSVF > 0.5f);

    // Pocket Exposure should match SVF
    auto pocket = analyzer.computePocketExposure(32);
    for (size_t i = 0; i < svf.size(); ++i) {
        assert(svf[i] == pocket[i]);
    }
    std::cout << "Pocket Exposure matches SVF as expected." << std::endl;
    
    std::cout << "GeometryAnalyzer Sphere test passed!" << std::endl;
}

int main() {
    try {
        test_GeometryAnalyzer_Sphere();
        std::cout << "GeometryAnalyzer tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
