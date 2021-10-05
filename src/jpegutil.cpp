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
        std::pair<int, int> size,
        const std::vector<JpegComponent> *components,
        JpegDensityUnits densityUnits,
        std::pair<int, int> density,
        int quality,
        int compressionFlags,
        int numQTables,
        const dqt_t *qtables[JPEG_MAX_COMPONENTS],
        std::pair<int, int> version,
        const codes_t *huffmanCodes,
        int bitDepth,
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
        if (components != nullptr) {
            this->components = *components;
        }
        else {
            JpegComponent defaultComponents[3] = {
                JpegComponent(std::pair<int, int>(2, 2), 0, 0, 0),
                JpegComponent(std::pair<int, int>(1, 1), 1, 1, 1),
                JpegComponent(std::pair<int, int>(1, 1), 1, 1, 1)
            };
            this->components = std::vector<JpegComponent>(defaultComponents, defaultComponents + 3);
        }
        if (huffmanCodes != nullptr) {
            this->huffmanCodes = *huffmanCodes;
        }
        for (int i = 0; i < numQTables; i++) {
            std::copy(qtables[i], qtables[i] + JPEG_BLOCK_SIZE, this->qtables[i]);
        }
        init();
}

// Jpeg::JpegSettings::JpegSettings(
        // std::pair<int, int> size,
        // JpegDensityUnits densityUnits,
        // std::pair<int, int> density,
        // int bitDepth,
        // int quality,
        // int compressionFlags,
        // int numQTables,
        // const int *qtables[JPEG_MAX_COMPONENTS],
        // std::pair<int, int> version,
        // int resetInterval) :
    // size {size},
    // densityUnits {densityUnits},
    // density {density},
    // bitDepth {bitDepth},
    // quality {quality},
    // compressionFlags {compressionFlags},
    // numQTables {numQTables},
    // resetInterval {resetInterval},
    // mcuSize {0},
    // version {version} {
        // JpegComponent defaultComponents[3] = {
            // JpegComponent(std::pair<int, int>(2, 2), 0, 0, 0),
            // JpegComponent(std::pair<int, int>(1, 1), 1, 1, 1),
            // JpegComponent(std::pair<int, int>(1, 1), 1, 1, 1)
        // };
        // components = std::vector<JpegComponent>(defaultComponents, defaultComponents + 3);
        // init();
// }

void Jpeg::JpegSettings::init()
{
    int maxX = 0, maxY = 0;
    float factor = (quality <= 50) ? 50.0 / quality : 1.0 / (quality - 50);
    // std::cout << "Factor=" << factor << std::endl;
    for (int i = 0; i < components.size(); i ++) {
        componentOffsets[i] = mcuSize;
        mcuSize += components[i].sampling.first * components[i].sampling.second;
        maxX = std::max(maxX, components[i].sampling.first);
        maxY = std::max(maxY, components[i].sampling.second);
    }
    for (int i = 0; i < numQTables; i++) {
        for (int j = 0; j < JPEG_BLOCK_SIZE; j++) {
            this->qtables[i][j] = std::max(1, std::min(255, (int)(qtables[i][j] * factor)));
            // std::cout << "Qtable " << i << ',' << j << ": " << this->qtables[i][j] << std::endl;
        }
    }
    mcuScale = std::pair<int, int>(maxX, maxY);
    numMcus = std::pair<int, int>(std::ceil((float)size.first / maxX / JPEG_BLOCK_ROW), std::ceil((float)size.second / maxY / JPEG_BLOCK_ROW));
}

Jpeg::Jpeg::Jpeg(const Jpeg& other) :
    settings {other.settings},
    blocks {new dct_t[other.settings.numMcus.first * other.settings.numMcus.second * other.settings.mcuSize][JPEG_BLOCK_SIZE]}
{
    std::copy(&other.blocks[0][0], &other.blocks[0][0] + JPEG_BLOCK_SIZE * settings.numMcus.first * settings.numMcus.second * settings.mcuSize, &blocks[0][0]);
}

Jpeg::Jpeg& Jpeg::Jpeg::operator=(const Jpeg& other)
{
    delete[] blocks;
    settings = other.settings;
    size_t size = settings.numMcus.first * settings.numMcus.second * settings.mcuSize;
    blocks = new dct_t[size][JPEG_BLOCK_SIZE];
    std::copy(&other.blocks[0][0], &other.blocks[0][0] + JPEG_BLOCK_SIZE * size, &blocks[0][0]);
    return *this;
}

Jpeg::Jpeg::~Jpeg()
{
    delete[] blocks;
}
