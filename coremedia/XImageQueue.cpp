//
//  XImageQueue.cpp
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#include "XImageQueue.hpp"

XImageQueue::XImageQueue(int capacity)
: mCapacity(capacity), mSize(0), mWindex(0), mRindex(0), mAborted(false) {
    mImageQueue.reserve(capacity);
    for (int i = 0; i < capacity; ++i) {
        auto image = std::make_shared<XImage>();
        mImageQueue.emplace_back(image);
    }
}

XImageQueue::~XImageQueue() {
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<std::shared_ptr<XImage>>().swap(mImageQueue);
}

std::shared_ptr<XImage> XImageQueue::peekWritable() {
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mCond.wait(lock, [=] {
            return !(mSize >= mCapacity || mAborted);
        });
    }

    if (mAborted) {
        return nullptr;
    }

    return mImageQueue.at(mWindex % mCapacity);
}

void XImageQueue::push() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (++mWindex == mCapacity) {
        mWindex = 0;
    }
    mSize++;
    mCond.notify_one();
}

std::shared_ptr<XImage> XImageQueue::peekReadable() {
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mCond.wait(lock, [=] {
            return !(mSize <= 0 || mAborted);
        });
    }

    if (mAborted) {
        return nullptr;
    }

    return mImageQueue.at(mRindex % mCapacity);
}

void XImageQueue::next() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (++mRindex == mCapacity) {
        mRindex = 0;
    }
    mSize--;
    mCond.notify_one();
}

void XImageQueue::flush() {
    std::lock_guard<std::mutex> lock(mMutex);
    int imageSize = static_cast<int>(mImageQueue.size());
    for (int i = 0; i < imageSize; ++i) {
        auto image = mImageQueue.at(i);
        image->free();
    }

    mSize = 0;
    mWindex = 0;
    mRindex = 0;
    mCond.notify_one();
}

void XImageQueue::abort() {
    std::lock_guard<std::mutex> lock(mMutex);
    mAborted = true;
    mCond.notify_one();
}
