#include "ImageUtils.h"
#include <cstdio>
#include <vector>
#include <cstring>
#include <algorithm>

namespace {

static unsigned int crc32_table[256];
static bool crc32_init = false;

void init_crc32() {
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

unsigned int crc32(const unsigned char* data, int len) {
    init_crc32();
    unsigned int crc = 0xffffffff;
    for (int i = 0; i < len; ++i) {
        crc = crc32_table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
    }
    return crc ^ 0xffffffff;
}

void adler32(const unsigned char* data, int len, unsigned int& a, unsigned int& b) {
    const unsigned int MOD = 65521;
    unsigned int s1 = a, s2 = b;
    for (int i = 0; i < len; ++i) {
        s1 = (s1 + data[i]) % MOD;
        s2 = (s2 + s1) % MOD;
    }
    a = s1;
    b = s2;
}

void deflate_store(const unsigned char* data, int len, unsigned char* out, int& outLen) {
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
}

} // namespace

bool ImageUtils::savePNG(const char* filename, int width, int height, const unsigned char* rgbData) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) return false;

    fwrite("\x89PNG\r\n\x1a\n", 8, 1, fp);

    unsigned int len = 13;
    unsigned int nlen = ((len << 24) & 0xff000000) | ((len << 8) & 0x00ff0000) | ((len >> 8) & 0x0000ff00) | ((len >> 24) & 0x000000ff);
    fwrite(&nlen, 4, 1, fp);
    fwrite("IHDR", 4, 1, fp);
    unsigned char ihdr[13] = {
        (unsigned char)(width >> 24), (unsigned char)(width >> 16), (unsigned char)(width >> 8), (unsigned char)width,
        (unsigned char)(height >> 24), (unsigned char)(height >> 16), (unsigned char)(height >> 8), (unsigned char)height,
        8, 2, 0, 0, 0
    };
    fwrite(ihdr, 13, 1, fp);
    unsigned int crc = crc32((const unsigned char*)"IHDR", 4);
    crc = crc32(ihdr, 13) ^ crc;
    unsigned int ncrc = ((crc << 24) & 0xff000000) | ((crc << 8) & 0x00ff0000) | ((crc >> 8) & 0x0000ff00) | ((crc >> 24) & 0x000000ff);
    fwrite(&ncrc, 4, 1, fp);

    std::vector<unsigned char> rawData(height * (width * 3 + 1));
    for (int y = 0; y < height; ++y) {
        rawData[y * (width * 3 + 1)] = 0;
        memcpy(&rawData[y * (width * 3 + 1) + 1], &rgbData[y * width * 3], width * 3);
    }

    int compressedSize = 0;
    std::vector<unsigned char> compressed(rawData.size() + rawData.size() / 1000 + 20);
    deflate_store(rawData.data(), rawData.size(), compressed.data(), compressedSize);

    len = compressedSize;
    nlen = ((len << 24) & 0xff000000) | ((len << 8) & 0x00ff0000) | ((len >> 8) & 0x0000ff00) | ((len >> 24) & 0x000000ff);
    fwrite(&nlen, 4, 1, fp);
    fwrite("IDAT", 4, 1, fp);
    fwrite(compressed.data(), compressedSize, 1, fp);
    crc = crc32((const unsigned char*)"IDAT", 4);
    crc = crc32(compressed.data(), compressedSize) ^ crc;
    ncrc = ((crc << 24) & 0xff000000) | ((crc << 8) & 0x00ff0000) | ((crc >> 8) & 0x0000ff00) | ((crc >> 24) & 0x000000ff);
    fwrite(&ncrc, 4, 1, fp);

    len = 0;
    fwrite(&len, 4, 1, fp);
    fwrite("IEND", 4, 1, fp);
    crc = crc32((const unsigned char*)"IEND", 4);
    ncrc = ((crc << 24) & 0xff000000) | ((crc << 8) & 0x00ff0000) | ((crc >> 8) & 0x0000ff00) | ((crc >> 24) & 0x000000ff);
    fwrite(&ncrc, 4, 1, fp);

    fclose(fp);
    return true;
}
