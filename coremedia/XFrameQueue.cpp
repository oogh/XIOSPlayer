//
//  XFrameQueue.cpp
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#include "XFrameQueue.hpp"

XFrameQueue::XFrameQueue(int capacity): mCapacity(capacity), mAborted(false) {
}

XFrameQueue::~XFrameQueue() {

}

void XFrameQueue::put(std::shared_ptr<Frame> frame) {
    if (mCapacity > 0) {
        std::unique_lock <std::mutex> lock(mMutex);
        mCond.wait(lock, [=] {
            return mFrameQueue.size() < mCapacity;
        });
    }

    std::lock_guard <std::mutex> lock(mMutex);
    mFrameQueue.emplace_back(frame);
    mCond.notify_one();
}

std::shared_ptr<Frame> XFrameQueue::get(bool wait) {
    if (wait) {
        std::unique_lock <std::mutex> lock(mMutex);
        mCond.wait(lock, [=] {
            return !(!mAborted && mFrameQueue.empty());
        });
    }
    
    if (mAborted) {
        return nullptr;
    }
    
    std::lock_guard <std::mutex> lock(mMutex);
    auto frame = mFrameQueue.front();
    mFrameQueue.pop_front();
    mCond.notify_one();
    return frame;
}

void XFrameQueue::flush() {
    std::lock_guard <std::mutex> lock(mMutex);
    std::list < std::shared_ptr < Frame >> ().swap(mFrameQueue);
    mCond.notify_one();
}

void XFrameQueue::signal() {
    std::lock_guard <std::mutex>lock(mMutex);
    mAborted = true;
    mCond.notify_one();
}
