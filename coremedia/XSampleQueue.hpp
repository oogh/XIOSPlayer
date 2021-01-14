//
//  XSampleQueue.h
//  XRender
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef MIXER_XSAMPLEQUEUE_H
#define MIXER_XSAMPLEQUEUE_H

#include <cinttypes>
#include <mutex>
#include <condition_variable>

class XSampleQueue {
public:
    XSampleQueue(int capacity = SAMPLE_QUEUE_CAPACITY);

    ~XSampleQueue();

    int write(uint8_t* in, int size);

    int read(uint8_t* out, int size);

    int available() const;

    int used() const;

    void flush();
    
    void signal();

private:
    static const int SAMPLE_QUEUE_CAPACITY = 4096;

private:
    int mCapacity;

    uint8_t* mBuffer;
    int mSize;
    
    int mWindex;
    int mRindex;
    
    std::mutex mMutex;
    std::condition_variable mCond;
    
    bool mSignaled;
};

#endif //MIXER_XSAMPLEQUEUE_H
