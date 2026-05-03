#include "RayTracedSlicer.h"
#include "InsideOutsideKernel.h"
#include <tbb/parallel_for.h>
#include <iostream>
#include <algorithm>
#include <cmath>

RayTracedSlicer::RayTracedSlicer(const Mesh& mesh) : m_mesh(mesh) {
    m_scene.addSharedMesh(m_mesh);
    m_scene.commit();
    for (const auto& v : m_mesh.vertices) {
        m_bbox.expand(v);
    }
    m_bbox.pad(0.005f); // 1% total expansion
}

SlicerLayer RayTracedSlicer::computeLayer(int layerIdx, int numLayers, int resolution, SliceAxis axis) {
    SlicerLayer layer;
    if (m_mesh.vertices.empty() || m_mesh.triangles.empty()) return layer;

    Vec3 size = m_bbox.size();
    float range, minPos;
    
    float step;
    int gridU, gridV;
    Vec3 uStep = {0,0,0}, vStep = {0,0,0};
    Vec3 rayDir = {0,0,0};
    float emitterPlanePos = 0.0f;

    switch (axis) {
        case SliceAxis::X:
            range = size.x; minPos = m_bbox.min.x;
            layer.z = minPos + (float)(layerIdx + 1) / (numLayers + 1) * range;
            step = std::max(size.y, size.z) / resolution;
            gridU = (int)(size.y / step) + 1;
            gridV = (int)(size.z / step) + 1;
            uStep = {0, step, 0};
            vStep = {0, 0, step};
            rayDir = {1, 0, 0};
            emitterPlanePos = minPos - 2.0f * (range / (numLayers + 1));
            break;
        case SliceAxis::Y:
            range = size.y; minPos = m_bbox.min.y;
            layer.z = minPos + (float)(layerIdx + 1) / (numLayers + 1) * range;
            step = std::max(size.x, size.z) / resolution;
            gridU = (int)(size.x / step) + 1;
            gridV = (int)(size.z / step) + 1;
            uStep = {step, 0, 0};
            vStep = {0, 0, step};
            rayDir = {0, 1, 0};
            emitterPlanePos = minPos - 2.0f * (range / (numLayers + 1));
            break;
        case SliceAxis::Z:
        default:
            range = size.z; minPos = m_bbox.min.z;
            layer.z = minPos + (float)(layerIdx + 1) / (numLayers + 1) * range;
            step = std::max(size.x, size.y) / resolution;
            gridU = (int)(size.x / step) + 1;
            gridV = (int)(size.y / step) + 1;
            uStep = {step, 0, 0};
            vStep = {0, step, 0};
            rayDir = {0, 0, 1};
            emitterPlanePos = minPos - 2.0f * (range / (numLayers + 1));
            break;
    }

    if (gridU <= 0 || gridV <= 0) return layer;

    layer.texWidth = gridU;
    layer.texHeight = gridV;
    layer.textureData.resize(gridU * gridV * 3, 40);
    layer.vertices.resize(gridU * gridV);
    layer.vertexColors.resize(gridU * gridV);
    layer.raySources.resize(gridU * gridV);

    // Instantiate kernel (cached per layer compute for efficiency)
    InsideOutsideKernel kernel(m_mesh);

    tbb::parallel_for(0, gridV, [&](int v) {
        for (int u = 0; u < gridU; ++u) {
            Vec3 p = {
                (axis == SliceAxis::X) ? layer.z : (m_bbox.min.x + u * uStep.x + v * vStep.x),
                (axis == SliceAxis::Y) ? layer.z : (m_bbox.min.y + u * uStep.y + v * vStep.y),
                (axis == SliceAxis::Z) ? layer.z : (m_bbox.min.z + u * uStep.z + v * vStep.z)
            };

            Vec3 org = {
                (axis == SliceAxis::X) ? emitterPlanePos : p.x,
                (axis == SliceAxis::Y) ? emitterPlanePos : p.y,
                (axis == SliceAxis::Z) ? emitterPlanePos : p.z
            };

            int idx = v * gridU + u;
            layer.vertices[idx] = {p.x, p.y, p.z};
            
            float visOffset = 2.0f;
            layer.raySources[idx] = {
                org.x - rayDir.x * visOffset,
                org.y - rayDir.y * visOffset,
                org.z - rayDir.z * visOffset
            };
            
            Color4b color;
            if (axis == SliceAxis::X) color = {255, 0, 0, 255};
            else if (axis == SliceAxis::Y) color = {0, 255, 0, 255};
            else color = {0, 0, 255, 255};
            layer.vertexColors[idx] = color;

            // Use the Kernel for foolproof Inside/Outside/Surface classification
            int status = kernel.classify(p);
            
            int texIdx = idx * 3;
            if (status <= 0) { // Inside (-1) or On Surface (0)
                layer.textureData[texIdx + 0] = 255;
                layer.textureData[texIdx + 1] = 255;
                layer.textureData[texIdx + 2] = 255;
            } else { // Outside (1)
                layer.textureData[texIdx + 0] = 40;
                layer.textureData[texIdx + 1] = 40;
                layer.textureData[texIdx + 2] = 40;
            }
        }
    });

    for (int v = 0; v < gridV - 1; ++v) {
        for (int u = 0; u < gridU - 1; ++u) {
            bool i00 = layer.textureData[((v)*gridU + (u))*3] == 255;
            bool i10 = layer.textureData[((v)*gridU + (u+1))*3] == 255;
            bool i01 = layer.textureData[((v+1)*gridU + (u))*3] == 255;
            bool i11 = layer.textureData[((v+1)*gridU + (u+1))*3] == 255;
            
            if (i00 || i10 || i01 || i11) {
                layer.triangles.push_back({(unsigned)(v * gridU + u), (unsigned)(v * gridU + u + 1), (unsigned)((v + 1) * gridU + u)});
                layer.triangles.push_back({(unsigned)(v * gridU + u + 1), (unsigned)((v + 1) * gridU + u + 1), (unsigned)((v + 1) * gridU + u)});
            }
        }
    }

    return layer;
}
