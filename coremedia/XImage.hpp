//
//  XImage.h
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XEXPORTER_XIMAGE_H
#define XEXPORTER_XIMAGE_H

#include <memory>

enum ImageType {
    IMG_TYPE_UNKNOWN = -1,
    IMG_TYPE_YUV420P = 0, // AV_PIX_FMT_YUV420P
    IMG_TYPE_RGB24 = 2, // AV_PIX_FMT_RGB24
    IMG_TYPE_RGBA = 26, // AV_PIX_FMT_RGBA
};

struct XImage {
    XImage();

    ~XImage();
    
    XImage(const XImage& that);
    
    XImage& operator=(const XImage& that);
    
    XImage(XImage&& that);
    
    XImage& operator=(XImage&& that);

    void free();
    
    uint8_t* pixels[4] = {nullptr};

    int linesize[4] = {0};

    int width = 0;

    int height = 0;

    long pts = -1;

    long duration = -1;

    int format = -1;

    int textureId = -1;
};

#endif //XEXPORTER_XIMAGE_H
