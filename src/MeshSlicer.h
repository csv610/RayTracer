#ifndef MESH_SLICER_H
#define MESH_SLICER_H

#include "mesh_utils.h"
#include <vector>
#include <cmath>

struct Plane {
    float a, b, c, d;
    Plane() : a(0), b(0), c(1), d(0) {}
    Plane(float a_, float b_, float c_, float d_) : a(a_), b(b_), c(c_), d(d_) {}
    float distance(const Vertex& v) const { return a * v.x + b * v.y + c * v.z + d; }
};

struct Segment {
    Vertex p1, p2;
};

class MeshSlicer {
public:
    enum SliceDirection { SLICE_X, SLICE_Y, SLICE_Z };

    static std::vector<Segment> sliceMesh(const Mesh& mesh, const Plane& plane);
    static std::vector<std::vector<Vertex>> extractContours(const Mesh& mesh, const Plane& plane);
    static Mesh sliceAndCreateSolid(const Mesh& mesh, const Plane& plane, float offset);

    static Plane createPlane(SliceDirection dir, float position);
    static AABB computeBoundingBox(const Mesh& mesh);

    static bool saveSlicePNG(const Mesh& mesh, const Plane& plane, const char* filename, int resolution);
    static std::vector<unsigned char> renderSlice(const Mesh& mesh, const Plane& plane, int resolution, int& outW, int& outH);

    static float getSlicePosition(const Mesh& mesh, SliceDirection dir, int sliceIndex, int totalSlices);
};

#endif