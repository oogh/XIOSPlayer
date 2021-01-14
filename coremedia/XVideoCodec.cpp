//
// Created by Oogh on 2020/9/18.
//

#include "XVideoCodec.hpp"
#include "XLogger.hpp"
#include "XPlatform.hpp"
#include "XThreadUtils.hpp"
#include "XImage.hpp"
#include "XImageQueue.hpp"
#include "libyuv.h"


XVideoCodec::XVideoCodec(): mLoop(false), mStatus(0), mLastRequestClock(LONG_MIN) {

}

XVideoCodec::~XVideoCodec() {

}

void XVideoCodec::setFilename(const std::string& filename) {
    mFilename = filename;
}

void XVideoCodec::setLoop(bool loop) {
    mLoop = loop;
}


