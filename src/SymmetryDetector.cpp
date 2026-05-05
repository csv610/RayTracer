#include "SymmetryDetector.h"
#include "MeshGeometry.h"
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <iostream>
#include <algorithm>
#include <cmath>

SymmetryDetector::SymmetryDetector(const Mesh& mesh) : m_mesh(mesh) {
    m_scene.addSharedMesh(m_mesh);
    m_scene.commit();

    MeshGeometry geom(m_mesh);
    m_meshDiagonal = geom.computeAABB().size().length();
    
    preSample(10000);
}

void SymmetryDetector::preSample(int numSamples) {
    SampleSurfacePoints sampler(m_mesh);
    m_preSampledPoints = sampler.sample(numSamples);
}

float SymmetryDetector::checkSymmetry(const Plane& plane) const {
    float epsilon = m_meshDiagonal * 0.001f;
    
    float totalError = tbb::parallel_reduce(
        tbb::blocked_range<size_t>(0, m_preSampledPoints.size()),
        0.0f,
        [&](const tbb::blocked_range<size_t>& r, float init) -> float {
            float localError = init;
            for (size_t i = r.begin(); i != r.end(); ++i) {
                const auto& sp = m_preSampledPoints[i];
                Vec3 p_ref = plane.reflectPoint(sp.p);
                Vec3 n_ref = plane.reflectVector(sp.n);

                Ray ray;
                ray.org = {p_ref.x + n_ref.x * epsilon, p_ref.y + n_ref.y * epsilon, p_ref.z + n_ref.z * epsilon};
                ray.dir = {-n_ref.x, -n_ref.y, -n_ref.z};
                ray.tnear = 0.0f;
                ray.tfar = epsilon * 10.0f;

                Hit hit = RayTracer::intersect(m_scene, ray);

                if (hit.hit) {
                    localError += std::abs(hit.t - epsilon);
                } else {
                    ray.org = {p_ref.x - n_ref.x * epsilon, p_ref.y - n_ref.y * epsilon, p_ref.z - n_ref.z * epsilon};
                    ray.dir = {n_ref.x, n_ref.y, n_ref.z};
                    hit = RayTracer::intersect(m_scene, ray);
                    
                    if (hit.hit) {
                        localError += std::abs(hit.t - epsilon);
                    } else {
                        localError += epsilon * 10.0f;
                    }
                }
            }
            return localError;
        },
        std::plus<float>()
    );

    return totalError / (m_preSampledPoints.size() * m_meshDiagonal);
}

static void eigenSolveSymmetric3x3(float m[3][3], float eigenvalues[3], Vec3 eigenvectors[3]) {
    float p1 = m[0][1]*m[0][1] + m[0][2]*m[0][2] + m[1][2]*m[1][2];
    if (p1 == 0) {
        eigenvalues[0] = m[0][0]; eigenvalues[1] = m[1][1]; eigenvalues[2] = m[2][2];
        eigenvectors[0] = {1,0,0}; eigenvectors[1] = {0,1,0}; eigenvectors[2] = {0,0,1};
        return;
    }
    float q = (m[0][0] + m[1][1] + m[2][2]) / 3.0f;
    float p2 = std::pow(m[0][0]-q, 2) + std::pow(m[1][1]-q, 2) + std::pow(m[2][2]-q, 2) + 2*p1;
    float p = std::sqrt(p2 / 6.0f);
    float B[3][3];
    float invP = 1.0f / p;
    for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) {
        B[i][j] = invP * (m[i][j] - (i==j ? q : 0));
    }
    float detB = B[0][0]*(B[1][1]*B[2][2] - B[1][2]*B[2][1]) - 
                 B[0][1]*(B[1][0]*B[2][2] - B[1][2]*B[2][0]) + 
                 B[0][2]*(B[1][0]*B[2][1] - B[1][1]*B[2][0]);
    float r = detB / 2.0f;
    if (r <= -1) r = -1; if (r >= 1) r = 1;
    float phi = acos(r) / 3.0f;
    eigenvalues[0] = q + 2*p*cos(phi);
    eigenvalues[2] = q + 2*p*cos(phi + (2.0f/3.0f)*M_PI);
    eigenvalues[1] = 3*q - eigenvalues[0] - eigenvalues[2];

    for (int k = 0; k < 3; ++k) {
        float lambda = eigenvalues[k];
        float A[3][3];
        for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) A[i][j] = m[i][j] - (i==j ? lambda : 0);
        
        Vec3 r1 = {A[0][0], A[0][1], A[0][2]};
        Vec3 r2 = {A[1][0], A[1][1], A[1][2]};
        Vec3 r3 = {A[2][0], A[2][1], A[2][2]};

        Vec3 n12 = {r1.y*r2.z - r1.z*r2.y, r1.z*r2.x - r1.x*r2.z, r1.x*r2.y - r1.y*r2.x};
        Vec3 n23 = {r2.y*r3.z - r2.z*r3.y, r2.z*r3.x - r2.x*r3.z, r2.x*r3.y - r2.y*r3.x};
        Vec3 n31 = {r3.y*r1.z - r3.z*r1.y, r3.z*r1.x - r3.x*r1.z, r3.x*r1.y - r3.y*r1.x};

        float l12 = n12.x*n12.x + n12.y*n12.y + n12.z*n12.z;
        float l23 = n23.x*n23.x + n23.y*n23.y + n23.z*n23.z;
        float l31 = n31.x*n31.x + n31.y*n31.y + n31.z*n31.z;

        Vec3 v;
        if (l12 > l23 && l12 > l31) v = n12;
        else if (l23 > l31) v = n23;
        else v = n31;
        
        float norm = v.length();
        if (norm > 0) { v.x /= norm; v.y /= norm; v.z /= norm; }
        else {
            if (k==0) v = {1,0,0};
            else if (k==1) v = {0,1,0};
            else v = {0,0,1};
        }
        eigenvectors[k] = v;
    }
}

std::vector<Plane> SymmetryDetector::findCandidatePlanes() const {
    Vec3 mean = {0,0,0};
    for (const auto& s : m_preSampledPoints) {
        mean.x += s.p.x; mean.y += s.p.y; mean.z += s.p.z;
    }
    mean.x /= m_preSampledPoints.size(); mean.y /= m_preSampledPoints.size(); mean.z /= m_preSampledPoints.size();

    float cov[3][3] = {0};
    for (const auto& s : m_preSampledPoints) {
        float dx = s.p.x - mean.x;
        float dy = s.p.y - mean.y;
        float dz = s.p.z - mean.z;
        cov[0][0] += dx*dx; cov[0][1] += dx*dy; cov[0][2] += dx*dz;
        cov[1][1] += dy*dy; cov[1][2] += dy*dz;
        cov[2][2] += dz*dz;
    }
    cov[1][0] = cov[0][1]; cov[2][0] = cov[0][2]; cov[2][1] = cov[1][2];
    for(int i=0; i<3; ++i) for(int j=0; j<3; ++j) cov[i][j] /= m_preSampledPoints.size();

    float eigenvalues[3];
    Vec3 eigenvectors[3];
    eigenSolveSymmetric3x3(cov, eigenvalues, eigenvectors);

    std::vector<Plane> planes;
    for (int i = 0; i < 3; ++i) {
        Plane p;
        p.normal = eigenvectors[i];
        p.distance = p.normal.x * mean.x + p.normal.y * mean.y + p.normal.z * mean.z;
        planes.push_back(p);
    }
    return planes;
}

SymmetryDetector::Result SymmetryDetector::detectSymmetry() const {
    std::vector<Plane> candidates = findCandidatePlanes();
    Result result;
    result.bestScore = 1e10f;
    result.quality = Quality::NONE;

    for (const auto& p : candidates) {
        float score = checkSymmetry(p);
        result.candidates.push_back({p, score});
        if (score < result.bestScore) {
            result.bestScore = score;
            result.bestPlane = p;
        }
    }

    if (result.bestScore < 0.005f) result.quality = Quality::STRONG;
    else if (result.bestScore < 0.02f) result.quality = Quality::MODERATE;
    else result.quality = Quality::NONE;

    return result;
}
