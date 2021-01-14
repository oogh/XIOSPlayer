//
//  XFFmpegVideoCodec.hpp
//  XRender
//
//  Created by Oogh on 2020/9/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XFFMPEGVIDEOCODEC_HPP
#define XFFMPEGVIDEOCODEC_HPP

#include <thread>
#include "XFFHeader.hpp"
#include "XVideoCodec.hpp"

class XFFmpegVideoCodec : public XVideoCodec {
public:
    XFFmpegVideoCodec();

    ~XFFmpegVideoCodec() override;

    std::shared_ptr<XImage> getImage(long clock) override;
    
private:
    int open() override;

    int close() override;

private:
    void frameConvert(std::shared_ptr<XImage> dst, AVFrame* src);

private:
    bool checkIsValidPacket(AVPacket* pkt);

private:
    const AVPixelFormat DST_PIX_FMT = AV_PIX_FMT_RGBA;

private:
    std::shared_ptr<AVFormatContext> mFormatCtx;

    int mIndex;
    std::unique_ptr<AVCodecContext, CodecDeleter> mCodecCtx;
    std::unique_ptr<std::thread> mVideoTid;
    std::unique_ptr<SwsContext, SwsContextDeleter> mSwsContext;

    ///< 在一个GOP中，两个P帧之间的B帧的数量 这里是一个丢B帧的逻辑。如果在两个P帧之间存在多个B帧，则丢弃第奇数个B帧
    int mBFrameIndex;

    ///< 上一次取到的帧
    std::shared_ptr<XImage> mLastImage;
    std::shared_ptr<Frame> mLastFrame;
    long mLastClock;
    static AVPixelFormat mHWPixelFormat;
};


#endif //XFFMPEGVIDEOCODEC_HPP
