//
//  XPlayer.cpp
//  XRender
//
//  Created by Oogh on 2020/11/4.
//  Copyright © 2020 Oogh. All rights reserved.
//

#include "XPlayer.hpp"
#include "XSounder.hpp"
#include "XTimeline.hpp"
#include "XThreadUtils.hpp"
#include "XLogger.hpp"

XPlayer::XPlayer() {
    mSounder = std::make_unique<XSounder>();
}

XPlayer::~XPlayer() {
    if (mAudioTid) {
        mAudioTid->join();
        mAudioTid.reset();
    }
    
    if (mVideoTid) {
        mVideoTid->join();
        mAudioTid.reset();
    }
}

void XPlayer::setTimeline(std::shared_ptr<XTimeline>&& timeline) {
    mTimeline = timeline;
}

void XPlayer::attachObserverRender(std::shared_ptr<XRenderObserver> observer) {
    mRenderObserver = observer;
}

int XPlayer::start() {
    if (!mAudioTid) {
        mAudioTid = std::make_unique<std::thread>([this]{ audioWorkThread(this); });
    }

    if (!mVideoTid) {
        mVideoTid = std::make_unique<std::thread>([this]{ videoWorkThread(this); });
    }

    return 0;
}

void XPlayer::audioWorkThread(void* opaque) {
    XThreadUtils::configThreadName("audioWorkThread");
    LOGD("[XPlayer] audioWorkThread ++++\n");
    auto player = reinterpret_cast<XPlayer*>(opaque);

    player->mSounder->start();

    for (;;) {
        if (player->mAborted) {
            break;
        }

        if (player->mTimeline->isCompleted()) {
            std::unique_lock<std::mutex> lock(player->mMutex);
            player->mContinueAudioWorkCond.wait(lock);
        }

        auto sample = player->mTimeline->getSample(4096);
        player->mSounder->updateAudio(sample->data, sample->length);
//        LOGE("[XPlayer] andy sample->length: %d\n", sample->length);
    }

    player->mSounder->stop();
    LOGD("[XPlayer] audioWorkThread ----\n");
}

void XPlayer::videoWorkThread(void* opaque) {
    XThreadUtils::configThreadName("videoWorkThread");
    LOGD("[XPlayer] videoWorkThread ++++\n");
    auto player = reinterpret_cast<XPlayer*>(opaque);
    long lastClock = -1;
    for (;;) {
        if (player->mAborted) {
            break;
        }

        if (player->mTimeline->isCompleted()) {
            std::unique_lock<std::mutex> lock(player->mMutex);
            player->mContinueVideoWorkCond.wait(lock);
        }
        
        long clock = player->mTimeline->getClock();
        if (clock != lastClock) {
            auto images = player->mTimeline->getImage(clock);
            if (player->mRenderObserver) {
                player->mRenderObserver->update(images);
            }
        }
        lastClock = clock;
    }

    LOGD("[XPlayer] videoWorkThread ----\n");
}

int XPlayer::stop() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mAborted = true;
    }
    
    if (mAudioTid) {
        mAudioTid->join();
        mAudioTid.reset();
    }
    return 0;
}

