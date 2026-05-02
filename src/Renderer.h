#ifndef RENDERER_H
#define RENDERER_H

#include "RayTracer.h"
#include <vector>

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
