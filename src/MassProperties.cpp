#include "MassProperties.h"
#include "MeshGeometry.h"
#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>
#include <iostream>
#include <algorithm>
#include <cmath>

MassProperties::MassProperties(const Mesh& mesh) : m_mesh(mesh) {
    m_scene.addSharedMesh(m_mesh);
    m_scene.commit();
}

struct AccumMass {
    double v=0, cx=0, cy=0, cz=0, ixx=0, iyy=0, izz=0, ixy=0, iyz=0, izx=0;
    AccumMass() = default;
};

MassProperties::Properties MassProperties::compute(int res) const {
    MeshGeometry geom(m_mesh);
    AABB bbox = geom.computeAABB();
    Vec3 size = bbox.size();
    float diag = size.length();

    double dx = size.x / res, dy = size.y / res;
    double areaStep = dx * dy;

    AccumMass total = tbb::parallel_reduce(tbb::blocked_range<int>(0, res), AccumMass(), [&](const tbb::blocked_range<int>& r, AccumMass a) {
        for (int i = r.begin(); i != r.end(); ++i) {
            for (int j = 0; j < res; ++j) {
                double x = bbox.min.x + (i + 0.5) * dx;
                double y = bbox.min.y + (j + 0.5) * dy;
                
                Vec3 org = {(float)x, (float)y, bbox.min.z - diag * 0.1f};
                Vec3 dir = {0, 0, 1.0f};
                
                // Centralized logic to find all intersection distances
                std::vector<float> hits = m_scene.findAllIntersections(org, dir, diag * 1.2f);
                
                if (hits.size() >= 2) {
                    std::sort(hits.begin(), hits.end());
                    for (size_t k = 0; k + 1 < hits.size(); k += 2) {
                        double zS = org.z + hits[k], zE = org.z + hits[k+1], L = zE - zS;
                        double dV = L * areaStep;
                        a.v += dV; a.cx += x * dV; a.cy += y * dV; a.cz += (zS+zE)*0.5 * dV;
                        a.ixx += (y*y*L + (zE*zE*zE - zS*zS*zS)/3.0)*areaStep;
                        a.iyy += (x*x*L + (zE*zE*zE - zS*zS*zS)/3.0)*areaStep;
                        a.izz += (x*x + y*y)*L*areaStep;
                        a.ixy += x*y*L*areaStep; a.iyz += y*(zE*zE - zS*zS)*0.5*areaStep; a.izx += x*(zE*zE - zS*zS)*0.5*areaStep;
                    }
                }
            }
        }
        return a;
    }, [](AccumMass a, AccumMass b) {
        a.v+=b.v; a.cx+=b.cx; a.cy+=b.cy; a.cz+=b.cz;
        a.ixx+=b.ixx; a.iyy+=b.iyy; a.izz+=b.izz; a.ixy+=b.ixy; a.iyz+=b.iyz; a.izx+=b.izx;
        return a;
    });

    Properties p;
    p.volume = total.v;
    if (p.volume > 0) {
        p.centerOfMass = {(float)(total.cx/total.v), (float)(total.cy/total.v), (float)(total.cz/total.v)};
        double mx = p.centerOfMass.x, my = p.centerOfMass.y, mz = p.centerOfMass.z;
        p.inertiaTensor[0][0] = total.ixx - total.v*(my*my + mz*mz);
        p.inertiaTensor[1][1] = total.iyy - total.v*(mx*mx + mz*mz);
        p.inertiaTensor[2][2] = total.izz - total.v*(mx*mx + my*my);
        double ixy = total.ixy - total.v*mx*my, iyz = total.iyz - total.v*my*mz, izx = total.izx - total.v*mz*mx;
        p.inertiaTensor[0][1] = p.inertiaTensor[1][0] = -ixy;
        p.inertiaTensor[1][2] = p.inertiaTensor[2][1] = -iyz;
        p.inertiaTensor[0][2] = p.inertiaTensor[2][0] = -izx;
    }
    return p;
}
