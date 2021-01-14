//
//  XFFProducer.h
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XEXPORTER_XFFPRODUCER_H
#define XEXPORTER_XFFPRODUCER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include "XProducable.hpp"
#include "XFFHeader.hpp"
#include "XSampleQueue.hpp"

class XPacketQueue;
class XImageQueue;
class XSampleQueue;

class XFFProducer : public XProducable {
public:
    XFFProducer();

    ~XFFProducer() override;

    void setInput(const std::string& filename) override;

    void start() override;

    std::shared_ptr<XImage> peekImage(long clock) override;
    
    void endCurrentImageUse() override;

    int readSamples(uint8_t* buffer, int length) override;
    
    void stop() override;
    
public:
    long getOriginalDuration() const override;
    
    int getOriginalWidth() const override;
    
    int getOriginalHeight() const override;
    
    int getOriginalRotation() const override;
    
    int getOriginalFrameRate() const override;
    
    void findKeyTimestamps();
    
private:
    void seekTo(long targetPos);
    
    int seekFileTo(long targetPos);

private:
    int openInFile();

    int openVideoCodec();

    int openAudioCodec();

    void closeVideoCodec();

    void closeAudioCodec();

    void closeInFile();

private:
    void readWorkThread(void* opaque);

    void videoWorkThread(void* opaque);

    void audioWorkThread(void* opaque);

private:
    int decodeVideoFrame();
    
    int decodeAudioFrame();

    void queueFrame(AVFrame* frame, long pts, long duration);
    
    void frameConvert(std::shared_ptr<XImage> dst, AVFrame* src);
    
    int sampleConvert(AVFrame* src);
    
private:
    bool isValidPacket(AVPacket* pkt);
    
    bool checkIfNeedSeek(long clock);

private:
    unsigned int mStatus;
    const int S_READ_END = 1 << 0;
    const int S_VIDEO_END = 1 << 1;
    const int S_AUDIO_END = 1 << 2;

    const AVPixelFormat DST_PIX_FMT = AV_PIX_FMT_RGBA;
    
    const uint64_t DST_CHANNEL_LAYOUT = AV_CH_LAYOUT_STEREO;
    const int DST_SAMPLE_RATE = 44100;
    const int DST_CHANNELS = 2;
    const AVSampleFormat DST_SAMPLE_FMT = AV_SAMPLE_FMT_S16;
    
    const int SAMPLE_QUEUE_SIZE = 4096;

private:
    std::unique_ptr<AVFormatContext, InputFormatDeleter> mFormatCtx;

    int mVideoIndex;
    int mAudioIndex;

    std::unique_ptr<AVCodecContext, CodecDeleter> mVideoCodecCtx;
    std::unique_ptr<AVCodecContext, CodecDeleter> mAudioCodecCtx;

    std::unique_ptr<std::thread> mReadTid;
    std::unique_ptr<std::thread> mVideoTid;
    std::unique_ptr<std::thread> mAudioTid;

    std::unique_ptr<XPacketQueue> mVideoPacketQueue;
    std::unique_ptr<XPacketQueue> mAudioPacketQueue;

    bool mSeekReq;
    bool mAVFormatSeeked;
    
    long mSeekTargetPos;
    
    bool mPauseReq;
    
    bool mAbortReq;

    std::mutex mMutex;
    std::condition_variable mContinueReadCond;
    std::condition_variable mContinueVideoCond;
    std::condition_variable mContinueAudioCond;

    std::unique_ptr<XImageQueue> mImageQueue;
    std::unique_ptr<XSampleQueue> mSampleQueue;

    uint8_t* mSampleData;
    int mSampleDataIndex;
    int mSampleBufferSize;
    int mSampleBufferSizeMax;
    int mDstSampleCountMax;

    std::unique_ptr<SwsContext, SwsContextDeleter> mSwsContext;
    std::unique_ptr<SwrContext, SwrContextDeleter> mSwrContext;

    int mPutVideoPacket = 0;
    int mPutAudioPacket = 0;
    int mGetVideoPacket = 0;
    int mGetAudioPacket = 0;
    
    long mLastReqClock;
    long mLastFrameClock;

    std::shared_ptr<Packet> mLastPacket;
    
    int mRobotIndex = 0;
    
    int mBFrameIndex;
    
    std::vector<long> mKeyTimestamps;
};


#endif //XEXPORTER_XFFPRODUCER_H
