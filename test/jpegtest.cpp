/*
jpegtest.cpp
Jpeg testing
*/

#include <iostream>
#include <fstream>
#include <iomanip>
#include <utility>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <sstream>
#include <string>
#include <cstdlib>
#include <getopt.h>
#include "jpegutil.hpp"

#include "bitutil.hpp"

#define W 64
#define H 64

int main(int argc, char **argv) {
    size_t w = W, h = H;
    int quality = 50;
    bool optimize = false;
    int c;
    while ((c = getopt(argc, argv, "w:h:oq:")) != -1) {
        switch (c) {
            case 'w':
                w = atoi(optarg);
                break;
            case 'h':
                h = atoi(optarg);
                break;
            case 'o':
                optimize = true;
                break;
            case 'q':
                quality = atoi(optarg);
                break;
        }
    }
    Jpeg::JpegSettings settings(
        std::pair<int, int>(w, h),
        nullptr,
        Jpeg::DPI,
        {1, 1},
        quality
    );
    if (optimize) {
        settings.compressionFlags = Jpeg::flagHuffmanOptimal;
    }
    Jpeg::Jpeg img(settings);
    std::uint8_t *rgb = new std::uint8_t[w * h * 3]{0};
    
    float x0 = (float)rand() / RAND_MAX * w;
    float x1 = (float)rand() / RAND_MAX * w;
    float y0 = (float)rand() / RAND_MAX * h;
    float y1 = (float)rand() / RAND_MAX * h;
    for (int x = 0; x < w; x++) {
        for (int y = 0; y < h; y++) {
            float dist0 = 1 / std::hypot((x - x0), (y - y0));
            float dist1 = 1 / std::hypot((x - x1), (y - y1));
            if (dist0 + dist1 >= 1.0 / 50) {
                rgb[(y * w + x) * 3] = 127;
            }
        }
    }
    
    img.encodeRGB((rgb));
    std::ofstream ss("test.jpeg", std::ios_base::out | std::ios_base::binary);
    img.write(ss);
    ss.close();
    delete[] rgb;
    return 0;
}