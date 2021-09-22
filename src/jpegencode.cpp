/*
jpegencode.cpp
*/

#include <iostream>
#include <cmath>
#include <cstdint>
#include "jpegutil.hpp"

/*
u-major, so given u and x, use the coefficient at 8u + x
*/
const float Jpeg::dctCoeffs[JPEG_DCT_COEFF_SIZE] = {
    1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    0.9807852804032304, 0.8314696123025452, 0.5555702330196023, 0.19509032201612833, -0.1950903220161282, -0.555570233019602, -0.8314696123025453, -0.9807852804032304,
    0.9238795325112867, 0.38268343236508984, -0.3826834323650897, -0.9238795325112867, -0.9238795325112868, -0.38268343236509034, 0.38268343236509, 0.9238795325112865,
    0.8314696123025452, -0.1950903220161282, -0.9807852804032304, -0.5555702330196022, 0.5555702330196018, 0.9807852804032304, 0.19509032201612878, -0.8314696123025451,
    0.7071067811865476, -0.7071067811865475, -0.7071067811865477, 0.7071067811865474, 0.7071067811865477, -0.7071067811865467, -0.7071067811865471, 0.7071067811865466,
    0.5555702330196023, -0.9807852804032304, 0.1950903220161283, 0.8314696123025456, -0.8314696123025451, -0.19509032201612803, 0.9807852804032307, -0.5555702330196015,
    0.38268343236508984, -0.9238795325112868, 0.9238795325112865, -0.3826834323650899, -0.38268343236509056, 0.9238795325112867, -0.9238795325112864, 0.38268343236508956,
    0.19509032201612833, -0.5555702330196022, 0.8314696123025456, -0.9807852804032307, 0.9807852804032305, -0.831469612302545, 0.5555702330196015, -0.19509032201612858
};

const size_t Jpeg::zigzag[JPEG_BLOCK_SIZE] = {
     0, 1, 8, 16, 9, 2, 3, 10,
     17, 24, 32, 25, 18, 11, 4, 5,
     12, 19, 26, 33, 40, 48, 41, 34,
     27, 20, 13, 6, 7, 14, 21, 28,
     35, 42, 49, 56, 57, 50, 43, 36,
     29, 22, 15, 23, 30, 37, 44, 51,
     58, 59, 52, 45, 38, 31, 39, 46,
     53, 60, 61, 54, 47, 55, 62, 63
};

const int Jpeg::defaultLuminanceQTable[JPEG_BLOCK_SIZE] = {
    16, 11, 10, 16,  24,  40,  51,  61,
    12, 12, 14, 19,  26,  58,  60,  55,
    14, 13, 16, 24,  40,  57,  69,  56,
    14, 17, 22, 29,  51,  87,  80,  62,
    18, 22, 37, 56,  68, 109, 103,  77,
    24, 35, 55, 64,  81, 104, 113,  92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103,  99
};

const int Jpeg::defaultChrominanceQTable[JPEG_BLOCK_SIZE] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

inline float accumRowRGB(std::uint8_t *rgb,
    size_t width, size_t height, size_t pitch, 
    size_t component, size_t bitDepth,
    int numX, int denX,
    size_t x0, size_t x, size_t y)
{
    float row = 0;
    float step = (float)denX / numX;
    float startX = x0 + x * step;
    float endX = startX + step;
    if ((x * denX) % numX != 0) {
        size_t coord = Jpeg::getPixelCoord(std::floor(startX), y, width, height, pitch);
        std::uint8_t *sample = rgb + 3 * coord;
        row += (Jpeg::componentFromRGB(sample, component)) * (std::ceil(startX) - startX);
    }
    for (size_t ix = std::ceil(startX); ix < std::floor(endX); ix++) {
        size_t coord = Jpeg::getPixelCoord(ix, y, width, height, pitch);
        std::uint8_t *sample = rgb + 3 * coord;
        row += Jpeg::componentFromRGB(sample, component);
    }
    if ((x * denX + denX) % numX != 0) {
        size_t coord = Jpeg::getPixelCoord(std::floor(endX), y, width, height, pitch);
        std::uint8_t *sample = rgb + 3 * coord;
        row += (Jpeg::componentFromRGB(sample, component)) * (endX - std::ceil(endX));
    }
    return row / step;
}

inline float accumBlockRGB(std::uint8_t *rgb,
    size_t width, size_t height, size_t pitch, 
    size_t component, size_t bitDepth,
    int numX, int denX,
    int numY, int denY,
    size_t x0, size_t y0,
    size_t x, size_t y)
{
    float block = 0;
    float step = (float)denY / numY;
    float startY = y0 + y * step;
    float endY = startY + step;
    if ((y * denY) % numY != 0) {
        block += (std::ceil(startY) - startY) * accumRowRGB(rgb, width, height, pitch, component, bitDepth, numX, denX, x0, x, std::floor(startY));
    }
    for (size_t iy = std::ceil(startY); iy < std::floor(endY); iy++) {
        block += accumRowRGB(rgb, width, height, pitch, component, bitDepth, numX, denX, x0, x, iy);
    }
    if ((y * denY + denY) % numY != 0) {
        block += (endY - std::ceil(endY)) * accumRowRGB(rgb, width, height, pitch, component, bitDepth, numX, denX, x0, x, std::floor(endY));
    }
    return block / step;
}

const float inverseSqrtTwo = 0.7071067811865476;

void DCT8(std::int16_t *data, size_t stride)
{
    std::int16_t buffer[JPEG_DCT_SIZE];
    for(size_t u = 0; u < JPEG_DCT_SIZE; u++) {
        float point = 0;
        for(size_t x = 0; x < JPEG_DCT_SIZE; x++) {
            point += data[x * stride] * Jpeg::dctCoeffs[u * 8 + x];
        }
        buffer[u] = std::round(point) / 2;
    }
    buffer[0] *= inverseSqrtTwo;
    for (size_t i = 0; i < JPEG_DCT_SIZE; i++) {
        data[i * stride] = buffer[i];
    }
}

void Jpeg::Jpeg::encodeRGB(std::uint8_t *rgb)
{
    int denX = settings.sampling[JPEG_MAX_COMPONENTS].first;
    int denY = settings.sampling[JPEG_MAX_COMPONENTS].second;
    /* Size of each MCU in pixels */
    size_t mcuWidth = denX * JPEG_BLOCK_ROW;
    size_t mcuHeight = denY * JPEG_BLOCK_ROW;
    size_t rowPitch = mcuWidth * settings.numMcus.first;
    
    std::int16_t tBlock[JPEG_BLOCK_SIZE];
    
    /* Iterate each MCU */
    for (size_t yMcu = 0; yMcu < settings.numMcus.second; yMcu++) {
    for (size_t xMcu = 0; xMcu < settings.numMcus.first; xMcu++) {
        size_t mcuInputStartY = yMcu * mcuHeight;
        size_t mcuInputStartX = xMcu * mcuWidth;
        size_t mcuOutputStart = settings.mcuSize * (yMcu * settings.numMcus.first + xMcu);
        /* Iterate each component */
        for (size_t iComp = 0; iComp < settings.numComponents; iComp++) {
            int numX = settings.sampling[iComp].first;
            int numY = settings.sampling[iComp].second;
            /* Number of pixels of the input image to be scanned over for each block in this component */
            size_t blockWidth = JPEG_BLOCK_ROW * numX / denX;
            size_t blockHeight = JPEG_BLOCK_ROW * numY / denY;
            size_t compOutputStart = settings.componentOffsets[iComp] + mcuOutputStart;
            const int *qTable = settings.qtables[settings.qIndices[iComp]];
            /* Iterate each block */
            for (size_t yBlock = 0; yBlock < numY; yBlock++) {
            for (size_t xBlock = 0; xBlock < numX; xBlock++) {
                size_t blockInputStartY = yBlock * blockHeight + mcuInputStartY;
                size_t blockInputStartX = xBlock * blockWidth + mcuInputStartX;
                size_t blockNum = yBlock * numX + xBlock + compOutputStart;
                /* Iterate over each output sample */
                for (size_t ox = 0; ox < JPEG_BLOCK_ROW; ox++) {
                for (size_t oy = 0; oy < JPEG_BLOCK_ROW; oy++) {
                    int sample = (int)accumBlockRGB(rgb,
                        settings.size.first, settings.size.second, rowPitch,
                        iComp, settings.bitDepth,
                        numX, denX, numY, denY,
                        blockInputStartX, blockInputStartY, ox, oy);
                    sample = std::max(0, std::min((1 << settings.bitDepth) - 1, sample));
                    sample -= 1 << (settings.bitDepth - 1);
                    tBlock[oy * JPEG_BLOCK_ROW + ox] = sample;
                }
                }
                /* Row-wise DCTs */
                for (size_t i = 0; i < JPEG_BLOCK_ROW; i++) {
                    DCT8(tBlock + i * JPEG_BLOCK_ROW, 1);
                }
                /* Column-wise DCTs */
                for (size_t i = 0; i < JPEG_BLOCK_ROW; i++) {
                    DCT8(tBlock + i, JPEG_BLOCK_ROW);
                }
                /* Copy zigzagged and quantized to the block */
                for (size_t i = 0; i < JPEG_BLOCK_SIZE; i++) {
                    size_t index = zigzag[i];
                    blocks[blockNum][i] = (std::int16_t)std::round((float)tBlock[index] / qTable[index]);
                }
            }
            }
        }
    }
    }
}

void Jpeg::Jpeg::encodeDeltas()
{
    /* Iterate every component */
    for (size_t iComp = 0; iComp < settings.numComponents; iComp++) {
        int predictor = 0;
        /* Iterate over every MCU */
        for (size_t iMcu = 0; iMcu < settings.numMcus.first * settings.numMcus.second; iMcu++) {
            if (settings.resetInterval != 0 && iMcu % settings.resetInterval == 0) {
                predictor = 0;
            }
            /* Iterate over every block in MCU of this component */
            size_t numBlocks = settings.sampling[iComp].first * settings.sampling[iComp].second;
            for (size_t iBlock = 0; iBlock < numBlocks; iBlock++) {
                std::int16_t *block = blocks[iBlock + settings.componentOffsets[iComp] + iMcu * settings.mcuSize];
                int delta = block[0] - predictor;
                predictor = block[0];
                block[0] = delta;
            }
        }
    }
}