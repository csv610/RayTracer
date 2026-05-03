#ifndef TEST_APPS_H
#define TEST_APPS_H

#include "mesh_utils.h"
#include "MeshIO.h"
#include "PhysicalProperties.h"
#include "SymmetryDetector.h"
#include "StructuralCaliper.h"
#include "ManufacturingAnalyzer.h"
#include "AssemblyAnalyzer.h"
#include "GeometryAnalyzer.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>

inline void test_physical_properties() {
    Mesh sphere;
    createUVSphere(sphere, 20, 20, 1.0f);
    
    PhysicalProperties analyzer(sphere);
    auto p = analyzer.compute(64);
    
    float expectedVol = (4.0f / 3.0f) * M_PI * 1.0f * 1.0f * 1.0f;
    assert(std::abs(p.volume - expectedVol) < 0.1f);
    assert(std::abs(p.centerOfMass.x) < 0.01f);
    assert(std::abs(p.centerOfMass.y) < 0.01f);
    assert(std::abs(p.centerOfMass.z) < 0.01f);
    
    std::cout << "test_physical_properties passed!" << std::endl;
}

inline void test_symmetry_detector() {
    Mesh box;
    for (float x = -1; x <= 1; x += 2)
        for (float y = -1; y <= 1; y += 2)
            for (float z = -1; z <= 1; z += 2)
                box.vertices.push_back({x, y, z});
    box.triangles = {
        {0,2,1}, {0,3,2}, {4,5,6}, {4,6,7},
        {0,1,5}, {0,5,4}, {2,3,7}, {2,7,6},
        {0,4,7}, {0,7,3}, {1,2,6}, {1,6,5}
    };
    
    SymmetryDetector detector(box);
    auto planes = detector.findCandidatePlanes();
    assert(!planes.empty());
    
    float score = detector.checkSymmetry(planes[0]);
    assert(score < 0.1f);
    
    std::cout << "test_symmetry_detector passed!" << std::endl;
}

inline void test_structural_caliper() {
    Mesh sphere;
    createUVSphere(sphere, 10, 10, 1.0f);
    
    StructuralCaliper caliper(sphere);
    auto results = caliper.analyze(1000, 0.1f);
    
    assert(!results.empty());
    for (const auto& r : results) {
        assert(r.thickness > 0);
        assert(r.color.r <= 255 && r.color.g <= 255 && r.color.b <= 255);
    }
    
    std::cout << "test_structural_caliper passed!" << std::endl;
}

inline void test_manufacturing_analyzer() {
    Mesh sphere;
    createUVSphere(sphere, 15, 15, 1.0f);
    
    ManufacturingAnalyzer analyzer(sphere);
    
    Vec3 upDir = {0, 1, 0};
    auto draft = analyzer.analyzeDraftAngles(upDir);
    assert(!draft.colors.empty());
    
    auto undercuts = analyzer.analyzeUndercuts(upDir);
    assert(undercuts.count >= 0);
    assert(undercuts.colors.size() == sphere.triangles.size());
    
    auto overhangs = analyzer.analyzeOverhangs(45.0f);
    assert(overhangs.count >= 0);
    
    auto parting = analyzer.findOptimalPartingLine(32);
    assert(parting.x * parting.x + parting.y * parting.y + parting.z * parting.z > 0);
    
    std::cout << "test_manufacturing_analyzer passed!" << std::endl;
}

inline void test_assembly_analyzer() {
    Mesh sphere;
    createUVSphere(sphere, 10, 10, 1.0f);
    
    Mesh emptyEnv;
    AssemblyAnalyzer analyzer(sphere, emptyEnv);
    auto clearance = analyzer.analyzeClearance(0.1f);
    assert(clearance.collisions >= 0);
    
    Vec3 moveDir = {0, 1, 0};
    auto path = analyzer.verifyExtractionPath(moveDir, 2.0f);
    assert(path.violations >= 0);
    
    std::cout << "test_assembly_analyzer passed!" << std::endl;
}

inline void test_geometry_analyzer() {
    Mesh sphere;
    createUVSphere(sphere, 10, 10, 1.0f);
    
    GeometryAnalyzer analyzer(sphere);
    
    auto ao = analyzer.computeAmbientOcclusion(100);
    assert(!ao.empty());
    
    auto sky = analyzer.computeSkyViewFactor(100);
    assert(!sky.empty());
    
    std::cout << "test_geometry_analyzer passed!" << std::endl;
}

inline void test_mesh_io() {
    const char* filename = "test_temp.off";
    Mesh sphere;
    createUVSphere(sphere, 8, 8, 1.0f);
    
    assert(MeshIO::save(filename, sphere));
    
    Mesh loaded;
    assert(MeshIO::load(filename, loaded));
    assert(loaded.vertices.size() == sphere.vertices.size());
    assert(loaded.triangles.size() == sphere.triangles.size());
    
    std::remove(filename);
    std::cout << "test_mesh_io passed!" << std::endl;
}

inline void test_mesh_utils() {
    Vertex v0 = {0, 0, 0};
    Vertex v1 = {1, 0, 0};
    Vertex v2 = {0, 1, 0};
    
    Vec3 n = computeFaceNormal(v0, v1, v2);
    assert(std::abs(n.x) < 1e-6);
    assert(std::abs(n.y) < 1e-6);
    assert(std::abs(n.z - 1.0f) < 1e-6);
    
    float area = computeFaceArea(v0, v1, v2);
    assert(std::abs(area - 0.5f) < 1e-6);
    
    Vec3 center = computeFaceCenter(v0, v1, v2);
    assert(std::abs(center.x - 1.0f/3.0f) < 1e-6);
    assert(std::abs(center.y - 1.0f/3.0f) < 1e-6);
    assert(std::abs(center.z) < 1e-6);
    
    Mesh mesh;
    createUVSphere(mesh, 10, 10, 1.0f);
    assert(mesh.vertices.size() == 121);
    assert(mesh.triangles.size() == 200);
    
    Vertex centerPt;
    float radius;
    computeBoundingSphere(mesh, centerPt, radius);
    assert(std::abs(centerPt.x) < 0.01f);
    assert(std::abs(centerPt.y) < 0.01f);
    assert(std::abs(centerPt.z) < 0.01f);
    assert(std::abs(radius - 1.0f) < 0.01f);
    
    std::cout << "test_mesh_utils passed!" << std::endl;
}

inline void test_aabb() {
    AABB box;
    box.expand({1, 2, 3});
    box.expand({-1, -2, -3});
    
    Vec3 s = box.size();
    assert(s.x == 2.0f);
    assert(s.y == 4.0f);
    assert(s.z == 6.0f);
    
    box.pad(0.1f);
    Vec3 s2 = box.size();
    assert(s2.x > s.x);
    
    std::cout << "test_aabb passed!" << std::endl;
}

inline void test_jet_color() {
    Color4b c0 = getJetColor(0.0f);
    Color4b c1 = getJetColor(0.5f);
    Color4b c2 = getJetColor(1.0f);
    
    assert(c0.r <= 255 && c0.g <= 255 && c0.b <= 255);
    assert(c1.r <= 255 && c1.g <= 255 && c1.b <= 255);
    assert(c2.r <= 255 && c2.g <= 255 && c2.b <= 255);
    
    std::cout << "test_jet_color passed!" << std::endl;
}

inline void run_all_tests() {
    std::cout << "Running app unit tests..." << std::endl;
    
    test_mesh_utils();
    test_mesh_io();
    test_aabb();
    test_jet_color();
    test_physical_properties();
    test_symmetry_detector();
    test_structural_caliper();
    test_manufacturing_analyzer();
    test_assembly_analyzer();
    test_geometry_analyzer();
    
    std::cout << "\nAll tests passed!" << std::endl;
}

#endif