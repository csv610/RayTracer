#include "ThicknessAnalyzer.h"
#include <tbb/parallel_for.h>
#include <cmath>
#include <algorithm>

ThicknessAnalyzer::ThicknessAnalyzer(const Mesh& mesh) : mesh(mesh) {
    buildScene();
    AABB bbox;
    for (const auto& v : mesh.vertices) bbox.expand(v);
    meshDiag = bbox.size().length();
}

ThicknessAnalyzer::~ThicknessAnalyzer() {}

void ThicknessAnalyzer::buildScene() {
    scene.addSharedMesh(mesh);
    scene.commit();
}

ThicknessAnalyzer::Result ThicknessAnalyzer::computeProjectedThickness() const {
    Result res;
    size_t nv = mesh.vertices.size();
    res.thickness.resize(nv, 0.0f);
    
    std::vector<Vec3> vtxNormals(nv, {0,0,0});
    for(const auto& tri : mesh.triangles) {
        Vec3 n = computeFaceNormal(mesh.vertices[tri.v0], mesh.vertices[tri.v1], mesh.vertices[tri.v2]);
        vtxNormals[tri.v0].x += n.x; vtxNormals[tri.v0].y += n.y; vtxNormals[tri.v0].z += n.z;
        vtxNormals[tri.v1].x += n.x; vtxNormals[tri.v1].y += n.y; vtxNormals[tri.v1].z += n.z;
        vtxNormals[tri.v2].x += n.x; vtxNormals[tri.v2].y += n.y; vtxNormals[tri.v2].z += n.z;
    }
    for(auto& n : vtxNormals) {
        float l = n.length();
        if(l > 0) { n.x /= l; n.y /= l; n.z /= l; }
    }

    float epsilon = meshDiag * 1e-4f;

    tbb::parallel_for(size_t(0), nv, [&](size_t i) {
        const auto& p = mesh.vertices[i];
        const auto& n = vtxNormals[i];
        
        Ray ray;
        ray.org = {p.x - n.x * epsilon, p.y - n.y * epsilon, p.z - n.z * epsilon};
        ray.dir = {-n.x, -n.y, -n.z};
        ray.tnear = 0.0f;
        ray.tfar = meshDiag;
        
        Hit hit = RayTracer::intersect(scene, ray);
        if (hit.hit) {
            res.thickness[i] = hit.t;
        } else {
            res.thickness[i] = 0.0f;
        }
    });

    res.maxThickness = *std::max_element(res.thickness.begin(), res.thickness.end());
    return res;
}

Mesh ThicknessAnalyzer::Result::getColoredMesh(const Mesh& original) const {
    Mesh m = original;
    m.vertexColors.resize(m.vertices.size());
    for (size_t i = 0; i < m.vertices.size(); ++i) {
        float t = (maxThickness > 0) ? thickness[i] / maxThickness : 0;
        m.vertexColors[i] = getJetColor(t);
    }
    return m;
}
