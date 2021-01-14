//
//  XFrameQueue.h
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XEXPORTER_XFRAMEQUEUE_H
#define XEXPORTER_XFRAMEQUEUE_H

#include <list>
#include <mutex>
#include <condition_variable>
#include "XFFHeader.hpp"

class XFrameQueue {
public:
    explicit XFrameQueue(int capacity = FQ_DEFAULT_PARAM_CAPACITY);

    ~XFrameQueue();

    void put(std::shared_ptr<Frame> frame);

    std::shared_ptr<Frame> get(bool wait = true);

    void flush();

    void signal();

private:
    static const int FQ_DEFAULT_PARAM_CAPACITY = 5;

private:
    std::list <std::shared_ptr<Frame>> mFrameQueue;
    std::mutex mMutex;
    std::condition_variable mCond;
    int mCapacity;
    bool mAborted;
};

#endif //XEXPORTER_XFRAMEQUEUE_H
