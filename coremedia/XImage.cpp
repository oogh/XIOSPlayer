//
//  XImage.cpp
//  XRender
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#include "XImage.hpp"
#include "XFFHeader.hpp"

XImage::XImage(): pixels{nullptr}, linesize{0}, width(0), height(0), pts(AV_NOPTS_VALUE), duration(AV_NOPTS_VALUE), format(AV_PIX_FMT_NONE), textureId(-1) {
}

XImage::~XImage() {
    free();
}

XImage::XImage(const XImage& that) {
    for (int i = 0; i < 4; ++i) {
        if (this->pixels[i]) {
            av_freep(&this->pixels[i]);
            this->pixels[i] = nullptr;
        }
        
        if (that.pixels[i]) {
            this->pixels[i] = reinterpret_cast<uint8_t*>(av_malloc(that.linesize[i] * that.height));
            memcpy(this->pixels[i], that.pixels[i], that.linesize[i] * that.height);
        }
        
        this->linesize[i] = that.linesize[i];
    }
    
    this->width = that.width;
    this->height = that.height;
    this->pts = that.pts;
    this->duration = that.duration;
    this->format = that.format;
    this->textureId = that.textureId;
}

XImage& XImage::operator=(const XImage& that) {
    if (this != &that) {
        for (int i = 0; i < 4; ++i) {
            if (this->pixels[i]) {
                av_freep(&this->pixels[i]);
                this->pixels[i] = nullptr;
            }
            
            if (that.pixels[i]) {
                this->pixels[i] = reinterpret_cast<uint8_t*>(av_malloc(that.linesize[i] * that.height));
                memcpy(this->pixels[i], that.pixels[i], that.linesize[i] * that.height);
            }
            
            this->linesize[i] = that.linesize[i];
        }
        
        this->width = that.width;
        this->height = that.height;
        this->pts = that.pts;
        this->duration = that.duration;
        this->format = that.format;
        this->textureId = that.textureId;
    }
    return *this;
}

XImage::XImage(XImage&& that) {
    for (int i = 0; i < 4; ++i) {
        this->pixels[i] = that.pixels[i];
        that.pixels[i] = nullptr;
        this->linesize[i] = that.linesize[i];
        that.linesize[i] = 0;
    }
    
    this->width = that.width;
    this->height = that.height;
    this->pts = that.pts;
    this->duration = that.duration;
    this->format = that.format;
    this->textureId = that.textureId;
}

XImage& XImage::operator=(XImage&& that) {
    if (this != &that) {
        for (int i = 0; i < 4; ++i) {
            this->pixels[i] = that.pixels[i];
            that.pixels[i] = nullptr;
            this->linesize[i] = that.linesize[i];
            that.linesize[i] = 0;
        }
        
        this->width = that.width;
        this->height = that.height;
        this->pts = that.pts;
        this->duration = that.duration;
        this->format = that.format;
        this->textureId = that.textureId;
    }
    return *this;
}

void XImage::free() {
    this->width = 0;
    this->height = 0;
    this->pts = -1;
    this->duration = -1;
    if (this->pixels[0] != nullptr) {
        av_freep(&this->pixels[0]);
        av_freep(&this->pixels);
    }
    
    for (int i = 0; i < 4; ++i) {
        this->linesize[i] = 0;
    }
}
