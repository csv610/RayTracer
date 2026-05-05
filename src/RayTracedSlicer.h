#ifndef RAY_TRACED_SLICER_H
#define RAY_TRACED_SLICER_H

#include "Mesh.h"
#include "RayTracer.h"
#include <vector>

enum class SliceAxis { X, Y, Z };

struct SlicerLayer {
    std::vector<Node> nodes;
    std::vector<Triangle> triangles;
    std::vector<Color4b> nodeColors;
    std::vector<unsigned char> textureData;
    std::vector<Node> raySources;
    int texWidth = 0;
    int texHeight = 0;
    float z = 0.0f;
};

/**
 * @class RayTracedSlicer
 * @brief A high-reliability mesh slicer based on ray-tracing.
 * 
 * Unlike traditional geometric slicers, this component uses ray-casting 
 * to determine the interior state of a mesh at any point on a slicing plane. 
 * This makes it extremely robust against mesh defects such as non-manifold 
 * edges or self-intersections.
 * 
 * It generates SlicerLayer objects containing sampled grid data, 
 * which can be used for voxelization or 2D image generation.
 */
class RayTracedSlicer {
public:
    RayTracedSlicer(const Mesh& mesh);
    ~RayTracedSlicer() = default;

    SlicerLayer computeLayer(int layerIdx, int numLayers, int resolution, SliceAxis axis);

    /**
     * @brief Generates multiple slices and saves them to disk as PPM images.
     */
    void sliceBatch(int numLayers, int resolution, const std::string& filenamePrefix) const;

private:
    const Mesh& m_mesh;
    Scene m_scene;
    AABB m_bbox;
};

#endif
