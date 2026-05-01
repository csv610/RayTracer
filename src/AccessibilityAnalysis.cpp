#include "AccessibilityAnalysis.h"

AccessibilityAnalysis::AccessibilityAnalysis(const Mesh& mesh) : mesh_(mesh) {
    device_ = rtcNewDevice(nullptr);
    scene_ = rtcNewScene(device_);
    RTCGeometry geom = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);

    Vertex* vb = (Vertex*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), mesh_.vertices.size());
    memcpy(vb, mesh_.vertices.data(), mesh_.vertices.size() * sizeof(Vertex));
    Triangle* ib = (Triangle*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), mesh_.triangles.size());
    memcpy(ib, mesh_.triangles.data(), mesh_.triangles.size() * sizeof(Triangle));

    rtcCommitGeometry(geom);
    rtcAttachGeometry(scene_, geom);
    rtcReleaseGeometry(geom);
    rtcCommitScene(scene_);
}

AccessibilityAnalysis::~AccessibilityAnalysis() {
    if (scene_) rtcReleaseScene(scene_);
    if (device_) rtcReleaseDevice(device_);
}

void AccessibilityAnalysis::analyze(float toolRadius) {
    Vertex center;
    float meshRadius;
    computeBoundingSphere(mesh_, center, meshRadius);
    float rayLength = meshRadius * 4.0f;
    float epsilon = meshRadius * 1e-4f;

    triColors_.resize(mesh_.triangles.size());
    inaccessibleCount_ = 0;

    tbb::parallel_for(size_t(0), mesh_.triangles.size(), [&](size_t i) {
        const Triangle& tri = mesh_.triangles[i];
        Vec3 normal = computeFaceNormal(mesh_.vertices[tri.v0], mesh_.vertices[tri.v1], mesh_.vertices[tri.v2]);
        Vec3 faceCenter = computeFaceCenter(mesh_.vertices[tri.v0], mesh_.vertices[tri.v1], mesh_.vertices[tri.v2]);

        if (normal.z < 0.05f) {
            triColors_[i] = {1.0f, 0.0f, 0.0f}; // Red
            return;
        }

        const int numPerimeterRays = 8;
        bool accessible = true;

        for (int j = -1; j < numPerimeterRays; ++j) {
            float dx = 0, dy = 0;
            if (j >= 0) {
                float angle = (2.0f * M_PI * j) / numPerimeterRays;
                dx = cos(angle) * toolRadius;
                dy = sin(angle) * toolRadius;
            }

            RTCRayHit rh;
            rh.ray.org_x = faceCenter.x + dx;
            rh.ray.org_y = faceCenter.y + dy;
            rh.ray.org_z = faceCenter.z + epsilon;
            rh.ray.dir_x = 0;
            rh.ray.dir_y = 0;
            rh.ray.dir_z = 1.0f; 
            rh.ray.tnear = 0.0f;
            rh.ray.tfar = rayLength;
            rh.ray.mask = -1;
            rh.ray.time = 0;
            rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

            RTCIntersectArguments args;
            rtcInitIntersectArguments(&args);
            rtcIntersect1(scene_, &rh, &args);

            if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
                accessible = false;
                break;
            }
        }

        if (accessible) {
            triColors_[i] = {0.0f, 1.0f, 0.0f}; // Green
        } else {
            triColors_[i] = {1.0f, 0.0f, 0.0f}; // Red
        }
    });

    for (const auto& c : triColors_) {
        if (c.x > 0.5f) inaccessibleCount_++;
    }
}
