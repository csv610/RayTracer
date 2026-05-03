#include "RayTracer.h"
#include <embree4/rtcore.h>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <limits>

Scene::Scene() {
    device = rtcNewDevice(nullptr);
    scene = rtcNewScene(device);
}

Scene::~Scene() {
    if (scene) rtcReleaseScene(scene);
    if (device) rtcReleaseDevice(device);
}

unsigned int Scene::addMesh(const Mesh& mesh) {
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh.vertices.size());
    memcpy(vb, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh.triangles.size());
    memcpy(ib, mesh.triangles.data(), mesh.triangles.size() * sizeof(Triangle));
    rtcCommitGeometry(geom);
    unsigned int geomID = rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
    return geomID;
}

unsigned int Scene::addSharedMesh(const Mesh& mesh) {
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, mesh.vertices.data(), 0, sizeof(Vertex), mesh.vertices.size());
    rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, mesh.triangles.data(), 0, sizeof(Triangle), mesh.triangles.size());
    rtcCommitGeometry(geom);
    unsigned int geomID = rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
    return geomID;
}

void Scene::commit() {
    rtcCommitScene(scene);
}

int Scene::countIntersections(const Vec3& org, const Vec3& dir, float tmax) const {
    RTCRayHit rh;
    rh.ray.org_x = org.x; rh.ray.org_y = org.y; rh.ray.org_z = org.z;
    rh.ray.dir_x = dir.x; rh.ray.dir_y = dir.y; rh.ray.dir_z = dir.z;
    rh.ray.tnear = 0.0f;
    rh.ray.tfar = tmax;
    rh.ray.mask = -1;
    rh.ray.time = 0;
    rh.ray.flags = 0;
    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    
    RTCIntersectArguments args; 
    rtcInitIntersectArguments(&args);
    
    int intersections = 0;
    while (true) {
        rtcIntersect1(scene, &rh, &args);
        if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) break;
        
        intersections++;
        
        // Robust advancement: use machine epsilon to step past hit
        float hit_t = rh.ray.tfar;
        rh.ray.tnear = hit_t + std::max(1e-6f, hit_t * 1e-6f);
        rh.ray.tfar = tmax;
        rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        
        if (rh.ray.tnear >= tmax) break;
    }
    return intersections;
}

bool Scene::isInside(const Vec3& p, const Vec3& dir) const {
    RTCBounds bounds;
    rtcGetSceneBounds(scene, &bounds);
    if (bounds.lower_x > bounds.upper_x) return false;

    // Use a very robust distance for "outside"
    float maxDim = std::max({bounds.upper_x - bounds.lower_x, 
                            bounds.upper_y - bounds.lower_y, 
                            bounds.upper_z - bounds.lower_z});
    float t_start = maxDim * 5.0f; 
    
    Vec3 org = { p.x - dir.x * t_start, p.y - dir.y * t_start, p.z - dir.z * t_start };
    
    // Count hits strictly before p
    return (countIntersections(org, dir, t_start - 1e-5f) % 2 != 0);
}

Hit RayTracer::intersect(const Scene& scene, const Ray& ray) {
    RTCRayHit rh;
    rh.ray.org_x = ray.org.x; rh.ray.org_y = ray.org.y; rh.ray.org_z = ray.org.z;
    rh.ray.dir_x = ray.dir.x; rh.ray.dir_y = ray.dir.y; rh.ray.dir_z = ray.dir.z;
    rh.ray.tnear = ray.tnear; rh.ray.tfar = ray.tfar;
    rh.ray.mask = -1; rh.ray.time = 0;
    rh.ray.flags = 0;
    rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    RTCIntersectArguments args;
    rtcInitIntersectArguments(&args);
    rtcIntersect1(scene.getInternalScene(), &rh, &args);

    Hit hit;
    if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
        hit.hit = true;
        hit.t = rh.ray.tfar;
        hit.geomID = rh.hit.geomID;
        hit.primID = rh.hit.primID;
        hit.normal = {rh.hit.Ng_x, rh.hit.Ng_y, rh.hit.Ng_z};
        float nLen = hit.normal.length();
        if (nLen > 0) { hit.normal.x /= nLen; hit.normal.y /= nLen; hit.normal.z /= nLen; }
    }
    return hit;
}

bool RayTracer::occluded(const Scene& scene, const Ray& ray) {
    RTCRay r;
    r.org_x = ray.org.x; r.org_y = ray.org.y; r.org_z = ray.org.z;
    r.dir_x = ray.dir.x; r.dir_y = ray.dir.y; r.dir_z = ray.dir.z;
    r.tnear = ray.tnear; r.tfar = ray.tfar;
    r.mask = -1; r.time = 0; r.flags = 0;

    RTCOccludedArguments args;
    rtcInitOccludedArguments(&args);
    rtcOccluded1(scene.getInternalScene(), &r, &args);
    return r.tfar < 0; 
}
