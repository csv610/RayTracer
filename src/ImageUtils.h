#ifndef IMAGE_UTILS_H
#define IMAGE_UTILS_H

#include <vector>

/**
 * @class ImageUtils
 * @brief Standalone utilities for image processing and export.
 * 
 * Provides a dependency-free PNG encoder suitable for saving
 * raw RGB buffers to disk.
 */
class ImageUtils {
public:
    /**
     * @brief Saves an RGB buffer as a PNG image.
     * @param filename Path to the output file.
     * @param width Image width in pixels.
     * @param height Image height in pixels.
     * @param rgbData Pointer to raw RGB888 data (3 bytes per pixel).
     * @return true if successful, false otherwise.
     */
    static bool savePNG(const char* filename, int width, int height, const unsigned char* rgbData);
};

#endif // IMAGE_UTILS_H
