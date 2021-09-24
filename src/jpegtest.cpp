/*
jpegtest.cpp
Jpeg testing
*/

#include <iostream>
#include <fstream>
#include <iomanip>
#include <utility>
#include <cstdint>
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
    bool optimize = false;
    int c;
    while ((c = getopt(argc, argv, "w:h:o")) != -1) {
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
        }
    }
    Jpeg::JpegSettings settings(
        std::pair<int, int>(w, h)
    );
    if (optimize) {
        settings.compressionFlags = Jpeg::flagHuffmanOptimal;
    }
    Jpeg::Jpeg img(settings);
    std::uint8_t *rgb = new std::uint8_t[w * h * 3]{0};
    for (size_t i = 0; i < w * h * 3; i += 3) {
        int pix = i / 3;
        int y = pix / w;
        int x = pix % w;
        rgb[i] = (x ^ y) & 0xff;
        rgb[i + 1] = (x) & 0xff;
        rgb[i + 2] = (y) & 0xff;
    }
    img.encodeRGB((rgb));
    std::ofstream ss("test.jpeg", std::ios_base::out | std::ios_base::binary);
    img.write(ss);
    ss.close();
    delete[] rgb;
    return 0;
}