/*
jpegutil.hpp
Yaakov Schectman, 2021
A utility specifically for JPEG encoding
Only 8-bit precision, non-progressive currently supported,
but frankly, that's all you'd normally need
*/

#ifndef _JPEGUTIL_HPP
#define _JPEGUTIL_HPP

#include <utility>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <algorithm>
#include <vector>

#include "bitutil.hpp"

#define JPEG_MAX_COMPONENTS 5

#define JPEG_BLOCK_SIZE 64
#define JPEG_BLOCK_ROW 8
#define JPEG_DCT_COEFF_SIZE 64
#define JPEG_DCT_SIZE 8

namespace Jpeg {
    
    using codes_t = std::pair<std::vector<Huffman::HuffmanCode>, std::vector<Huffman::HuffmanCode>>;
    using dqt_t = std::uint16_t;
    using dct_t = std::int32_t;
    
    /* Constant tables used for operations */
    extern const dqt_t defaultLuminanceQTable[JPEG_BLOCK_SIZE];
    extern const dqt_t defaultChrominanceQTable[JPEG_BLOCK_SIZE];
    extern const float dctCoeffs[JPEG_DCT_COEFF_SIZE];
    extern const size_t zigzag[JPEG_BLOCK_SIZE];
    
    /* Flags */
    const int flagHuffmanDefault = 0;
    const int flagHuffmanProvided = 1;
    const int flagHuffmanOptimal = 2;
    const int flagHuffmanMask = 3;

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
            JpegComponent(
                std::pair<int, int> sampling,
                size_t qtable,
                size_t dcTable,
                size_t acTable
            ) :
                sampling {sampling},
                qtable {qtable},
                dcTable {dcTable},
                acTable {acTable}
            {}
    };

    /*
    Data object to hold settings for JPEG encoding and metadata
    */
    struct JpegSettings {
        private:
            void init();
        public:
            std::vector<JpegComponent> components;
            std::pair<int, int> size;
            JpegDensityUnits densityUnits;
            std::pair<int, int> density;
            int bitDepth;
            int quality;
            int compressionFlags;
            int numQTables;
            dqt_t qtables[JPEG_MAX_COMPONENTS][JPEG_BLOCK_SIZE];
            std::pair<int, int> version;
            int resetInterval;
            codes_t huffmanCodes;

            std::pair<int, int> mcuScale;
            std::pair<int, int> numMcus;
            size_t componentOffsets[JPEG_MAX_COMPONENTS];
            size_t mcuSize;
            
            /*
            bitDepth and resetInterval are not (yet) supported as non-default values
            */
            JpegSettings(
                std::pair<int, int> size,
                const std::vector<JpegComponent> *components = nullptr,
                JpegDensityUnits densityUnits = DPI,
                std::pair<int, int> density = std::pair<int, int>(1, 1),
                int quality = 50,
                int compressionFlags = flagHuffmanDefault,
                int numQTables = 2,
                const dqt_t *qtables[JPEG_MAX_COMPONENTS] = (const dqt_t* [JPEG_MAX_COMPONENTS]){
                    defaultLuminanceQTable,
                    defaultChrominanceQTable
                },
                std::pair<int, int> version = std::pair<int, int>(1, 1),
                const codes_t *huffmanCodes = nullptr,
                int bitDepth = 8,
                int resetInterval = 0);
    };
    
    /*
    Data of a compressing JPEG
    
    Screw it, only 8 bits allowed
    */
    class Jpeg {
        public:
            JpegSettings settings;
            volatile dct_t (*blocks)[JPEG_BLOCK_SIZE];
            void encodeDeltas();
            void encodeCompressed(BitBuffer::BitBufferOut& dst);
        public:
            Jpeg(JpegSettings jpegSettings) :
                settings {jpegSettings},
                blocks {new dct_t[
                        jpegSettings.numMcus.first *
                        jpegSettings.numMcus.second *
                        jpegSettings.mcuSize
                    ][JPEG_BLOCK_SIZE]}
            {}
            
            Jpeg(const Jpeg& other);
            
            Jpeg& operator=(const Jpeg& other);
            
            ~Jpeg();
            
            /*
            Populate this JPEG with RGB data
            */
            void encodeRGB(const std::uint8_t *rgb);
            
            /*
            Compress and write out to a stream
            */
            void write(std::ostream& dst);
    };
    
    /*
    Exception raised when an error in JPEG encoding is encountered
    */
    class JpegEncodingException : public std::exception {
        private:
            const char* message;
        public:
            JpegEncodingException(std::string message) :
                message{("Jpeg Encoding Exception: " + message).c_str()} {}
            virtual const char* what() {
                return message;
            }
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
    

    inline size_t getPixelCoord(size_t x, size_t y, size_t width, size_t height)
    {
        x = std::min(x, width - 1);
        y = std::min(y, height - 1);
        return y * width + x;
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