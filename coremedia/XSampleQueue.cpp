//
//  XSampleQueue.cpp
//  XRender
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#include "XSampleQueue.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/types.h>

XSampleQueue::XSampleQueue(int capacity)
: mCapacity(capacity), mSize(0), mWindex(0), mRindex(0), mSignaled(false) {
    mBuffer = reinterpret_cast<uint8_t*>(malloc(capacity));
    memset(mBuffer, '\0', capacity);
}

XSampleQueue::~XSampleQueue() {
    if (mBuffer) {
        free(mBuffer);
        mBuffer = nullptr;
    }
}

int XSampleQueue::write(uint8_t* in, int size) {
    if (!in || size <= 0) {
        return -1;
    }

    std::unique_lock<std::mutex> lock(mMutex);
    mCond.wait(lock, [&] {
        return mSize < mCapacity && !mSignaled;
    });
    
    if (mSize >= mCapacity) {
        return 0;
    }

    int available = mCapacity - mSize;
    int writeSize = std::min(size, available);
    if (writeSize > 0) {
        // 写数据，直到写到末尾位置
        int toEndSize = mCapacity - mWindex;
        int writeToEndSize = std::min(toEndSize, writeSize);
        if (writeToEndSize > 0) {
            std::memcpy(mBuffer + mWindex, in, writeToEndSize);
            mSize += writeToEndSize;
            mWindex += writeToEndSize;
            if (mWindex >= mCapacity) {
                mWindex = 0;
            }
        }
        
        // 到末尾位置，数据仍然没有写完，则从头开始写
        int writeFromHeadSize = writeSize - writeToEndSize;
        if (writeFromHeadSize > 0) {
            in += writeToEndSize;
            std::memcpy(mBuffer + mWindex, in, writeFromHeadSize);
            mSize += writeFromHeadSize;
            mWindex += writeFromHeadSize;
            if (mWindex > mCapacity) {
                mWindex = 0;
            }
        }
    }
    
    mCond.notify_one();
    
    return writeSize;
}

int XSampleQueue::read(uint8_t* out, int size) {
    if (!out || size <= 0) {
        return -1;
    }

    std::unique_lock<std::mutex> lock(mMutex);
    mCond.wait_for(lock, std::chrono::duration<double, std::ratio<1, 1000>>(30), [&] {
        return mSize > 0 && !mSignaled;
    });
    
    if (mSize <= 0) {
        return 0;
    }
    
    int readSize = std::min(size, mSize);
    if (readSize > 0) {
        int toEndSize = mCapacity - mRindex;
        if (toEndSize >= readSize) {
            std::memcpy(out, mBuffer + mRindex, readSize);
            mSize -= readSize;
            mRindex += readSize;
            if (mRindex >= mCapacity) {
                mRindex = 0;
            }
        } else {
            std::memcpy(out, mBuffer + mRindex, toEndSize);
            mSize -= toEndSize;
            mRindex += toEndSize;
            if (mRindex > mCapacity) {
                mRindex = 0;
            }
            
            int readFromHeadSize = readSize - toEndSize;
            if (readFromHeadSize > 0) {
                out += toEndSize;
                std::memcpy(out, mBuffer + mRindex, readFromHeadSize);
                mSize -= readFromHeadSize;
                mRindex += readFromHeadSize;
                if (mRindex >= mCapacity) {
                    mRindex = 0;
                }
            }
        }
    }
    
    mCond.notify_one();

    return readSize;
}

int XSampleQueue::available() const {
    return mCapacity - mSize;
}

int XSampleQueue::used() const {
    return mSize;
}

void XSampleQueue::flush() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mBuffer) {
        free(mBuffer);
        mBuffer = nullptr;
    }
    mSize = 0;
    mWindex = 0;
    mRindex = 0;
    mCond.notify_one();
}

void XSampleQueue::signal() {
    std::lock_guard<std::mutex> lock(mMutex);
    mCond.notify_all();
}
