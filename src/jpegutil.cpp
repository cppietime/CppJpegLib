/*
jpegutil.cpp
*/

#include <algorithm>
#include <utility>
#include <memory>
#include <numeric>
#include <cmath>
#include "jpegutil.hpp"

Jpeg::JpegSettings::JpegSettings(
        std::vector<JpegComponent> components,
        std::pair<int, int> size,
        JpegDensityUnits densityUnits,
        std::pair<int, int> density,
        int bitDepth,
        int quality,
        int compressionFlags,
        int numQTables,
        const int *qtables[JPEG_MAX_COMPONENTS],
        std::pair<int, int> version,
        int resetInterval) :
    components {components},
    size {size},
    densityUnits {densityUnits},
    density {density},
    bitDepth {bitDepth},
    quality {quality},
    compressionFlags {compressionFlags},
    numQTables {numQTables},
    resetInterval {resetInterval},
    mcuSize {0},
    version {version} {
        init();
}

Jpeg::JpegSettings::JpegSettings(
        std::pair<int, int> size,
        JpegDensityUnits densityUnits,
        std::pair<int, int> density,
        int bitDepth,
        int quality,
        int compressionFlags,
        int numQTables,
        const int *qtables[JPEG_MAX_COMPONENTS],
        std::pair<int, int> version,
        int resetInterval) :
    size {size},
    densityUnits {densityUnits},
    density {density},
    bitDepth {bitDepth},
    quality {quality},
    compressionFlags {compressionFlags},
    numQTables {numQTables},
    resetInterval {resetInterval},
    mcuSize {0},
    version {version} {
        JpegComponent defaultComponents[3] = {
            JpegComponent(std::pair<int, int>(2, 2), 0, 0, 0),
            JpegComponent(std::pair<int, int>(1, 1), 1, 1, 1),
            JpegComponent(std::pair<int, int>(1, 1), 1, 1, 1)
        };
        components = std::vector<JpegComponent>(defaultComponents, defaultComponents + 3);
        init();
}

void Jpeg::JpegSettings::init()
{
    numComponents = components.size();
    int maxX = 0, maxY = 0;
    float factor = (quality < 50) ? quality / 50.0 : quality - 50;
    for (int i = 0; i < numComponents; i ++) {
        componentOffsets[i] = mcuSize;
        mcuSize += components[i].sampling.first * components[i].sampling.second;
        maxX = std::max(maxX, components[i].sampling.first);
        maxY = std::max(maxY, components[i].sampling.second);
    }
    for (int i = 0; i < numQTables; i++) {
        for (int j = 0; j < JPEG_BLOCK_SIZE; j++) {
            this->qtables[i][j] = std::max(1, std::min(255, (int)(qtables[i][j] * factor)));
        }
    }
    mcuScale = std::pair<int, int>(maxX, maxY);
    numMcus = std::pair<int, int>(std::ceil(size.first / maxX / JPEG_BLOCK_ROW), std::ceil(size.second / maxY / JPEG_BLOCK_ROW));
}

Jpeg::Jpeg::~Jpeg()
{
    delete[] blocks;
}
