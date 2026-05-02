#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include <vector>
#include "mesh_utils.h"

// Forward declaration to avoid exposing Embree headers in RayTracer.h
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
    Vec3 normal; // Geometric normal
};

class Scene {
public:
    Scene();
    ~Scene();

    unsigned int addMesh(const Mesh& mesh);
    unsigned int addSharedMesh(const Mesh& mesh); // Use shared buffers
    void commit();

    bool isInside(const Vec3& p) const;
    
    // Low-level access if absolutely needed by internal src/ components
    RTCScene getInternalScene() const { return scene; }
    RTCDevice getInternalDevice() const { return device; }

private:
    RTCDevice device;
    RTCScene scene;
};

class RayTracer {
public:
    static Hit intersect(const Scene& scene, const Ray& ray);
    static bool occluded(const Scene& scene, const Ray& ray);
};

#endif // RAY_TRACER_H
