#include "SkyViewFactor.h"
#include "AmbientOcclusion.h"
#include "Mesh.h"
#include "MeshGeometry.h"
#include <iostream>
#include <cassert>
#include <vector>

void test_SkyViewFactor_Sphere() {
    std::cout << "Testing SkyViewFactor with a sphere..." << std::endl;
    Mesh mesh;
    MeshGeometry::createUVSphere(mesh, 20, 20, 1.0f);
    
    SkyViewFactor svf(mesh);
    auto results = svf.compute(32);
    assert(results.size() == mesh.triangles.size());
    
    float avg = 0;
    for (float v : results) avg += v;
    avg /= results.size();
    std::cout << "Average SVF for Sphere: " << avg << std::endl;
    assert(avg > 0.5f);
    std::cout << "SkyViewFactor Sphere test passed!" << std::endl;
}

void test_AmbientOcclusion_Sphere() {
    std::cout << "Testing AmbientOcclusion with a sphere..." << std::endl;
    Mesh mesh;
    MeshGeometry::createUVSphere(mesh, 20, 20, 1.0f);
    
    AmbientOcclusion ao(mesh);
    auto results = ao.compute(32);
    assert(results.size() == mesh.triangles.size());
    
    float avg = 0;
    for (float v : results) avg += v;
    avg /= results.size();
    std::cout << "Average AO for Sphere: " << avg << std::endl;
    assert(avg < 0.5f);
    std::cout << "AmbientOcclusion Sphere test passed!" << std::endl;
}

int main() {
    try {
        test_SkyViewFactor_Sphere();
        test_AmbientOcclusion_Sphere();
        std::cout << "Decomposed analyzer tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
