//
// Created by Oogh on 2020/11/4.
//

#include "XTrack.hpp"
#include "XPlatform.hpp"
#include "XTimeline.hpp"
#include "XAudioCodec.hpp"
#include "XFFmpegVideoCodec.hpp"
#include "XIOSVideoCodec.hpp"

int XTrack::ID_GENERATOR = 0;

XTrack::XTrack() : mId(ID_GENERATOR++), mDelay(0), mClipStartTime(0), mClipEndTime(0) {
    mAudioCodec = std::make_unique<XAudioCodec>();
//    mVideoCodec = std::make_unique<XFFmpegVideoCodec>();
    mVideoCodec = std::make_unique<XIOSVideoCodec>();
}

XTrack::~XTrack() {

}

void XTrack::setTimeline(const std::shared_ptr<XTimeline>& timeline) {
    mTimeline = timeline;
}

void XTrack::setFilename(const std::string& filename) {
    mFilename = filename;

    if (mVideoCodec) {
        mVideoCodec->close();
    }
    mVideoCodec->setFilename(filename);
    mVideoCodec->open();

    if (mAudioCodec) {
        mAudioCodec->close();
    }
    mAudioCodec->setFilename(filename);
    mAudioCodec->open();
}

int XTrack::getId() const {
    return mId;
}

void XTrack::setDelay(long delay) {
    mDelay = delay;
}

void XTrack::setClipStartTime(long startTime) {
    mClipStartTime = startTime;
}

void XTrack::setClipEndTime(long endTime) {
    mClipEndTime = endTime;
}

std::string XTrack::getFilename() const {
    return mFilename;
}

long XTrack::getDelay() const {
    return mDelay;
}

long XTrack::getClipStartTime() const {
    return mClipStartTime;
}

long XTrack::getClipEndTime() const {
    return mClipEndTime;
}

long XTrack::getClipDuration() const {
    return mClipEndTime - mClipStartTime;
}

std::shared_ptr<XImage> XTrack::getImage(long clock) {
    if (mVideoCodec && (mDelay <= clock && clock <= mDelay + getClipDuration())) {
        auto result =  mVideoCodec->getImage(clock - mDelay);
        if (result) {
            result->textureId = mId;
        }
        return result;
    }
    return nullptr;
}

std::shared_ptr<XSample> XTrack::getSample(long clock, int length) {
    if (mAudioCodec && (mDelay <= clock && clock <= mDelay + getClipDuration())) {
        return mAudioCodec->getSample(length);
    }
    return nullptr;
}
