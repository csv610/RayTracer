#include "Mesh.h"
#include "MeshGeometry.h"
#include "MeshVoxelizer.h"
#include "RayTracedSlicer.h"
#include "MaterialRemover.h"
#include "MassProperties.h"
#include "MeshIO.h"
#include <iostream>
#include <cassert>
#include <filesystem>

void test_MeshVoxelizer() {
    std::cout << "Testing MeshVoxelizer..." << std::endl;
    Mesh mesh;
    MeshGeometry::createUVSphere(mesh, 10, 10, 1.0f);
    
    MeshVoxelizer voxelizer(mesh);
    Mesh voxelMesh = voxelizer.generateVoxelMesh(0.2f);
    
    assert(!voxelMesh.nodes.empty());
    assert(!voxelMesh.triangles.empty());
    std::cout << "MeshVoxelizer passed! (Generated " << voxelMesh.nodes.size() << " nodes)" << std::endl;
}

void test_MaterialRemover() {
    std::cout << "Testing MaterialRemover..." << std::endl;
    Mesh mesh;
    MeshGeometry::createUVSphere(mesh, 10, 10, 1.0f);
    
    MaterialRemover remover(mesh);
    Mesh cutMesh = remover.simulateCnc(32, 0.1f);
    
    assert(!cutMesh.nodes.empty());
    assert(cutMesh.nodeColors.size() == cutMesh.nodes.size());
    std::cout << "MaterialRemover passed!" << std::endl;
}

void test_MassProperties() {
    std::cout << "Testing MassProperties..." << std::endl;
    Mesh mesh;
    MeshGeometry::createUVSphere(mesh, 20, 20, 1.0f);
    
    MassProperties mp(mesh);
    auto props = mp.compute(64);
    
    // Analytic volume of sphere R=1 is 4/3 * PI ~= 4.188
    std::cout << "Calculated Volume: " << props.volume << " (Target ~4.188)" << std::endl;
    assert(props.volume > 3.5 && props.volume < 4.5);
    
    // Center of mass should be near origin
    assert(std::abs(props.centerOfMass.x) < 0.1f);
    assert(std::abs(props.centerOfMass.y) < 0.1f);
    assert(std::abs(props.centerOfMass.z) < 0.1f);
    
    std::cout << "MassProperties passed!" << std::endl;
}

void test_BatchSlicing() {
    std::cout << "Testing Batch Slicing..." << std::endl;
    Mesh mesh;
    MeshGeometry::createUVSphere(mesh, 10, 10, 1.0f);
    
    RayTracedSlicer slicer(mesh);
    std::string prefix = "test_batch_slice";
    slicer.sliceBatch(10, 64, prefix);
    
    // Check if at least the first slice was saved (sliceBatch saves every 5th or so)
    if (std::filesystem::exists(prefix + "_0.ppm")) {
        std::cout << "Batch Slicing passed!" << std::endl;
        // Cleanup
        for (int i=0; i<10; ++i) {
            std::string f = prefix + "_" + std::to_string(i) + ".ppm";
            if (std::filesystem::exists(f)) std::filesystem::remove(f);
        }
    } else {
        std::cerr << "Batch Slicing failed to produce files!" << std::endl;
        exit(1);
    }
}

int main() {
    try {
        test_MeshVoxelizer();
        test_MaterialRemover();
        test_MassProperties();
        test_BatchSlicing();
        std::cout << "All system component tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
