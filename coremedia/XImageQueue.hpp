//
//  XImageQueue.h
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XEXPORTER_XIMAGEQUEUE_H
#define XEXPORTER_XIMAGEQUEUE_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include "XImage.hpp"

class XImageQueue {
public:
    explicit XImageQueue(int capacity = IQ_DEFAULT_CAPACITY);

    ~XImageQueue();

    std::shared_ptr<XImage> peekWritable();

    void push();

    std::shared_ptr<XImage> peekReadable();

    void next();

    void flush();

    void abort();

private:
    static const int IQ_DEFAULT_CAPACITY = 3;

    std::vector<std::shared_ptr<XImage>> mImageQueue;
    std::mutex mMutex;
    std::condition_variable mCond;

    int mCapacity;

    int mSize;

    int mWindex;

    int mRindex;

    bool mAborted;

};


#endif //XEXPORTER_XIMAGEQUEUE_H
