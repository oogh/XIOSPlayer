//
// Created by Oogh on 2020/11/4.
//

#ifndef XRENDER_XSAMPLE_HPP
#define XRENDER_XSAMPLE_HPP

#include <memory>
#include "XFFHeader.hpp"

struct XSample {
    uint8_t* data = nullptr;

    int length = 0;

    XSample(int length) {
        this->length = length;
        this->data = reinterpret_cast<uint8_t*>(av_malloc(length));
        memset(this->data, '\0', length);
    }

    ~XSample() {
        this->length = 0;
        if (this->data != nullptr) {
            av_freep(&this->data);
        }
    }
};


#endif //XRENDER_XSAMPLE_HPP
