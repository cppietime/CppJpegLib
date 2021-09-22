/*
jpegtest.cpp
Jpeg testing
*/

#include <iostream>
#include <utility>
#include <cstdint>
#include "jpegutil.hpp"

#define W 16
#define H 16

int main() {
    
    Jpeg::JpegSettings settings(
        std::pair<int, int>(W, H)
    );
    Jpeg::Jpeg img(settings);
    std::uint8_t rgb[W * H * 3] = {0};
    for (size_t i = 0; i < W * H * 3; i++) {
        rgb[i] = 0;
    }
    img.encodeRGB((rgb));
    size_t numMcus = img.settings.numMcus.first * img.settings.numMcus.second;
    for (int i = 0; i < numMcus; i++) {
        for (int j = 0; j < img.settings.mcuSize; j++) {
            std::cout << i << "," << j << ": " << (int)img.blocks[img.settings.mcuSize * i + j][0] << std::endl;
        }
    }
    img.encodeDeltas();
    for (int i = 0; i < numMcus; i++) {
        for (int j = 0; j < img.settings.mcuSize; j++) {
            std::cout << i << "," << j << ": " << (int)img.blocks[img.settings.mcuSize * i + j][0] << std::endl;
        }
    }
    return 0;
}