#ifndef RAY_TRACED_SLICER_H
#define RAY_TRACED_SLICER_H

#include "mesh_utils.h"
#include "RayTracer.h"
#include <vector>

enum class SliceAxis { X, Y, Z };

struct SlicerLayer {
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    std::vector<Color4b> vertexColors;
    std::vector<unsigned char> textureData;
    std::vector<Vertex> raySources;
    int texWidth = 0;
    int texHeight = 0;
    float z = 0.0f;
};

class RayTracedSlicer {
public:
    RayTracedSlicer(const Mesh& mesh);
    ~RayTracedSlicer() = default;

    SlicerLayer computeLayer(int layerIdx, int numLayers, int resolution, SliceAxis axis);

private:
    const Mesh& m_mesh;
    Scene m_scene;
    AABB m_bbox;
};

#endif
