//
//  XPlayer.hpp
//  XRender
//
//  Created by Oogh on 2020/11/4.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XRENDER_XPLAYER_HPP
#define XRENDER_XPLAYER_HPP

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "XRenderObserver.hpp"

class XRender;
class XSounder;

class XTimeline;

class XPlayer {
public:
    XPlayer();

    ~XPlayer();

    void setTimeline(std::shared_ptr<XTimeline>&& timeline);

    void attachObserverRender(std::shared_ptr<XRenderObserver> observer);

    int start();
    
    int stop();

private:
    void audioWorkThread(void *opaque);

    void videoWorkThread(void *opaque);

private:
    std::unique_ptr<XSounder> mSounder;
    std::shared_ptr<XTimeline> mTimeline;

    std::unique_ptr<std::thread> mAudioTid;
    std::unique_ptr<std::thread> mVideoTid;
    std::mutex mMutex;
    std::condition_variable mContinueAudioWorkCond;
    std::condition_variable mContinueVideoWorkCond;

    bool mAborted;
    std::shared_ptr<XRenderObserver> mRenderObserver;
};

#endif //XRENDER_XPLAYER_HPP
