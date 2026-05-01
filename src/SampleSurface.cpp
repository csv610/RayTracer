#include "SampleSurface.h"
#include <tbb/parallel_for.h>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

SampleSurface::SampleSurface(const Mesh& mesh) : mesh(mesh), totalArea(0.0f) {
    device = rtcNewDevice(nullptr);
    scene = rtcNewScene(device);
    buildCDF();
    buildScene();

    // Calculate internal epsilon based on bounding box
    Vec3 minP = {1e20f, 1e20f, 1e20f}, maxP = {-1e20f, -1e20f, -1e20f};
    for (const auto& v : mesh.vertices) {
        minP.x = std::min(minP.x, v.x); minP.y = std::min(minP.y, v.y); minP.z = std::min(minP.z, v.z);
        maxP.x = std::max(maxP.x, v.x); maxP.y = std::max(maxP.y, v.y); maxP.z = std::max(maxP.z, v.z);
    }
    float diag = sqrt(pow(maxP.x-minP.x,2) + pow(maxP.y-minP.y,2) + pow(maxP.z-minP.z,2));
    internalEpsilon = diag * 0.0001f; // 0.01% of diagonal
    if (internalEpsilon < 1e-6f) internalEpsilon = 1e-6f;

    precomputeOrientations();
}

SampleSurface::~SampleSurface() {
    if (scene) rtcReleaseScene(scene);
    if (device) rtcReleaseDevice(device);
}

void SampleSurface::buildCDF() {
    triangleCDF.resize(mesh.triangles.size());
    totalArea = 0.0f;
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        const Triangle& t = mesh.triangles[i];
        Vec3 e1 = {mesh.vertices[t.v1].x - mesh.vertices[t.v0].x, mesh.vertices[t.v1].y - mesh.vertices[t.v0].y, mesh.vertices[t.v1].z - mesh.vertices[t.v0].z};
        Vec3 e2 = {mesh.vertices[t.v2].x - mesh.vertices[t.v0].x, mesh.vertices[t.v2].y - mesh.vertices[t.v0].y, mesh.vertices[t.v2].z - mesh.vertices[t.v0].z};
        Vec3 cross = { e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x };
        totalArea += 0.5f * sqrt(cross.x * cross.x + cross.y * cross.y + cross.z * cross.z);
        triangleCDF[i] = totalArea;
    }
}

void SampleSurface::buildScene() {
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
    memcpy(vb, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
    memcpy(ib, mesh.triangles.data(), mesh.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
    rtcCommitScene(scene);
}

bool SampleSurface::checkInside(const Vec3& p) const {
    RTCRayHit rh;
    rh.ray.org_x = p.x; rh.ray.org_y = p.y; rh.ray.org_z = p.z;
    rh.ray.dir_x = 0.314f; rh.ray.dir_y = 0.718f; rh.ray.dir_z = 0.941f; 
    float len = sqrt(rh.ray.dir_x*rh.ray.dir_x + rh.ray.dir_y*rh.ray.dir_y + rh.ray.dir_z*rh.ray.dir_z);
    rh.ray.dir_x /= len; rh.ray.dir_y /= len; rh.ray.dir_z /= len;
    
    rh.ray.tnear = 0.0f; rh.ray.tfar = 1e10f; rh.ray.mask = -1; rh.ray.time = 0;
    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    RTCIntersectArguments args; rtcInitIntersectArguments(&args);
    
    int intersections = 0;
    while (true) {
        rtcIntersect1(scene, &rh, &args);
        if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) break;
        intersections++;
        rh.ray.tnear = rh.ray.tfar + (internalEpsilon * 0.1f);
        rh.ray.tfar = 1e10f;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    }
    return (intersections % 2 != 0);
}

void SampleSurface::precomputeOrientations() {
    std::cout << "Precomputing face orientations..." << std::endl;
    isNormalOutward.resize(mesh.triangles.size());
    tbb::parallel_for(size_t(0), mesh.triangles.size(), [&](size_t i) {
        const Triangle& t = mesh.triangles[i];
        Vec3 center = computeFaceCenter(mesh.vertices[t.v0], mesh.vertices[t.v1], mesh.vertices[t.v2]);
        Vec3 normal = computeFaceNormal(mesh.vertices[t.v0], mesh.vertices[t.v1], mesh.vertices[t.v2]);
        
        Vec3 p_test = { center.x + normal.x * internalEpsilon, 
                        center.y + normal.y * internalEpsilon, 
                        center.z + normal.z * internalEpsilon };
        
        isNormalOutward[i] = !checkInside(p_test);
    });
}

std::vector<SampledPoint> SampleSurface::sample(int numSamples, int side) const {
    size_t numTris = mesh.triangles.size();
    size_t totalToSample = std::max((size_t)numSamples, numTris);
    std::vector<SampledPoint> points(totalToSample);

    std::cout << "Generating " << totalToSample << " surface samples with normals..." << std::endl;
    tbb::parallel_for(size_t(0), totalToSample, [&](size_t i) {
        thread_local std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dis(0.0f, 1.0f);

        size_t triIdx = (i < numTris) ? i : (std::distance(triangleCDF.begin(), std::lower_bound(triangleCDF.begin(), triangleCDF.end(), dis(gen) * totalArea)));
        if (triIdx >= numTris) triIdx = numTris - 1;

        const Triangle& t = mesh.triangles[triIdx];
        const Vertex& v0 = mesh.vertices[t.v0]; const Vertex& v1 = mesh.vertices[t.v1]; const Vertex& v2 = mesh.vertices[t.v2];

        float r1 = sqrt(dis(gen)), r2 = dis(gen);
        float u = 1.0f - r1, v = r2 * r1, w = 1.0f - u - v;
        
        Vec3 p = { u*v0.x + v*v1.x + w*v2.x, u*v0.y + v*v1.y + w*v2.y, u*v0.z + v*v1.z + w*v2.z };
        Vec3 normal = computeFaceNormal(v0, v1, v2);

        // Ensure normal points outward based on precomputed orientation
        if (!isNormalOutward[triIdx]) {
            normal.x = -normal.x; normal.y = -normal.y; normal.z = -normal.z;
        }

        if (side != 0) {
            float s = (float)side;
            p.x += normal.x * s * internalEpsilon;
            p.y += normal.y * s * internalEpsilon;
            p.z += normal.z * s * internalEpsilon;
        }
        
        points[i] = {p, normal};
    });
    return points;
}
