#include "RayTracer.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <random>
#include "mesh_utils.h"

int main(int argc, char** argv) {
    int stacks = 32;
    int slices = 64;
    int numRays = 1000000;
    
    if (argc >= 2) numRays = atoi(argv[1]);
    if (argc >= 3) stacks = atoi(argv[2]);
    if (argc >= 4) slices = atoi(argv[3]);

    printf("Sphere Benchmark: %d stacks, %d slices, %d rays\n", stacks, slices, numRays);

    Mesh mesh;
    createUVSphere(mesh, stacks, slices, 1.0f);

    Scene scene;
    scene.addMesh(mesh);
    scene.commit();

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> distTheta(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> distPhi(0.0f, M_PI);
    std::uniform_real_distribution<float> distDist(2.0f, 10.0f);

    std::vector<Ray> rays(numRays);
    for (int i = 0; i < numRays; ++i) {
        float theta = distTheta(rng);
        float phi = distPhi(rng);
        float dist = distDist(rng);
        
        rays[i].org = {dist * sin(phi) * cos(theta), dist * sin(phi) * sin(theta), dist * cos(phi)};
        rays[i].dir = {-rays[i].org.x, -rays[i].org.y, -rays[i].org.z};
        float len = rays[i].dir.length();
        if (len > 0) { rays[i].dir.x /= len; rays[i].dir.y /= len; rays[i].dir.z /= len; }
        
        rays[i].tnear = 0.0f;
        rays[i].tfar = 1e10f;
    }

    auto start = std::chrono::high_resolution_clock::now();
    int hitCount = 0;
    for (int i = 0; i < numRays; ++i) {
        Hit hit = RayTracer::intersect(scene, rays[i]);
        if (hit.hit) hitCount++;
    }
    auto end = std::chrono::high_resolution_clock::now();

    double elapsed = std::chrono::duration<double>(end - start).count();
    printf("Time: %.3f seconds\n", elapsed);
    printf("Rays/sec: %.0f\n", numRays / elapsed);
    printf("Hits: %d / %d (%.1f%%)\n", hitCount, numRays, 100.0 * hitCount / numRays);

    return 0;
}
