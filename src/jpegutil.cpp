/*
jpegutil.cpp
*/

#include <algorithm>
#include <utility>
#include <memory>
#include <numeric>
#include <cmath>
#include "jpegutil.hpp"

Jpeg::JpegSettings::JpegSettings(int numComponents,
        std::pair<int, int> sampling[JPEG_MAX_COMPONENTS],
        std::pair<int, int> size,
        JpegDensityUnits densityUnits,
        std::pair<int, int> density,
        int bitDepth,
        int quality,
        int compressionFlags,
        int numQTables,
        const int *qtables[JPEG_MAX_COMPONENTS],
        size_t qIndices[JPEG_MAX_COMPONENTS],
        std::pair<int, int> version,
        int resetInterval) :
    numComponents {numComponents},
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
        int maxX = 0, maxY = 0;
        float factor = (quality < 50) ? quality / 50.0 : quality - 50;
        for (int i = 0; i < numComponents; i ++) {
            componentOffsets[i] = mcuSize;
            this->sampling[i] = sampling[i];
            mcuSize += sampling[i].first * sampling[i].second;
            maxX = std::max(maxX, sampling[i].first);
            maxY = std::max(maxY, sampling[i].second);
        }
        for (int i = 0; i < numQTables; i++) {
            for (int j = 0; j < JPEG_BLOCK_SIZE; j++) {
                this->qtables[i][j] = std::max(1, std::min(255, (int)(qtables[i][j] * factor)));
            }
        }
        this->sampling[JPEG_MAX_COMPONENTS] = std::pair<int, int>(maxX, maxY);
        numMcus = std::pair<int, int>(std::ceil(size.first / maxX / JPEG_BLOCK_ROW), std::ceil(size.second / maxY / JPEG_BLOCK_ROW));
        std::copy(qIndices, qIndices + JPEG_MAX_COMPONENTS, this->qIndices);
}

Jpeg::Jpeg::Jpeg(const JpegSettings& jpegSettings) :
    settings {jpegSettings},
    blocks {new std::int16_t[jpegSettings.numMcus.first * jpegSettings.numMcus.second * jpegSettings.mcuSize][JPEG_BLOCK_SIZE]}
{
    
}

Jpeg::Jpeg::~Jpeg()
{
    delete[] blocks;
}
