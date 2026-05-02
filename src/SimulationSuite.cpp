#include "SimulationSuite.h"
#include <tbb/parallel_for.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <fstream>

SimulationSuite::SimulationSuite(const Mesh& mesh) : mesh(mesh) {
    buildScene();
}

SimulationSuite::~SimulationSuite() {}

void SimulationSuite::buildScene() {
    scene.addSharedMesh(mesh);
    scene.commit();
}

Mesh SimulationSuite::voxelize(int res) const {
    AABB bbox; for(const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 size = bbox.size();
    float vSize = std::max({size.x, size.y, size.z}) / res;
    int nx = (int)ceil(size.x/vSize)+2, ny = (int)ceil(size.y/vSize)+2, nz = (int)ceil(size.z/vSize)+2;
    Vec3 minB = {bbox.min.x-vSize, bbox.min.y-vSize, bbox.min.z-vSize};
    std::vector<uint8_t> grid(nx*ny*nz, 0);

    tbb::parallel_for(0, nz, [&](int z) {
        for(int y=0; y<ny; ++y) for(int x=0; x<nx; ++x) {
            Vec3 p = {minB.x + (x+0.5f)*vSize, minB.y + (y+0.5f)*vSize, minB.z + (z+0.5f)*vSize};
            if(scene.isInside(p)) grid[(z*ny+y)*nx+x] = 1;
        }
    });

    Mesh resMesh;
    auto addCube = [&](Vec3 p, float s) {
        unsigned int start = resMesh.vertices.size(); float h = s*0.5f;
        resMesh.vertices.push_back({p.x-h,p.y-h,p.z-h}); resMesh.vertices.push_back({p.x+h,p.y-h,p.z-h});
        resMesh.vertices.push_back({p.x+h,p.y+h,p.z-h}); resMesh.vertices.push_back({p.x-h,p.y+h,p.z-h});
        resMesh.vertices.push_back({p.x-h,p.y-h,p.z+h}); resMesh.vertices.push_back({p.x+h,p.y-h,p.z+h});
        resMesh.vertices.push_back({p.x+h,p.y+h,p.z+h}); resMesh.vertices.push_back({p.x-h,p.y+h,p.z+h});
        resMesh.triangles.push_back({start+0,start+2,start+1}); resMesh.triangles.push_back({start+0,start+3,start+2});
        resMesh.triangles.push_back({start+4,start+5,start+6}); resMesh.triangles.push_back({start+4,start+6,start+7});
        resMesh.triangles.push_back({start+0,start+1,start+5}); resMesh.triangles.push_back({start+0,start+5,start+4});
        resMesh.triangles.push_back({start+1,start+2,start+6}); resMesh.triangles.push_back({start+1,start+6,start+5});
        resMesh.triangles.push_back({start+2,start+3,start+7}); resMesh.triangles.push_back({start+2,start+7,start+6});
        resMesh.triangles.push_back({start+3,start+0,start+4}); resMesh.triangles.push_back({start+3,start+4,start+7});
    };
    for(int z=0; z<nz; ++z) for(int y=0; y<ny; ++y) for(int x=0; x<nx; ++x) if(grid[(z*ny+y)*nx+x]) addCube({minB.x+(x+0.5f)*vSize, minB.y+(y+0.5f)*vSize, minB.z+(z+0.5f)*vSize}, vSize*0.95f);
    return resMesh;
}

void SimulationSuite::slice(int numLayers, int res, const std::string& prefix) const {
    AABB bbox; for(const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 size = bbox.size(); float dT = size.z / numLayers;
    for(int l=0; l<numLayers; ++l) {
        float z = bbox.min.z + (l+0.5f)*dT; std::vector<uint8_t> data(res*res, 0);
        tbb::parallel_for(0, res, [&](int y) {
            for(int x=0; x<res; ++x) {
                Vec3 p = {bbox.min.x+(x+0.5f)*(size.x/res), bbox.min.y+(y+0.5f)*(size.y/res), z};
                if(scene.isInside(p)) data[y*res+x] = 255;
            }
        });
        if(l % std::max(1, numLayers/5) == 0) {
            std::ofstream out(prefix + "_" + std::to_string(l) + ".ppm", std::ios::binary);
            out << "P5\n" << res << " " << res << "\n255\n"; out.write((char*)data.data(), data.size());
        }
    }
}

Mesh SimulationSuite::simulateCnc(int res, float toolRadius) const {
    AABB bbox; for(const auto& v : mesh.vertices) bbox.expand(v);
    Vec3 size = bbox.size(); std::vector<float> targetH(res*res, bbox.min.z), simH(res*res, bbox.min.z);
    tbb::parallel_for(0, res, [&](int y) {
        for(int x=0; x<res; ++x) {
            float px = bbox.min.x+(x+0.5f)*(size.x/res), py = bbox.min.y+(y+0.5f)*(size.y/res);
            Ray ray;
            ray.org = {px, py, bbox.max.z + size.z * 0.1f};
            ray.dir = {0, 0, -1};
            ray.tnear = 0.0f;
            ray.tfar = size.z * 1.5f;
            
            Hit hit = RayTracer::intersect(scene, ray);
            if(hit.hit) targetH[y*res+x] = ray.org.z - hit.t;
        }
    });
    int pR = (int)ceil(toolRadius / (size.x/res));
    tbb::parallel_for(0, res, [&](int y) {
        for(int x=0; x<res; ++x) {
            float maxH = bbox.min.z;
            for(int dy=-pR; dy<=pR; ++dy) for(int dx=-pR; dx<=pR; ++dx) {
                int nx=x+dx, ny=y+dy; if(nx>=0&&nx<res&&ny>=0&&ny<res) {
                    float dS = (dx*dx+dy*dy)*pow(size.x/res, 2); if(dS<=toolRadius*toolRadius) {
                        float off = sqrt(toolRadius*toolRadius - dS) - toolRadius;
                        maxH = std::max(maxH, targetH[ny*res+nx]+off);
                    }
                }
            }
            simH[y*res+x] = maxH;
        }
    });
    Mesh cr;
    for(int y=0; y<res; ++y) for(int x=0; x<res; ++x) {
        cr.vertices.push_back({bbox.min.x+x*(size.x/res), bbox.min.y+y*(size.y/res), simH[y*res+x]});
        float L = simH[y*res+x] - targetH[y*res+x]; cr.vertexColors.push_back(getJetColor(1.0f - std::clamp(L/(toolRadius*0.1f), 0.0f, 1.0f)));
    }
    for(int y=0; y<res-1; ++y) for(int x=0; x<res-1; ++x) {
        unsigned int i0=y*res+x, i1=y*res+x+1, i2=(y+1)*res+x+1, i3=(y+1)*res+x;
        cr.triangles.push_back({i0,i1,i2}); cr.triangles.push_back({i0,i2,i3});
    }
    return cr;
}
