#include "MeshSlicer.h"
#include <cstdio>
#include <algorithm>
#include <cstring>

namespace {

Vertex intersectEdge(const Vertex& v1, const Vertex& v2, float d1, float d2) {
    if (std::abs(d1 - d2) < 1e-8f) return v2;
    float t = d1 / (d1 - d2);
    return {
        v1.x + t * (v2.x - v1.x),
        v1.y + t * (v2.y - v1.y),
        v1.z + t * (v2.z - v1.z)
    };
}

bool pointsEqual(const Vertex& a, const Vertex& b, float eps = 1e-5f) {
    return std::abs(a.x - b.x) < eps && std::abs(a.y - b.y) < eps && std::abs(a.z - b.z) < eps;
}

}

std::vector<Segment> MeshSlicer::sliceMesh(const Mesh& mesh, const Plane& plane) {
    std::vector<Segment> segments;

    for (const auto& tri : mesh.triangles) {
        const Vertex& v0 = mesh.vertices[tri.v0];
        const Vertex& v1 = mesh.vertices[tri.v1];
        const Vertex& v2 = mesh.vertices[tri.v2];

        float d0 = plane.distance(v0);
        float d1 = plane.distance(v1);
        float d2 = plane.distance(v2);

        int posCount = 0, negCount = 0;
        if (d0 > 0) posCount++; else if (d0 < 0) negCount++;
        if (d1 > 0) posCount++; else if (d1 < 0) negCount++;
        if (d2 > 0) posCount++; else if (d2 < 0) negCount++;

        if (posCount > 0 && negCount > 0) {
            std::vector<Vertex> intersections;
            if ((d0 > 0) != (d1 > 0)) intersections.push_back(intersectEdge(v0, v1, d0, d1));
            if ((d1 > 0) != (d2 > 0)) intersections.push_back(intersectEdge(v1, v2, d1, d2));
            if ((d2 > 0) != (d0 > 0)) intersections.push_back(intersectEdge(v2, v0, d2, d0));

            if (intersections.size() >= 2) {
                segments.push_back({intersections[0], intersections[1]});
            }
        }
    }

    return segments;
}

std::vector<std::vector<Vertex>> MeshSlicer::extractContours(const Mesh& mesh, const Plane& plane) {
    std::vector<Segment> segments = sliceMesh(mesh, plane);
    std::vector<std::vector<Vertex>> contours;

    if (segments.empty()) return contours;

    std::vector<bool> used(segments.size(), false);

    for (size_t i = 0; i < segments.size(); ++i) {
        if (used[i]) continue;

        std::vector<Vertex> contour;
        size_t current = i;
        used[current] = true;
        contour.push_back(segments[current].p1);
        contour.push_back(segments[current].p2);

        bool added = true;
        while (added) {
            added = false;
            Vertex last = contour.back();

            for (size_t j = 0; j < segments.size(); ++j) {
                if (used[j]) continue;
                if (pointsEqual(segments[j].p1, last)) {
                    used[j] = true;
                    contour.push_back(segments[j].p2);
                    added = true;
                    break;
                } else if (pointsEqual(segments[j].p2, last)) {
                    used[j] = true;
                    contour.push_back(segments[j].p1);
                    added = true;
                    break;
                }
            }
        }

        if (contour.size() >= 3) {
            contours.push_back(contour);
        }
    }

    return contours;
}

Mesh MeshSlicer::sliceAndCreateSolid(const Mesh& mesh, const Plane& plane, float offset) {
    Mesh result;
    Plane offsetPlane(plane.a, plane.b, plane.c, plane.d + offset);

    for (const auto& tri : mesh.triangles) {
        const Vertex& v0 = mesh.vertices[tri.v0];
        const Vertex& v1 = mesh.vertices[tri.v1];
        const Vertex& v2 = mesh.vertices[tri.v2];

        float d0 = offsetPlane.distance(v0);
        float d1 = offsetPlane.distance(v1);
        float d2 = offsetPlane.distance(v2);

        int posCount = 0, negCount = 0;
        if (d0 > 0) posCount++; else if (d0 < 0) negCount++;
        if (d1 > 0) posCount++; else if (d1 < 0) negCount++;
        if (d2 > 0) posCount++; else if (d2 < 0) negCount++;

        if (posCount > 0 && negCount > 0) {
            std::vector<Vertex> intersections;
            if ((d0 > 0) != (d1 > 0)) intersections.push_back(intersectEdge(v0, v1, d0, d1));
            if ((d1 > 0) != (d2 > 0)) intersections.push_back(intersectEdge(v1, v2, d1, d2));
            if ((d2 > 0) != (d0 > 0)) intersections.push_back(intersectEdge(v2, v0, d2, d0));

            if (intersections.size() >= 2) {
                unsigned int baseIdx = (unsigned int)result.vertices.size();
                result.vertices.push_back(intersections[0]);
                result.vertices.push_back(intersections[1]);
                result.vertices.push_back(intersections[1]);
                result.triangles.push_back({baseIdx, baseIdx + 1, baseIdx + 2});
            }
        } else if (negCount == 3) {
            unsigned int baseIdx = (unsigned int)result.vertices.size();
            result.vertices.push_back(v0);
            result.vertices.push_back(v1);
            result.vertices.push_back(v2);
            result.triangles.push_back({baseIdx, baseIdx + 1, baseIdx + 2});
        }
    }

    return result;
}

Plane MeshSlicer::createPlane(SliceDirection dir, float position) {
    switch (dir) {
        case SLICE_X: return Plane(1, 0, 0, -position);
        case SLICE_Y: return Plane(0, 1, 0, -position);
        case SLICE_Z: return Plane(0, 0, 1, -position);
    }
    return Plane();
}

AABB MeshSlicer::computeBoundingBox(const Mesh& mesh) {
    AABB bbox;
    for (const auto& v : mesh.vertices) {
        bbox.expand(v);
    }
    return bbox;
}

float MeshSlicer::getSlicePosition(const Mesh& mesh, SliceDirection dir, int sliceIndex, int totalSlices) {
    AABB bbox = computeBoundingBox(mesh);
    float minVal, maxVal;

    switch (dir) {
        case SLICE_X:
            minVal = bbox.min.x; maxVal = bbox.max.x;
            break;
        case SLICE_Y:
            minVal = bbox.min.y; maxVal = bbox.max.y;
            break;
        case SLICE_Z:
            minVal = bbox.min.z; maxVal = bbox.max.z;
            break;
    }

    if (totalSlices <= 1) return minVal;
    return minVal + (maxVal - minVal) * sliceIndex / (float)(totalSlices - 1);
}

static unsigned int crc32_table[256];
static bool crc32_init = false;

static void init_crc32() {
    if (crc32_init) return;
    for (unsigned int i = 0; i < 256; ++i) {
        unsigned int c = i;
        for (int j = 0; j < 8; ++j) {
            c = (c >> 1) ^ (c & 1 ? 0xedb88320 : 0);
        }
        crc32_table[i] = c;
    }
    crc32_init = true;
}

static unsigned int crc32(const unsigned char* data, int len) {
    init_crc32();
    unsigned int crc = 0xffffffff;
    for (int i = 0; i < len; ++i) {
        crc = crc32_table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
    }
    return crc ^ 0xffffffff;
}

static void adler32(const unsigned char* data, int len, unsigned int& a, unsigned int& b) {
    const unsigned int MOD = 65521;
    unsigned int s1 = a, s2 = b;
    for (int i = 0; i < len; ++i) {
        s1 = (s1 + data[i]) % MOD;
        s2 = (s2 + s1) % MOD;
    }
    a = s1;
    b = s2;
}

static unsigned char deflate_store(const unsigned char* data, int len, unsigned char* out, int& outLen) {
    int pos = 0;
    out[pos++] = 0x78;
    out[pos++] = 0x01;

    int blockSize = 65535;
    for (int i = 0; i < len; i += blockSize) {
        int remaining = len - i;
        int blockLen = (remaining > blockSize) ? blockSize : remaining;
        bool isLast = (i + blockLen >= len);

        out[pos++] = isLast ? 1 : 0;
        out[pos++] = blockLen & 0xff;
        out[pos++] = (blockLen >> 8) & 0xff;
        out[pos++] = (~blockLen) & 0xff;
        out[pos++] = ((~blockLen) >> 8) & 0xff;

        memcpy(out + pos, data + i, blockLen);
        pos += blockLen;
    }

    unsigned int a = 1, b = 1;
    adler32(data, len, a, b);
    out[pos++] = (a >> 24) & 0xff;
    out[pos++] = (a >> 16) & 0xff;
    out[pos++] = (a >> 8) & 0xff;
    out[pos++] = a & 0xff;
    out[pos++] = (b >> 24) & 0xff;
    out[pos++] = (b >> 16) & 0xff;
    out[pos++] = (b >> 8) & 0xff;
    out[pos++] = b & 0xff;

    outLen = pos;
    return 0;
}

bool MeshSlicer::saveSlicePNG(const Mesh& mesh, const Plane& plane, const char* filename, int resolution) {
    std::vector<Segment> segments = sliceMesh(mesh, plane);
    if (segments.empty()) return false;

    AABB bbox;
    for (const auto& seg : segments) {
        bbox.expand(seg.p1);
        bbox.expand(seg.p2);
    }

    float width = bbox.max.x - bbox.min.x;
    float height = bbox.max.y - bbox.min.y;

    if (width < 1e-6f && height < 1e-6f) {
        width = 100; height = 100;
    }

    float scale = 1.0f;
    if (width > height) {
        scale = (float)resolution / width;
    } else {
        scale = (float)resolution / height;
    }

    int imgW = (int)(width * scale) + 4;
    int imgH = (int)(height * scale) + 4;
    imgW = std::max(1, std::min(imgW, 4096));
    imgH = std::max(1, std::min(imgH, 4096));

    std::vector<unsigned char> image(imgW * imgH * 3, 255);

    for (const auto& seg : segments) {
        int x1 = (int)((seg.p1.x - bbox.min.x) * scale) + 2;
        int y1 = (int)((seg.p1.y - bbox.min.y) * scale) + 2;
        int x2 = (int)((seg.p2.x - bbox.min.x) * scale) + 2;
        int y2 = (int)((seg.p2.y - bbox.min.y) * scale) + 2;

        x1 = std::max(0, std::min(imgW - 1, x1));
        y1 = std::max(0, std::min(imgH - 1, y1));
        x2 = std::max(0, std::min(imgW - 1, x2));
        y2 = std::max(0, std::min(imgH - 1, y2));

        int dx = std::abs(x2 - x1);
        int dy = std::abs(y2 - y1);
        int sx = x1 < x2 ? 1 : -1;
        int sy = y1 < y2 ? 1 : -1;
        int err = dx - dy;

        while (true) {
            if (x1 >= 0 && x1 < imgW && y1 >= 0 && y1 < imgH) {
                image[(y1 * imgW + x1) * 3 + 0] = 0;
                image[(y1 * imgW + x1) * 3 + 1] = 0;
                image[(y1 * imgW + x1) * 3 + 2] = 0;
            }
            if (x1 == x2 && y1 == y2) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x1 += sx; }
            if (e2 < dx) { err += dx; y1 += sy; }
        }
    }

    FILE* fp = fopen(filename, "wb");
    if (!fp) return false;

    fwrite("\x89PNG\r\n\x1a\n", 8, 1, fp);

    unsigned int len = 13;
    fwrite(&len, 4, 1, fp);
    fwrite("IHDR", 4, 1, fp);
    unsigned char ihdr[13] = {
        (unsigned char)(imgW >> 24), (unsigned char)(imgW >> 16), (unsigned char)(imgW >> 8), (unsigned char)imgW,
        (unsigned char)(imgH >> 24), (unsigned char)(imgH >> 16), (unsigned char)(imgH >> 8), (unsigned char)imgH,
        8, 2, 0, 0, 0
    };
    fwrite(ihdr, 13, 1, fp);
    unsigned int crc = crc32((const unsigned char*)"IHDR", 4);
    crc = crc32(ihdr, 13) ^ crc;
    fwrite(&crc, 4, 1, fp);

    std::vector<unsigned char> rawData(imgH * (imgW * 3 + 1));
    for (int y = 0; y < imgH; ++y) {
        rawData[y * (imgW * 3 + 1)] = 0;
        for (int x = 0; x < imgW; ++x) {
            rawData[y * (imgW * 3 + 1) + 1 + x * 3 + 0] = image[(y * imgW + x) * 3 + 0];
            rawData[y * (imgW * 3 + 1) + 1 + x * 3 + 1] = image[(y * imgW + x) * 3 + 1];
            rawData[y * (imgW * 3 + 1) + 1 + x * 3 + 2] = image[(y * imgW + x) * 3 + 2];
        }
    }

    int compressedSize = 0;
    std::vector<unsigned char> compressed(rawData.size() + rawData.size() / 1000 + 20);
    deflate_store(rawData.data(), rawData.size(), compressed.data(), compressedSize);

    len = compressedSize;
    fwrite(&len, 4, 1, fp);
    fwrite("IDAT", 4, 1, fp);
    fwrite(compressed.data(), compressedSize, 1, fp);
    crc = crc32((const unsigned char*)"IDAT", 4);
    crc = crc32(compressed.data(), compressedSize) ^ crc;
    fwrite(&crc, 4, 1, fp);

    len = 0;
    fwrite(&len, 4, 1, fp);
    fwrite("IEND", 4, 1, fp);
    crc = crc32((const unsigned char*)"IEND", 4);
    fwrite(&crc, 4, 1, fp);

    fclose(fp);
    return true;
}

std::vector<unsigned char> MeshSlicer::renderSlice(const Mesh& mesh, const Plane& plane, int resolution, int& outW, int& outH) {
    std::vector<Segment> segments = sliceMesh(mesh, plane);
    outW = outH = 0;

    if (segments.empty()) return {};

    AABB bbox;
    for (const auto& seg : segments) {
        bbox.expand(seg.p1);
        bbox.expand(seg.p2);
    }

    float width = bbox.max.x - bbox.min.x;
    float height = bbox.max.y - bbox.min.y;

    if (width < 1e-6f && height < 1e-6f) {
        width = 100; height = 100;
    }

    float scale = 1.0f;
    if (width > height) {
        scale = (float)resolution / width;
    } else {
        scale = (float)resolution / height;
    }

    outW = (int)(width * scale) + 4;
    outH = (int)(height * scale) + 4;
    outW = std::max(1, std::min(outW, 4096));
    outH = std::max(1, std::min(outH, 4096));

    std::vector<unsigned char> image(outW * outH * 3, 255);

    for (const auto& seg : segments) {
        int x1 = (int)((seg.p1.x - bbox.min.x) * scale) + 2;
        int y1 = (int)((seg.p1.y - bbox.min.y) * scale) + 2;
        int x2 = (int)((seg.p2.x - bbox.min.x) * scale) + 2;
        int y2 = (int)((seg.p2.y - bbox.min.y) * scale) + 2;

        x1 = std::max(0, std::min(outW - 1, x1));
        y1 = std::max(0, std::min(outH - 1, y1));
        x2 = std::max(0, std::min(outW - 1, x2));
        y2 = std::max(0, std::min(outH - 1, y2));

        int dx = std::abs(x2 - x1);
        int dy = std::abs(y2 - y1);
        int sx = x1 < x2 ? 1 : -1;
        int sy = y1 < y2 ? 1 : -1;
        int err = dx - dy;

        while (true) {
            if (x1 >= 0 && x1 < outW && y1 >= 0 && y1 < outH) {
                image[(y1 * outW + x1) * 3 + 0] = 0;
                image[(y1 * outW + x1) * 3 + 1] = 0;
                image[(y1 * outW + x1) * 3 + 2] = 0;
            }
            if (x1 == x2 && y1 == y2) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x1 += sx; }
            if (e2 < dx) { err += dx; y1 += sy; }
        }
    }

    return image;
}