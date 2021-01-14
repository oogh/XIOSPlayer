//
//  XPacketQueue.cpp
//  XMedia
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#include "XPacketQueue.hpp"

XPacketQueue::XPacketQueue(int capacity)
: mSize(0), mCapacity(capacity) {
}

XPacketQueue::~XPacketQueue() {
    std::lock_guard<std::mutex> lock(mMutex);
    std::queue<std::shared_ptr<Packet>>().swap(mPacketQueue);
}

int XPacketQueue::put(const std::shared_ptr<Packet> packet) {
    
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mCond.wait(lock, [=] {
            return mPacketQueue.size() < mCapacity;
        });
    }
    
    std::shared_ptr<Packet> pkt;
    if (packet->avpkt->data && packet->avpkt->size) {
        pkt = std::make_shared<Packet>();
        if (av_packet_ref(pkt->avpkt, packet->avpkt) < 0) {
            return -1;
        }
    } else {
        /* AVPacket{data: null, size: 0} 在执行 av_packet_ref 之后会变成 AVPacket{data: "", size: 0}
           这会造成 avcodec_send_packet 的时候出现 Invaild Argument 错误 */
        pkt = packet;
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mPacketQueue.emplace(pkt);
    mSize += pkt->avpkt->size;
    mCond.notify_one();
    return 0;
}

int XPacketQueue::putNullPacket(int streamIndex) {
    std::shared_ptr<Packet> pkt = std::make_shared<Packet>();
    pkt->avpkt->stream_index = streamIndex;
    return put(pkt);
}

std::shared_ptr<Packet> XPacketQueue::get() {
    std::unique_lock<std::mutex> lock(mMutex);
    mCond.wait(lock, [=] {
        return !mPacketQueue.empty();
    });
    
    auto pkt = std::move(mPacketQueue.front());
    mPacketQueue.pop();
    mCond.notify_one();
    return pkt;
}

int XPacketQueue::getAvailableCount() const {
    return static_cast<int>(mPacketQueue.size());
}

void XPacketQueue::flush() {
    std::lock_guard<std::mutex> lock(mMutex);
    std::queue<std::shared_ptr<Packet>>().swap(mPacketQueue);
    mSize = 0;
    mCond.notify_one();
}
