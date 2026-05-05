#include "AutoOrientationOptimizer.h"
#include "Renderer.h"
#include "ImageUtils.h"
#include "MeshGeometry.h"
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <random>

AutoOrientationOptimizer::AutoOrientationOptimizer(const Mesh& mesh) : mesh(mesh) {
    buildScene();
    MeshGeometry geom(mesh);
    AABB bbox = geom.computeAABB();
    meshDiag = bbox.size().length();
}

AutoOrientationOptimizer::~AutoOrientationOptimizer() {}

void AutoOrientationOptimizer::buildScene() {
    scene.addSharedMesh(mesh);
    scene.commit();
}

float AutoOrientationOptimizer::calculateSupportVolume(const Quaternion& q) const {
    Vec3 upDir = q.rotate(Vec3{0, 0, 1});
    float criticalCos = cos(135.0f * M_PI / 180.0f);
    float epsilon = meshDiag * 1e-4f;
    double totalVol = tbb::parallel_reduce(tbb::blocked_range<size_t>(0, mesh.triangles.size()), 0.0, [&](const auto& r, double init) {
        for(size_t i=r.begin(); i!=r.end(); ++i) {
            Vec3 n = MeshGeometry::computeFaceNormal(mesh.nodes[mesh.triangles[i].v0], mesh.nodes[mesh.triangles[i].v1], mesh.nodes[mesh.triangles[i].v2]);
            if (n.x*upDir.x + n.y*upDir.y + n.z*upDir.z < criticalCos) {
                Vec3 center = MeshGeometry::computeFaceCenter(mesh.nodes[mesh.triangles[i].v0], mesh.nodes[mesh.triangles[i].v1], mesh.nodes[mesh.triangles[i].v2]);
                Ray ray;
                ray.org = {center.x - n.x * epsilon, center.y - n.y * epsilon, center.z - n.z * epsilon};
                ray.dir = {-upDir.x, -upDir.y, -upDir.z};
                ray.tnear = 0.0f;
                ray.tfar = meshDiag * 2.0f;

                Hit hit = RayTracer::intersect(scene, ray);
                float dist = hit.hit ? hit.t : meshDiag;
                init += (double)dist * MeshGeometry::computeFaceArea(mesh.nodes[mesh.triangles[i].v0], mesh.nodes[mesh.triangles[i].v1], mesh.nodes[mesh.triangles[i].v2]);
            }
        }
        return init;
    }, std::plus<double>());
    return (float)totalVol;
}

std::vector<AutoOrientationOptimizer::Result> AutoOrientationOptimizer::optimize(int numSamples) const {
    std::vector<Result> results;
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for(int s=0; s<numSamples; ++s) {
        Quaternion q = Quaternion::random(dist(gen), dist(gen), dist(gen));
        results.push_back({q, calculateSupportVolume(q)});
    }
    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) { return a.supportVolume < b.supportVolume; });
    return results;
}

void AutoOrientationOptimizer::saveOptimalViewPNG(const Result& res, const char* filename) const {
    Node center;
    float radius;
    MeshGeometry geom(mesh);
    geom.computeBoundingSphere(center, radius);

    // Use the orientation to determine the camera setup
    Vec3 up = res.orientation.rotate(Vec3{0, 0, 1});
    Vec3 forwardCandidate = res.orientation.rotate(Vec3{0, 1, 0});
    Vec3 rightCandidate = res.orientation.rotate(Vec3{1, 0, 0});

    // Camera at an angle
    float dist = radius * 3.0f;
    Renderer::Camera cam;
    cam.pos = {
        center.x + (up.x + forwardCandidate.x + rightCandidate.x) * dist * 0.577f,
        center.y + (up.y + forwardCandidate.y + rightCandidate.y) * dist * 0.577f,
        center.z + (up.z + forwardCandidate.z + rightCandidate.z) * dist * 0.577f
    };
    cam.target = {center.x, center.y, center.z};
    cam.up = up;
    cam.fov = 40.0f;
    cam.width = 1024;
    cam.height = 1024;

    std::vector<Vec3> pixels = Renderer::renderSimpleLighting(scene, cam, {1.0f, 1.0f, 1.2f});
    std::vector<unsigned char> rgb(cam.width * cam.height * 3);
    
    for (int i = 0; i < cam.width * cam.height; ++i) {
        rgb[i * 3 + 0] = (unsigned char)(std::min(1.0f, std::max(0.0f, pixels[i].x)) * 255.0f);
        rgb[i * 3 + 1] = (unsigned char)(std::min(1.0f, std::max(0.0f, pixels[i].y)) * 255.0f);
        rgb[i * 3 + 2] = (unsigned char)(std::min(1.0f, std::max(0.0f, pixels[i].z)) * 255.0f);
    }

    ImageUtils::savePNG(filename, cam.width, cam.height, rgb.data());
}
