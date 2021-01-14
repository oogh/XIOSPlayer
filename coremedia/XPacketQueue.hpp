//
//  XPacketQueue.hpp
//  XMedia
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XPacketQueue_hpp
#define XPacketQueue_hpp

#include <queue>
#include <mutex>
#include <condition_variable>
#include "XFFHeader.hpp"

class XPacketQueue {
public:
    /**
     * @brief 构建一个新的队列
     */
    explicit XPacketQueue(int capacity = PQ_DEFAULT_CAPACITY);
    
    /**
     * @brief 销毁一个队列
     */
    ~XPacketQueue();
    
    /**
     * @brief 向队列中放入一个AVPacket数据
     * @param pkt AVPacket数据
     * @return 0表示成功，其他表示失败
     */
    int put(const std::shared_ptr<Packet> pkt);
    
    /**
     * @brief 向队列中放入一个Pcket{data: nullptr, size: 0}数据
     * @param streamIndex 流索引
     * @return 0表示成功，其他表示失败
     */
    int putNullPacket(int streamIndex);
    
    /**
     * @brief 从队列中获取一个Packet数据
     * @return nullptr表示失败，其他表示成功
     */
    std::shared_ptr<Packet> get();
    
    /**
     * @brief 获取队列中可用的Packet的数量
     */
    int getAvailableCount() const;
    
    /**
     * @brief 刷新队列
     */
    void flush();
    
private:
    static const size_t PQ_DEFAULT_CAPACITY = 2500;
    
private:
    std::queue<std::shared_ptr<Packet>> mPacketQueue;
    
    mutable std::mutex mMutex;
    
    std::condition_variable mCond;
    
    int mSize;
    
    int mCapacity;
};

#endif /* XPacketQueue_hpp */
