//
//  XImageUtils.h
//  XRender
//
//  Created by Oogh on 2020/07/16.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XRENDER_XIMAGEUTILS_H
#define XRENDER_XIMAGEUTILS_H

#include <cstdio>
#include <cstdlib>
#include <cstring>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int LONG;

#define BITS_PER_PIXEL 24
#define FORMAT_RGBA 4
#define FORMAT_RGB  3

//must needed pack
#pragma pack(1)

struct BMPFileHeader {
    WORD    bfType;
    DWORD   bfSize;
    WORD    bfReserved1;
    WORD    bfReserved2;
    DWORD   bfOffBits;
};

struct BMPInfoHeader{
    DWORD      biSize;
    LONG       biWidth;
    LONG       biHeight;
    WORD       biPlanes;
    WORD       biBitCount;
    DWORD      biCompression;
    DWORD      biSizeImage;
    LONG       biXPelsPerMeter;
    LONG       biYPelsPerMeter;
    DWORD      biClrUsed;
    DWORD      biClrImportant;
};

#pragma pack()

int rgb2bmp(const char *outFile, uint8_t* rgbData, int width, int height, int format = FORMAT_RGBA) {
    BMPFileHeader bmpHeader;
    BMPInfoHeader bmpInfo;
    
    FILE* fp = nullptr;
    uint8_t* bmpData = nullptr;
    uint8_t* ptr = nullptr;
    
    // 4 bytes pack. must be 4 times per line
    int stride = (width * BITS_PER_PIXEL + 31) / 32 * 4;
    int dataSize  = stride * height;
    
    bmpHeader.bfType = 0x4D42;
    bmpHeader.bfReserved1 = 0;
    bmpHeader.bfReserved2 = 0;
    bmpHeader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
    bmpHeader.bfSize = bmpHeader.bfOffBits     + dataSize;
    
    bmpInfo.biSize = sizeof(BMPInfoHeader);
    bmpInfo.biWidth = width;
    bmpInfo.biHeight = -height; // 这样图片才不会倒置
    bmpInfo.biPlanes = 1;
    bmpInfo.biBitCount = BITS_PER_PIXEL;
    bmpInfo.biCompression = 0;
    bmpInfo.biSizeImage = dataSize;
    bmpInfo.biXPelsPerMeter = 100;
    bmpInfo.biYPelsPerMeter = 100;
    bmpInfo.biClrUsed = 0;
    bmpInfo.biClrImportant = 0;
    
    /** convert in memort, then write to file. */
    bmpData = (uint8_t*)malloc(dataSize);
    if (!bmpData) {
        return -1;
    }
    
    fp = fopen(outFile,"wb+");
    if (!fp) {
        return -1;
    }
    
    fwrite(&bmpHeader, sizeof(BMPFileHeader), 1, fp);
    fwrite(&bmpInfo,   sizeof(BMPInfoHeader), 1, fp);
    /** Here you should consider color format. RGBA ? RGB? BGR?
     Param format is RGBA, format for file is BGR */
    ptr = bmpData;
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            ptr[0] = rgbData[2];
            ptr[1] = rgbData[1];
            ptr[2] = rgbData[0];
            rgbData += format;
            ptr += FORMAT_RGB;
        }
        //pack for 4 bytes
        ptr +=(stride - width * FORMAT_RGB);
    }
    fwrite(bmpData, dataSize, 1, fp);
    
    fclose(fp);
    free(bmpData);
    
    return 0;
}

#endif //XRENDER_XIMAGEUTILS_H
