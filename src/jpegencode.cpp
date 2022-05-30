/*
jpegencode.cpp
*/

#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdint>
// #include <endian.h>
#include <climits>
#include <omp.h>
#include "bitutil.hpp"
#include "jpegutil.hpp"

#define SYMBOL_LENGTHS 16
#define SYMBOL_CAP 256

static Jpeg::codes_t defaultEncodingCodes;
const std::int16_t defaultDcLuminance[SYMBOL_LENGTHS][SYMBOL_CAP] = {
    {-1},
    {0, -1},
    {1, 2, 3, 4, 5, -1},
    {6, -1},
    {7, -1},
    {8, -1},
    {9, -1},
    {10, -1},
    {11, -1},
    {-1},
    {-1},
    {-1},
    {-1},
    {-1},
    {-1},
    {-1}
};
const std::int16_t defaultDcChrominance[SYMBOL_LENGTHS][SYMBOL_CAP] = {
    {-1},
    {0, 1, 2, -1},
    {3, -1},
    {4, -1},
    {5, -1},
    {6, -1},
    {7, -1},
    {8, -1},
    {9, -1},
    {10, -1},
    {11, -1},
    {-1},
    {-1},
    {-1},
    {-1},
    {-1}
};
const std::int16_t defaultAcLuminance[SYMBOL_LENGTHS][SYMBOL_CAP] = {
    {-1},
    {0x01, 0x02, -1},
    {0x03, -1},
    {0x00, 0x04, 0x11, -1},
    {0x05, 0x12, 0x21, -1},
    {0x31, 0x41, -1},
    {0x06, 0x13, 0x51, 0x61, -1},
    {0x07, 0x22, 0x71, -1},
    {0x14, 0x32, 0x81, 0x91, 0xA1, -1},
    {0x08, 0x23, 0x42, 0xB1, 0xC1, -1},
    {0x15, 0x52, 0xD1, 0xF0, -1},
    {0x24, 0x33, 0x62, 0x72, -1},
    {-1},
    {-1},
    {0x82, -1},
    {0x09, 0x0A,
     0x16, 0x17, 0x18, 0x19, 0x1A,
     0x25, 0x26, 0x27, 0x28, 0x29, 0x2A,
     0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
     0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
     0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,
     0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
     0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x78, 0x7A,
     0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A,
     0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A,
     0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA,
     0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA,
     0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA,
     0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
     0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
     0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,
    -1}
};
const std::int16_t defaultAcChrominance[SYMBOL_LENGTHS][SYMBOL_CAP] = {
    {-1},
    {0x00, 0x01, -1},
    {0x02, -1},
    {0x03, 0x11, -1},
    {0x04, 0x05, 0x21, 0x31, -1},
    {0x06, 0x12, 0x41, 0x51, -1},
    {0x07, 0x61, 0x71, -1},
    {0x13, 0x22, 0x32, 0x81, -1},
    {0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, -1},
    {0x09, 0x23, 0x33, 0x52, 0xF0, -1},
    {0x15, 0x62, 0x72, 0xD1, -1},
    {0x0A, 0x16, 0x24, 0x34, -1},
    {-1},
    {0xE1, -1},
    {0x25, 0xF1, -1},
    {0x17, 0x18, 0x19, 0x1A,
     0x26, 0x27, 0x28, 0x29, 0x2A,
     0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
     0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A,
     0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,
     0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A,
     0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x78, 0x7A,
     0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A,
     0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A,
     0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA,
     0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA,
     0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA,
     0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
     0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
     0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA,
    -1}
};

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

const Jpeg::dqt_t Jpeg::defaultLuminanceQTable[JPEG_BLOCK_SIZE] = {
    16, 11, 10, 16,  24,  40,  51,  61,
    12, 12, 14, 19,  26,  58,  60,  55,
    14, 13, 16, 24,  40,  57,  69,  56,
    14, 17, 22, 29,  51,  87,  80,  62,
    18, 22, 37, 56,  68, 109, 103,  77,
    24, 35, 55, 64,  81, 104, 113,  92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103,  99
};

const Jpeg::dqt_t Jpeg::defaultChrominanceQTable[JPEG_BLOCK_SIZE] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

inline float accumRowRGB(const std::uint8_t *rgb,
    size_t width, size_t height,
    size_t component, size_t bitDepth,
    int numX, int denX,
    size_t x0, size_t x, size_t y)
{
    float row = 0;
    float step = (float)denX / numX;
    float startX = x0 + x * step;
    float endX = startX + step;
    if ((x * denX) % numX != 0) {
        size_t coord = Jpeg::getPixelCoord(std::floor(startX), y, width, height);
        const std::uint8_t *sample = rgb + 3 * coord;
        row += (Jpeg::componentFromRGB(sample, component)) * (std::ceil(startX) - startX);
    }
    for (size_t ix = std::ceil(startX); ix < std::floor(endX); ix++) {
        size_t coord = Jpeg::getPixelCoord(ix, y, width, height);
        const std::uint8_t *sample = rgb + 3 * coord;
        row += Jpeg::componentFromRGB(sample, component);
    }
    if ((x * denX + denX) % numX != 0) {
        size_t coord = Jpeg::getPixelCoord(std::floor(endX), y, width, height);
        const std::uint8_t *sample = rgb + 3 * coord;
        row += (Jpeg::componentFromRGB(sample, component)) * (endX - std::ceil(endX));
    }
    return row / step;
}

inline float accumRowRGBi(const std::uint8_t *rgb,
    size_t width, size_t height,
    size_t component, size_t bitDepth,
    int numX, int denX,
    size_t x0, size_t x, size_t y)
{
    float row = 0;
    int step = denX / numX;
    size_t startX = x0 + x * step;
    size_t endX = startX + step;
    for (size_t ix = startX; ix < endX; ix++) {
        size_t coord = Jpeg::getPixelCoord(ix, y, width, height);
        const std::uint8_t *sample = rgb + 3 * coord;
        row += Jpeg::componentFromRGB(sample, component);
    }
    return row / step;
}

inline float accumBlockRGB(const std::uint8_t *rgb,
    size_t width, size_t height, 
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
        block += (std::ceil(startY) - startY) * accumRowRGB(rgb, width, height, component, bitDepth, numX, denX, x0, x, std::floor(startY));
    }
    for (size_t iy = std::ceil(startY); iy < std::floor(endY); iy++) {
        block += accumRowRGB(rgb, width, height, component, bitDepth, numX, denX, x0, x, iy);
    }
    if ((y * denY + denY) % numY != 0) {
        block += (endY - std::ceil(endY)) * accumRowRGB(rgb, width, height, component, bitDepth, numX, denX, x0, x, std::floor(endY));
    }
    return block / step;
}

inline float accumBlockRGBi(const std::uint8_t *rgb,
    size_t width, size_t height, 
    size_t component, size_t bitDepth,
    int numX, int denX,
    int numY, int denY,
    size_t x0, size_t y0,
    size_t x, size_t y)
{
    float block = 0;
    int step = denY / numY;
    size_t startY = y0 + y * step;
    size_t endY = startY + step;
    for (size_t iy = startY; iy < endY; iy++) {
        block += accumRowRGBi(rgb, width, height, component, bitDepth, numX, denX, x0, x, iy);
    }
    return block / step;
}

const float inverseSqrtTwo = 0.7071067811865476;

const static float dct8_scales[8] = {
	0.353553390593273762200422,
	0.254897789552079584470970,
	0.270598050073098492199862,
	0.300672443467522640271861,
	0.353553390593273762200422,
	0.449988111568207852319255,
	0.653281482438188263928322,
	1.281457723870753089398043,
};

const static float dct8_consts[5] = {
	0.707106781186547524400844,
	0.541196100146196984399723,
	0.707106781186547524400844,
	1.306562964876376527856643,
	0.382683432365089771728460,
};

void DCT8(Jpeg::dct_t *data, size_t stride)
{
    // Jpeg::dct_t buffer[JPEG_DCT_SIZE];
    // for(size_t u = 0; u < JPEG_DCT_SIZE; u++) {
        // float point = 0;
        // for(size_t x = 0; x < JPEG_DCT_SIZE; x++) {
            // point += data[x * stride] * Jpeg::dctCoeffs[u * 8 + x];
        // }
        // buffer[u] = std::round(point) / 2;
    // }
    // buffer[0] *= inverseSqrtTwo;
    // for (size_t i = 0; i < JPEG_DCT_SIZE; i++) {
        // data[i * stride] = buffer[i];
    // }
    // return;
    
    // Idea from https://web.stanford.edu/class/ee398a/handouts/lectures/07-TransformCoding.pdf#page=30
    float v0 = data[0 * stride] + data[7 * stride];
    float v1 = data[1 * stride] + data[6 * stride];
    float v2 = data[2 * stride] + data[5 * stride];
    float v3 = data[3 * stride] + data[4 * stride];
    float v4 = data[3 * stride] - data[4 * stride];
    float v5 = data[2 * stride] - data[5 * stride];
    float v6 = data[1 * stride] - data[6 * stride];
    float v7 = data[0 * stride] - data[7 * stride];
    
    float w0 = v0 + v3;
    float w1 = v1 + v2;
    float w2 = v1 - v2;
    float w3 = v0 - v3;
    float w4 = -(v4 + v5);
    float w5 = v5 + v6;
    float w6 = v6 + v7;
    float w7 = v7;
    
    v0 = w0 + w1;
    v1 = w0 - w1;
    v2 = w2 + w3;
    v3 = w3;
    v4 = w4;
    v5 = w5;
    v6 = w6;
    v7 = w7;
    
    float y = (v4 + v6) * dct8_consts[4];
    
    w0 = v0;
    w1 = v1;
    w2 = v2 * dct8_consts[0];
    w3 = v3;
    w4 = -y - v4 * dct8_consts[1];
    w5 = v5 * dct8_consts[2];
    w6 = v6 * dct8_consts[3] - y;
    w7 = v7;
    
    v0 = w0;
    v1 = w1;
    v2 = w2 + w3;
    v3 = w3 - w2;
    v4 = w4;
    v5 = w5 + w7;
    v6 = w6;
    v7 = w7 - w5;
    
    w0 = v0;
    w1 = v1;
    w2 = v2;
    w3 = v3;
    w4 = v4 + v7;
    w5 = v5 + v6;
    w6 = v5 - v6;
    w7 = v7 - v4;
    
    data[0 * stride] = dct8_scales[0] * w0;
    data[4 * stride] = dct8_scales[4] * w1;
    data[2 * stride] = dct8_scales[2] * w2;
    data[6 * stride] = dct8_scales[6] * w3;
    data[5 * stride] = dct8_scales[5] * w4;
    data[1 * stride] = dct8_scales[1] * w5;
    data[7 * stride] = dct8_scales[7] * w6;
    data[3 * stride] = dct8_scales[3] * w7;
}

void Jpeg::Jpeg::encodeRGB(const std::uint8_t *rgb)
{
    int denX = settings.mcuScale.first;
    int denY = settings.mcuScale.second;
    /* Size of each MCU in pixels */
    size_t mcuWidth = denX * JPEG_BLOCK_ROW;
    size_t mcuHeight = denY * JPEG_BLOCK_ROW;
    size_t rowPitch = mcuWidth * settings.numMcus.first;
    
    
    /* Iterate each MCU */
    #pragma omp parallel for collapse(2)
    for (size_t yMcu = 0; yMcu < settings.numMcus.second; yMcu++) {
    for (size_t xMcu = 0; xMcu < settings.numMcus.first; xMcu++) {
        dct_t tBlock[JPEG_BLOCK_SIZE];
        size_t mcuInputStartY = yMcu * mcuHeight;
        size_t mcuInputStartX = xMcu * mcuWidth;
        size_t mcuOutputStart = settings.mcuSize * (yMcu * settings.numMcus.first + xMcu);
        /* Iterate each component */
        for (size_t iComp = 0; iComp < settings.components.size(); iComp++) {
            int numX = settings.components[iComp].sampling.first;
            int numY = settings.components[iComp].sampling.second;
            auto accumFunc = accumBlockRGB;
            if ((denX % numX == 0) && (denY % numY == 0)) {
                accumFunc = accumBlockRGBi;
            }
            /* Number of pixels of the input image to be scanned over for each block in this component */
            size_t blockWidth = JPEG_BLOCK_ROW * numX / denX;
            size_t blockHeight = JPEG_BLOCK_ROW * numY / denY;
            size_t compOutputStart = settings.componentOffsets[iComp] + mcuOutputStart;
            const dqt_t *qTable = settings.qtables[settings.components[iComp].qtable];
            /* Iterate each block */
            for (size_t yBlock = 0; yBlock < numY; yBlock++) {
            for (size_t xBlock = 0; xBlock < numX; xBlock++) {
                size_t blockInputStartY = yBlock * blockHeight + mcuInputStartY;
                size_t blockInputStartX = xBlock * blockWidth + mcuInputStartX;
                size_t blockNum = yBlock * numX + xBlock + compOutputStart;
                /* Iterate over each output sample */
                for (size_t ox = 0; ox < JPEG_BLOCK_ROW; ox++) {
                for (size_t oy = 0; oy < JPEG_BLOCK_ROW; oy++) {
                    dct_t sample = (dct_t)std::round(accumFunc(rgb,
                        settings.size.first, settings.size.second,
                        iComp, settings.bitDepth,
                        numX, denX, numY, denY,
                        blockInputStartX, blockInputStartY, ox, oy));
                    sample = std::max(dct_t{0}, std::min((dct_t)(1 << settings.bitDepth) - 1, sample));
                    sample -= 1 << (settings.bitDepth - 1);
                    const size_t index = oy * JPEG_BLOCK_ROW + ox;
                    // std::cout << index << ": " << sample << std::endl;
                    tBlock[index] = sample;
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
                    blocks[blockNum][i] = (dct_t)std::round((float)tBlock[index] / qTable[index]);
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
    #pragma omp parallel for
    for (size_t iComp = 0; iComp < settings.components.size(); iComp++) {
        dct_t predictor = 0;
        /* Iterate over every MCU */
        for (size_t iMcu = 0; iMcu < settings.numMcus.first * settings.numMcus.second; iMcu++) {
            if (settings.resetInterval != 0 && iMcu % settings.resetInterval == 0) {
                predictor = 0;
            }
            /* Iterate over every block in MCU of this component */
            size_t numBlocks = settings.components[iComp].sampling.first * settings.components[iComp].sampling.second;
            for (size_t iBlock = 0; iBlock < numBlocks; iBlock++) {
                volatile dct_t *block = blocks[iBlock + settings.componentOffsets[iComp] + iMcu * settings.mcuSize];
                dct_t delta = block[0] - predictor;
                predictor = block[0];
                block[0] = delta;
            }
        }
    }
}

using split_t = std::pair<std::uint8_t, std::uint16_t>;

split_t splitNumber(Jpeg::dct_t number)
{
    if (number == 0) {
        return split_t(0, 0);
    }
    std::uint16_t anum = number;
    if (number < 0) {
        anum = -anum;
        number--;
    }
    std::uint8_t bits = BitManip::msbSet(anum) + 1;
    anum = number & ((1 << bits) - 1);
    return split_t(bits, anum);
}

using block_t = std::vector<split_t>;
using mcu_t = std::vector<block_t>;

void createJpegHuffmanCodes(
    Jpeg::codes_t& codeList,
    std::vector<mcu_t>& mcus,
    Jpeg::JpegSettings& settings)
{
    size_t maxDc = 0, maxAc = 0;
    for (auto it = settings.components.begin(); it != settings.components.end(); it++) {
        maxDc = std::max(maxDc, it->dcTable);
        maxAc = std::max(maxAc, it->acTable);
    }
    maxDc++;
    maxAc++;
    using ftable_t = std::map<int, int>;
    using ftables = std::vector<ftable_t>;
    ftables dcFreq(maxDc, ftable_t()), acFreq(maxAc, ftable_t());
    ftable_t *dcTable, *acTable;
    for (auto itMcu = mcus.begin(); itMcu != mcus.end(); itMcu++) {
        size_t compP1 = 0; // Current component plus 1
        mcu_t& mcu = *itMcu;
        for (size_t iBlock = 0; iBlock < settings.mcuSize; iBlock++) {
            if (compP1 < settings.components.size() && iBlock == settings.componentOffsets[compP1]) {
                compP1++;
                dcTable = &(dcFreq[settings.components[compP1 - 1].dcTable]);
                acTable = &(acFreq[settings.components[compP1 - 1].acTable]);
            }
            block_t& block = mcu[iBlock];
            /* Get DC */
            split_t dc = block[0];
            dcTable->insert(std::pair<int, int>(dc.first, 1));
            (*dcTable)[dc.first]++;
            /* Get AC */
            for (size_t i = 1; i < block.size(); i++) {
                split_t ac = block[i];
                acTable->insert(std::pair<int, int>(ac.first, 1));
                (*acTable)[ac.first]++;
            }
        }
    }
    codeList.first.clear();
    for (auto it = dcFreq.begin(); it != dcFreq.end(); it++) {
        // for (int i = 0; i <= 11; i++) {
            // it->insert(std::pair<int, int>(i, 1));
        // }
        it->insert(std::pair<int, int>(INT_MAX, 0));
        codeList.first.push_back(Huffman::HuffmanCode(*it, 16));
    }
    codeList.second.clear();
    for (auto it = acFreq.begin(); it != acFreq.end(); it++) {
        // for (int i = 0; i < 16; i++) {
            // for (int j = 1; j <= 10; j++) {
                // it->insert(std::pair<int, int>((i << 4) | j, 1));
            // }
        // }
        // it->insert(std::pair<int, int>(0, 1));
        // it->insert(std::pair<int, int>(0xF0, 1));
        it->insert(std::pair<int, int>(INT_MAX, 0));
        codeList.second.push_back(Huffman::HuffmanCode(*it, 16));
    }
}

Huffman::HuffmanCode fromDefault(const std::int16_t table[SYMBOL_LENGTHS][SYMBOL_CAP])
{
    std::vector<std::vector<int>> symbolsList;
    symbolsList.reserve(SYMBOL_LENGTHS);
    for (size_t i = 0; i < SYMBOL_LENGTHS; i++) {
        std::vector<int> ofLength;
        for (size_t j = 0; j < SYMBOL_CAP; j++) {
            if (table[i][j] == -1) {
                break;
            }
            ofLength.push_back(table[i][j]);
        }
        symbolsList.push_back(ofLength);
    }
    return Huffman::HuffmanCode(symbolsList);
}

void setupDefaultEncodingCodes()
{
    std::vector<Huffman::HuffmanCode>& dcTables = defaultEncodingCodes.first;
    if (dcTables.empty()) {
        dcTables.push_back(fromDefault(defaultDcLuminance));
        dcTables.push_back(fromDefault(defaultDcChrominance));
    }
    std::vector<Huffman::HuffmanCode>& acTables = defaultEncodingCodes.second;
    if (acTables.empty()) {
        acTables.push_back(fromDefault(defaultAcLuminance));
        acTables.push_back(fromDefault(defaultAcChrominance));
        
    }
}

void Jpeg::Jpeg::encodeCompressed(BitBuffer::BitBufferOut& dst)
{
    std::vector<mcu_t> mcus;
    size_t numMcus = settings.numMcus.first * settings.numMcus.second;
    mcus.reserve(numMcus);
    for (size_t iMcu = 0; iMcu < numMcus; iMcu++) {
        mcu_t mcu;
        mcu.reserve(settings.mcuSize);
        for (size_t iBlock = 0; iBlock < settings.mcuSize; iBlock++) {
            size_t blockNum = iMcu * settings.mcuSize + iBlock;
            block_t block;
            /* DC component */
            block.push_back(splitNumber(blocks[blockNum][0]));
            /* AC components */
            size_t leadingZeros = 0;
            size_t lastInserted = 1;
            for (size_t i = 1; i < JPEG_BLOCK_SIZE; i++) {
                if (blocks[blockNum][i] != 0) {
                    while (leadingZeros > 15) {
                        block.push_back(split_t(0xF0, 0));
                        leadingZeros -= 16;
                    }
                    split_t entry = splitNumber(blocks[blockNum][i]);
                    entry.first |= leadingZeros << 4;
                    leadingZeros = 0;
                    block.push_back(entry);
                    lastInserted = i + 1;
                }
                else {
                    leadingZeros++;
                }
            }
            if (blocks[blockNum][JPEG_BLOCK_SIZE - 1] == 0) {
                block.push_back(split_t(0, 0));
            }
            mcu.push_back(block);
        }
        mcus.push_back(mcu);
    }
    
    /* Get appropriate huffman codes */
    codes_t& huffmanCodes = settings.huffmanCodes;
    switch ((settings.compressionFlags & flagHuffmanMask)) {
        case flagHuffmanOptimal:
            createJpegHuffmanCodes(huffmanCodes, mcus, settings);
            break;
        case flagHuffmanDefault:
            setupDefaultEncodingCodes();
            huffmanCodes = defaultEncodingCodes;
    }
    
    size_t maxDc = 0, maxAc = 0;
    for (auto it = settings.components.begin(); it != settings.components.end(); it++) {
        JpegComponent& comp = *it;
        maxDc = std::max(maxDc, comp.dcTable);
        maxAc = std::max(maxAc, comp.acTable);
    }
    maxDc++;
    maxAc++;
    if (maxDc > huffmanCodes.first.size()) {
        throw JpegEncodingException("Not enough DC Huffman codes");
    }
    if (maxAc > huffmanCodes.first.size()) {
        throw JpegEncodingException("Not enough AC Huffman codes");
    }
    
    std::stringstream strstream;
    BitBuffer::BitBufferOut bout(strstream);
    
    for (size_t iMcu = 0; iMcu < settings.numMcus.first * settings.numMcus.second; iMcu++) {
        mcu_t& mcu = mcus[iMcu];
        size_t compP1 = 0;
        Huffman::HuffmanCode *dcTable, *acTable;
        for (size_t iBlock = 0; iBlock < settings.mcuSize; iBlock++) {
            if (compP1 < settings.components.size() && iBlock == settings.componentOffsets[compP1]) {
                compP1 += 1;
                dcTable = &(huffmanCodes.first[settings.components[compP1 - 1].dcTable]);
                acTable = &(huffmanCodes.second[settings.components[compP1 - 1].acTable]);
            }
            block_t& block = mcu[iBlock];
            
            /* Write DC */
            split_t& dc = block[0];
            // std::cout << "DC: " << (int)dc.first << ',' << dc.second << std::endl;
            dcTable->write(dc.first, bout);
            if (dc.first != 0) {
                bout.write(dc.second, dc.first);
            }
            
            /* Write AC */
            for (size_t i = 1; i < block.size(); i++) {
                split_t &ac = block[i];
                // std::cout << "AC: " << (int)ac.first << ',' << ac.second << std::endl;
                acTable->write(ac.first, bout);
                if ((ac.first & 0xF) != 0) {
                    bout.write(ac.second, ac.first & 0xF);
                }
            }
        }
    }
    
    bout.flush(true);
    
    /* Transfer from temp buffer to output while replacing 0xFF with 0xFF 0x00 */
    std::string src = strstream.str();
    const std::uint8_t *srcDat = reinterpret_cast<const std::uint8_t*>(src.data());
    for (size_t i = 0; i < src.size(); i++) {
        dst.write(srcDat[i], 8);
        if (srcDat[i] == 0xFF) {
            dst.write(0, 8);
        }
    }
}

void writeBe16(std::uint16_t num, std::ostream& dst)
{
    dst.put((std::uint8_t)(num >> 8));
    dst.put((std::uint8_t)num);
}

void Jpeg::Jpeg::write(std::ostream& dst)
{
    std::stringstream temp;
    BitBuffer::BitBufferOut bbo(temp);
    encodeDeltas();
    encodeCompressed(bbo);
    bbo.flush(true);
    
    dst.write(reinterpret_cast<const char*>((const unsigned char[]){
            0xFF, 0xD8, 0xFF, 0xE0, // SOI, APP0
            0x00, 0x10, // length, Version
            'J', 'F', 'I', 'F', 0
        }), 11);
    dst.put(settings.version.first);
    dst.put(settings.version.second);
    dst.put(settings.densityUnits);
    writeBe16(settings.density.first, dst);
    writeBe16(settings.density.second, dst);
    dst.write(reinterpret_cast<const char*>((const unsigned char[]){0, 0}), 2); // Thumbnail size
    
    for (size_t i = 0; i < settings.numQTables; i++) {
        const dqt_t *qtable = settings.qtables[i];
        dst.write(reinterpret_cast<const char*>((const unsigned char[]){
            0xFF, 0xDB, 0x00, 0x43 // DQT, length
        }), 4);
        dst.put(i);
        for (size_t j = 0; j < JPEG_BLOCK_SIZE; j++) {
            dst.put(qtable[j]);
        }
    }
    
    dst.write(reinterpret_cast<const char*>((const unsigned char[]){0xFF, 0xC0}), 2); // SOF
    writeBe16(8 + 3 * settings.components.size(), dst); // Length
    dst.put(8); // Precision
    writeBe16(settings.size.second, dst); // Height
    writeBe16(settings.size.first, dst); // Width
    dst.put(settings.components.size()); // Num components
    for (size_t i = 0; i < settings.components.size(); i++) {
        dst.put(i + 1);
        auto component = settings.components[i];
        dst.put((component.sampling.first << 4) | component.sampling.second);
        dst.put(component.qtable);
    }
    
    codes_t& hc = settings.huffmanCodes;
    for (size_t i = 0; i < hc.first.size(); i++) {
        std::vector<size_t> lengths = hc.first[i].lengthCounts();
        size_t length = 3 + 16;
        if ((settings.compressionFlags & flagHuffmanMask) == flagHuffmanOptimal) {
            lengths[lengths.size() - 1]--;
        }
        for (auto it2 = lengths.begin(); it2 != lengths.end(); it2++) {
            length += *it2;
        }
        dst.write(reinterpret_cast<const char*>((const unsigned char[]){0xFF, 0xC4}), 2); // DHT
        writeBe16(length, dst); // Length
        dst.put(i); // ID (class = 0 for DC)
        for (size_t j = 0; j < 16; j++) {
            if (j < lengths.size()) {
                dst.put(lengths[j]);
            }
            else {
                dst.put(0);
            }
        }
        std::vector<std::vector<int>> byLength = hc.first[i].orderedSymbols();
        for (auto it2 = byLength.begin(); it2 != byLength.end(); it2++) {
            // std::cout << "lcodes:\n";
            for (auto it3 = it2->begin(); it3 != it2->end(); it3++) {
                if (*it3 != INT_MAX) {
                    dst.put(*it3);
                    // std::cout << *it3 << ", ";
                }
                // else {
                    // std::cout << "Max int present\n";
                // }
            }
            // std::cout << std::endl;
        }
    }
    for (size_t i = 0; i < hc.second.size(); i++) {
        std::vector<size_t> lengths = hc.second[i].lengthCounts();
        size_t length = 3 + 16;
        if ((settings.compressionFlags & flagHuffmanMask) == flagHuffmanOptimal) {
            lengths[lengths.size() - 1]--;
        }
        for (auto it2 = lengths.begin(); it2 != lengths.end(); it2++) {
            length += *it2;
        }
        dst.write(reinterpret_cast<const char*>((const unsigned char[]){0xFF, 0xC4}), 2); // DHT
        writeBe16(length, dst); // Length
        dst.put(0x10 | i); // ID (class = 0 for DC)
        for (size_t j = 0; j < 16; j++) {
            if (j < lengths.size()) {
                dst.put(lengths[j]);
            }
            else {
                dst.put(0);
            }
        }
        std::vector<std::vector<int>> byLength = hc.second[i].orderedSymbols();
        for (auto it2 = byLength.begin(); it2 != byLength.end(); it2++) {
            for (auto it3 = it2->begin(); it3 != it2->end(); it3++) {
                if (*it3 != INT_MAX) {
                    dst.put(*it3);
                    // std::cout << *it3 << ", ";
                }
                // else {
                    // std::cout << "Max int present\n";
                // }
            }
        }
    }
    
    dst.write(reinterpret_cast<const char*>((const unsigned char[]){0xFF, 0xDA}), 2); // SOS
    writeBe16(6 + 2 * settings.components.size(), dst); // Length
    dst.put(settings.components.size());
    for (size_t i = 0; i < settings.components.size(); i++) {
        JpegComponent& comp = settings.components[i];
        dst.put(i + 1);
        dst.put((comp.dcTable << 4) | comp.acTable);
    }
    dst.write(reinterpret_cast<const char*>((const unsigned char[]){0x00, 0x3F, 0x00}), 3); // Spec/succ, unused

    dst.write(temp.str().data(), temp.str().size());
    
    dst.write(reinterpret_cast<const char*>((const unsigned char[]){0xFF, 0xD9}), 2); // EOI
}
