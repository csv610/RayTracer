#ifndef RENDERER_H
#define RENDERER_H

#include "RayTracer.h"
#include <vector>

/**
 * @class Renderer
 * @brief A ray-tracing based visualization engine for 3D scenes.
 * 
 * Renderer provides functionality to transform 3D scenes into 2D image 
 * representations. It supports generating normal maps (surface orientation), 
 * depth maps (distance from camera), and simple shaded renders using 
 * basic lighting models. 
 * 
 * It uses a pinhole camera model and is highly parallelized for performance.
 */
class Renderer {
public:
    struct Camera {
        Vec3 pos;
        Vec3 target;
        Vec3 up;
        float fov;
        int width, height;
    };

    static std::vector<Vec3> renderNormalMap(const Scene& scene, const Camera& cam);
    static std::vector<float> renderDepthMap(const Scene& scene, const Camera& cam);
    static std::vector<Vec3> renderSimpleLighting(const Scene& scene, const Camera& cam, Vec3 lightDir);
};

#endif // RENDERER_H
