#include "MeshSlicerViewer.h"
#include "MeshIO.h"
#include "ImageUtils.h"
#include "MeshGeometry.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <iostream>

MeshSlicerViewer::MeshSlicerViewer()
    : m_resolution(512)
    , m_axis(SliceAxis::Y)
    , m_currentSlice(0)
    , m_totalSlices(100)
    , m_dirty(true)
{
}

bool MeshSlicerViewer::loadMesh(const char* filename) {
    if (MeshIO::load(filename, m_mesh)) {
        m_slicer = std::make_unique<RayTracedSlicer>(m_mesh);
        return true;
    }
    return false;
}

void MeshSlicerViewer::setResolution(int res) {
    m_resolution = res;
    m_dirty = true;
}

void MeshSlicerViewer::setAxis(SliceAxis axis) {
    m_axis = axis;
    m_currentSlice = 0;
    m_dirty = true;
}

void MeshSlicerViewer::saveCurrentSlice() {
    if (!m_slicer) return;
    
    SlicerLayer layer = m_slicer->computeLayer(m_currentSlice, m_totalSlices, m_resolution, m_axis);
    std::string filename = getOutputFilename();
    
    if (ImageUtils::savePNG(filename.c_str(), layer.texWidth, layer.texHeight, layer.textureData.data())) {
        std::cout << "Saved: " << filename << std::endl;
    } else {
        std::cerr << "Failed to save: " << filename << std::endl;
    }
}

std::string MeshSlicerViewer::getOutputFilename() const {
    char buf[256];
    const char* axisName[] = {"X", "Y", "Z"};
    snprintf(buf, sizeof(buf), "slice_%s_%03d.png", axisName[(int)m_axis], m_currentSlice);
    return std::string(buf);
}

void MeshSlicerViewer::run() {
    if (m_mesh.nodes.empty() || !m_slicer) {
        std::cerr << "No mesh loaded or slicer not initialized!" << std::endl;
        return;
    }

    MeshGeometry geom(m_mesh);
    AABB bbox = geom.computeAABB();
    Vec3 size = bbox.size();
    float extent = 0;
    switch (m_axis) {
        case SliceAxis::X: extent = size.x; break;
        case SliceAxis::Y: extent = size.y; break;
        case SliceAxis::Z: extent = size.z; break;
    }
    m_totalSlices = (int)(extent * 10) + 1;
    if (m_totalSlices < 10) m_totalSlices = 10;
    if (m_totalSlices > 500) m_totalSlices = 500;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Ray Traced Slicer - n:next p:prev s:save q:quit 1-3:axis",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        m_resolution,
        m_resolution,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = NULL;

    bool running = true;
    SDL_Event event;

    while (running) {
        if (m_dirty) {
            SlicerLayer layer = m_slicer->computeLayer(m_currentSlice, m_totalSlices, m_resolution, m_axis);

            if (texture) SDL_DestroyTexture(texture);

            if (layer.texWidth > 0 && layer.texHeight > 0 && !layer.textureData.empty()) {
                texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, layer.texWidth, layer.texHeight);
                SDL_UpdateTexture(texture, NULL, layer.textureData.data(), layer.texWidth * 3);

                char title[256];
                const char* axisName[] = {"X", "Y", "Z"};
                snprintf(title, sizeof(title), "Slice %d/%d (%s-axis, z=%.2f)",
                         m_currentSlice, m_totalSlices, axisName[(int)m_axis], layer.z);
                SDL_SetWindowTitle(window, title);

                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
            }
            m_dirty = false;
        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_n:
                        if (m_currentSlice < m_totalSlices - 1) {
                            m_currentSlice++;
                            m_dirty = true;
                        }
                        break;
                    case SDLK_p:
                        if (m_currentSlice > 0) {
                            m_currentSlice--;
                            m_dirty = true;
                        }
                        break;
                    case SDLK_s:
                        saveCurrentSlice();
                        break;
                    case SDLK_q:
                        running = false;
                        break;
                    case SDLK_1:
                        setAxis(SliceAxis::X);
                        break;
                    case SDLK_2:
                        setAxis(SliceAxis::Y);
                        break;
                    case SDLK_3:
                        setAxis(SliceAxis::Z);
                        break;
                    default:
                        break;
                }
            }
        }

        SDL_Delay(16);
    }

    if (texture) SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Viewer closed." << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <mesh.off> [resolution] [axis]" << std::endl;
        std::cout << "  axis: 0=X, 1=Y (default), 2=Z" << std::endl;
        std::cout << "Controls: n=next p=prev s=save q=quit 1-3=change axis" << std::endl;
        return 1;
    }

    MeshSlicerViewer viewer;
    if (!viewer.loadMesh(argv[1])) {
        std::cerr << "Failed to load mesh: " << argv[1] << std::endl;
        return 1;
    }

    int res = 512;
    if (argc >= 3) res = std::atoi(argv[2]);
    viewer.setResolution(res);

    if (argc >= 4) {
        int axis = std::atoi(argv[3]);
        if (axis == 0) viewer.setAxis(SliceAxis::X);
        else if (axis == 2) viewer.setAxis(SliceAxis::Z);
    }

    viewer.run();

    return 0;
}
