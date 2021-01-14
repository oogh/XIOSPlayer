//
//  XFileProducer.cpp
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#include "XFileProducer.hpp"
#include "XException.hpp"

XFileProducer::XFileProducer() {
    mWidth = 720;
    mHeight = 1280;
    mRGBABufferSize = mWidth * mHeight * 4;
    mRGBABuffer = new uint8_t[mRGBABufferSize];
    memset(mRGBABuffer, '\0', mRGBABufferSize);

}

XFileProducer::~XFileProducer() {
    mFileStream.close();
    if (mRGBABuffer) {
        delete [] mRGBABuffer;
        mRGBABuffer = nullptr;
    }
    mRGBABufferSize = 0;
}

void XFileProducer::setInput(const std::string& filename) {
    XProducable::setInput(filename);
    mFileStream.open(mFilename.data());
    if (mFileStream.fail()) {
        throw XException("[XFileProducer] open file failed!");
    }
}

std::shared_ptr<XImage> XFileProducer::peekImage(long clock) {
    mFileStream.read(reinterpret_cast<char *>(mRGBABuffer), mRGBABufferSize);
    auto image = std::make_shared<XImage>();
//    image->copyPixels(mRGBABuffer, mWidth, mHeight);
    return image;
}
