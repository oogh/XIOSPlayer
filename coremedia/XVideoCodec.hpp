//
//  XVideoCodec.hpp
//  XRender
//
//  Created by Oogh on 2020/9/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XVIDEOCODEC_HPP
#define XVIDEOCODEC_HPP

#include <memory>
#include <string>
#include "XImage.hpp"

struct XVideoCodecContext;

class XVideoCodec {
public:
    XVideoCodec();

    virtual ~XVideoCodec();

    virtual void setFilename(const std::string& filename);

    void setLoop(bool loop);
    
    virtual int open() = 0;

    virtual std::shared_ptr<XImage> getImage(long clock) = 0;
    
    virtual int close() = 0;

protected:
    int mStatus;
    const int S_READ_EOF = 1 << 0;
    const int S_VIDEO_DECODE_EOF = 1 << 1;

protected:
    std::string mFilename;
    bool mLoop;
    std::shared_ptr<XVideoCodecContext> mContext;
    
    long mLastRequestClock;
};


#endif //XVIDEOCODEC_HPP
