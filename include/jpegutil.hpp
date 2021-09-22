/*
jpegutil.hpp
*/

#ifndef _JPEGUTIL_HPP
#define _JPEGUTIL_HPP

#include <utility>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <algorithm>

#define JPEG_MAX_COMPONENTS 5

#define JPEG_FLAG_OPTIMIZE_HUFFMAN_CODES 1

#define JPEG_BLOCK_SIZE 64
#define JPEG_BLOCK_ROW 8
#define JPEG_DCT_COEFF_SIZE 64
#define JPEG_DCT_SIZE 8

namespace Jpeg {
    
    extern const int defaultLuminanceQTable[JPEG_BLOCK_SIZE];
    extern const int defaultChrominanceQTable[JPEG_BLOCK_SIZE];
    extern const float dctCoeffs[JPEG_DCT_COEFF_SIZE];
    extern const size_t zigzag[JPEG_BLOCK_SIZE];

    enum JpegDensityUnits {
        DPI = 1,
        DPCM = 2,
        RELATIVE = 0
    };
    
    struct JpegComponent {
        public:
            std::pair<int, int> sampling;
            size_t qtable;
            size_t dcTable;
            size_t acTable;
    };

    /*
    Data object to hold settings for JPEG encoding and metadata
    */
    struct JpegSettings {
        public:
            int numComponents;
            std::pair<int, int> sampling[JPEG_MAX_COMPONENTS + 2]; // Final 2 store the maxima and gcd of ratios
            std::pair<int, int> size;
            JpegDensityUnits densityUnits;
            std::pair<int, int> density;
            int bitDepth;
            int quality;
            int compressionFlags;
            int numQTables;
            int qtables[JPEG_MAX_COMPONENTS][JPEG_BLOCK_SIZE];
            size_t qIndices[JPEG_MAX_COMPONENTS];
            std::pair<int, int> version;
            int resetInterval;

            std::pair<int, int> numMcus;
            size_t componentOffsets[JPEG_MAX_COMPONENTS];
            size_t mcuSize;
            
            JpegSettings(int numComponents,
                std::pair<int, int> sampling[JPEG_MAX_COMPONENTS],
                std::pair<int, int> size,
                JpegDensityUnits densityUnits = DPI,
                std::pair<int, int> density = std::pair<int, int>(1, 1),
                int bitDepth = 8,
                int quality = 50,
                int compressionFlags = 0,
                int numQTables = 2,
                const int *qtables[JPEG_MAX_COMPONENTS] = (const int* [JPEG_MAX_COMPONENTS]){defaultLuminanceQTable, defaultChrominanceQTable, nullptr},size_t qIndices[JPEG_MAX_COMPONENTS] = (size_t[JPEG_MAX_COMPONENTS]){0, 1, 1, 0},
                std::pair<int, int> version = std::pair<int, int>(1, 1),
                int resetInterval = 0); // This should always be 1, 1, if I'm not mistaken
    };
    
    /*
    Data of a compressing JPEG
    
    Screw it, only 8 bits allowed
    */
    class Jpeg {
        public:
            const JpegSettings& settings;
            std::int16_t (*blocks)[JPEG_BLOCK_SIZE];
            void encodeDeltas();
        public:
            Jpeg(const JpegSettings& settings);
            ~Jpeg();
            void encodeRGB(std::uint8_t *rgb);            
    };
    
    /*
    Convert R, G, B to Y/luminance in the same range
    
    T: type of samples
    r: red value of a sample
    g: green value of a sample
    b: blue value of a sample
    
    returns Y
    */
    template<class T>
    inline T Y(T r, T g, T b)
    {
        return (T)(std::max(0.0, std::min(255.0, .299 * r + .587 * g + .114 * b)));
    }
    
    /*
    Convert R, G, B to Cb chrominance in the same range
    
    T: type of samples
    r: red value of a sample
    g: green value of a sample
    b: blue value of a sample
    
    returns Cb
    */
    template<class T>
    inline T Cb(T r, T g, T b)
    {
        return (T)(std::max(0.0, std::min(255.0, 128 - .168736 * r - .331264 * g + .5 * b)));
    }
    
    /*
    Convert R, G, B to Cr chrominance in the same range
    
    T: type of samples
    r: red value of a sample
    g: green value of a sample
    b: blue value of a sample
    
    returns Cr
    */
    template<class T>
    inline T Cr(T r, T g, T b)
    {
        return (T)(std::max(0.0, std::min(255.0, 128 + .5 * r - .418688 * g - .081312 * b)));
    }
    

    inline size_t getPixelCoord(size_t x, size_t y, size_t width, size_t height, size_t pitch)
    {
        x = std::min(x, width - 1);
        y = std::min(y, height - 1);
        return y * pitch + x;
    }

    template <class sample_t>
    inline sample_t componentFromRGB(sample_t *sample, size_t component)
    {
        switch (component) {
            case 0:
                return Y(sample[0], sample[1], sample[2]);
            case 1:
                return Cb(sample[0], sample[1], sample[2]);
            default:
                return Cr(sample[0], sample[1], sample[2]);
        }
    }

}

#endif