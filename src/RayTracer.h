#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include <vector>
#include "Mesh.h"

struct RTCSceneTy;
typedef struct RTCSceneTy* RTCScene;
struct RTCDeviceTy;
typedef struct RTCDeviceTy* RTCDevice;

struct Ray {
    Vec3 org;
    Vec3 dir;
    float tnear = 0.0f;
    float tfar = 1e10f;
};

struct Hit {
    bool hit = false;
    float t = 1e10f;
    unsigned int geomID = -1;
    unsigned int primID = -1;
    Vec3 normal;
};

/**
 * @class Scene
 * @brief Manages a collection of geometric objects for ray-tracing.
 * 
 * The Scene class serves as a wrapper for Embree's RTCScene. It handles 
 * the lifecycle of geometry buffers, manages spatial acceleration 
 * structure builds (commit), and provides high-level queries like 
 * point-in-mesh tests and intersection counting.
 */
class Scene {
public:
    Scene();
    ~Scene();

    unsigned int addMesh(const Mesh& mesh);
    unsigned int addSharedMesh(const Mesh& mesh);
    void commit();

    bool isInside(const Vec3& p, const Vec3& dir = {0,0,1}) const;
    int countIntersections(const Vec3& org, const Vec3& dir, float tmax) const;
    std::vector<float> findAllIntersections(const Vec3& org, const Vec3& dir, float tmax) const;
    
    RTCScene getInternalScene() const { return scene; }
    RTCDevice getInternalDevice() const { return device; }

private:
    RTCDevice device;
    RTCScene scene;
};

/**
 * @class RayTracer
 * @brief Static utility for fundamental ray-mesh intersection operations.
 * 
 * This class provides a simplified interface to Embree's intersection and 
 * occlusion kernels. It abstracts the complexity of RTCIntersectArguments 
 * and RTCRayHit, returning clean Hit results or boolean occlusion states.
 */
class RayTracer {
public:
    static Hit intersect(const Scene& scene, const Ray& ray);
    static bool occluded(const Scene& scene, const Ray& ray);
};

#endif
