#include "MeshSlicerViewer.h"
#include "MeshIO.h"
#include <SDL2/SDL.h>
#include <cstdio>
#include <iostream>

MeshSlicerViewer::MeshSlicerViewer()
    : m_resolution(512)
    , m_direction(MeshSlicer::SLICE_Y)
    , m_currentSlice(0)
    , m_totalSlices(100)
    , m_dirty(true)
{
}

bool MeshSlicerViewer::loadMesh(const char* filename) {
    return MeshIO::load(filename, m_mesh);
}

void MeshSlicerViewer::setResolution(int res) {
    m_resolution = res;
    m_dirty = true;
}

void MeshSlicerViewer::setDirection(MeshSlicer::SliceDirection dir) {
    m_direction = dir;
    m_currentSlice = 0;
    m_dirty = true;
}

void MeshSlicerViewer::saveCurrentSlice() {
    float pos = MeshSlicer::getSlicePosition(m_mesh, m_direction, m_currentSlice, m_totalSlices);
    Plane plane = MeshSlicer::createPlane(m_direction, pos);
    std::string filename = getOutputFilename();
    MeshSlicer::saveSlicePNG(m_mesh, plane, filename.c_str(), m_resolution);
    std::cout << "Saved: " << filename << std::endl;
}

std::string MeshSlicerViewer::getOutputFilename() const {
    char buf[256];
    const char* dirName[] = {"X", "Y", "Z"};
    snprintf(buf, sizeof(buf), "slice_%s_%03d.png", dirName[m_direction], m_currentSlice);
    return std::string(buf);
}

void MeshSlicerViewer::run() {
    if (m_mesh.vertices.empty()) {
        std::cerr << "No mesh loaded!" << std::endl;
        return;
    }

    AABB bbox = MeshSlicer::computeBoundingBox(m_mesh);
    float extent = 0;
    switch (m_direction) {
        case MeshSlicer::SLICE_X: extent = bbox.max.x - bbox.min.x; break;
        case MeshSlicer::SLICE_Y: extent = bbox.max.y - bbox.min.y; break;
        case MeshSlicer::SLICE_Z: extent = bbox.max.z - bbox.min.z; break;
    }
    m_totalSlices = (int)(extent * 10) + 1;
    if (m_totalSlices < 10) m_totalSlices = 10;
    if (m_totalSlices > 500) m_totalSlices = 500;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Mesh Slicer - n:next p:prev s:save q:quit 1-3:axis",
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
            float pos = MeshSlicer::getSlicePosition(m_mesh, m_direction, m_currentSlice, m_totalSlices);
            Plane plane = MeshSlicer::createPlane(m_direction, pos);

            int w, h;
            std::vector<unsigned char> pixels = MeshSlicer::renderSlice(m_mesh, plane, m_resolution, w, h);

            if (texture) SDL_DestroyTexture(texture);

            if (w > 0 && h > 0 && !pixels.empty()) {
                texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, w, h);
                SDL_UpdateTexture(texture, NULL, pixels.data(), w * 3);

                char title[256];
                const char* axisName[] = {"X", "Y", "Z"};
                snprintf(title, sizeof(title), "Slice %d/%d (%s-axis, pos=%.2f)",
                         m_currentSlice, m_totalSlices, axisName[m_direction], pos);
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
                        setDirection(MeshSlicer::SLICE_X);
                        break;
                    case SDLK_2:
                        setDirection(MeshSlicer::SLICE_Y);
                        break;
                    case SDLK_3:
                        setDirection(MeshSlicer::SLICE_Z);
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
        std::cout << "Usage: " << argv[0] << " <mesh.off> [resolution] [direction]" << std::endl;
        std::cout << "  direction: 0=X, 1=Y (default), 2=Z" << std::endl;
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
        int dir = std::atoi(argv[3]);
        if (dir == 0) viewer.setDirection(MeshSlicer::SLICE_X);
        else if (dir == 2) viewer.setDirection(MeshSlicer::SLICE_Z);
    }

    viewer.run();

    return 0;
}