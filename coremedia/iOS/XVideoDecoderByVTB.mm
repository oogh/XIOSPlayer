//
//  XVideoDecoderByVTB.mm
//  XIOSPlayer
//
//  Created by oogh on 2021/1/14.
//

#import "XVideoDecoderByVTB.hpp"
#include "XFFHeader.hpp"

#define PRINT_IMAGE_ALLOC_LOG 0
#define PRINT_PLAY_LOG 0

void err2string(OSStatus);

void decompressionOutputCallback(void *decompressionOutputRefCon,
                                 void *sourceFrameRefCon,
                                 OSStatus status,
                                 VTDecodeInfoFlags infoFlags,
                                 CVImageBufferRef imageBuffer,
                                 CMTime presentationTimeStamp,
                                 CMTime presentationDuration);

struct DecoderContext {
    std::shared_ptr<AVFormatContext> formatCtx;
    int videoIndex = -1;
    
    uint8_t* vps = nullptr;
    size_t vpsSize = 0;
    uint8_t* sps = nullptr;
    size_t spsSize = 0;
    uint8_t* pps = nullptr;
    size_t ppsSize = 0;
    
    CMVideoFormatDescriptionRef videoFormatDescription;
    VTDecompressionSessionRef decompressionSession;
    
    ~DecoderContext() {
        if (formatCtx) {
            formatCtx.reset();
        }
        
        videoIndex = -1;
        
        if (vps) {
            delete [] vps;
            vps = nullptr;
        }
        if (sps) {
            delete [] sps;
            sps = nullptr;
        }
        if (pps) {
            delete [] pps;
            pps = nullptr;
        }
        vpsSize = 0;
        spsSize = 0;
        ppsSize = 0;
        
        if (decompressionSession) {
            VTDecompressionSessionInvalidate(decompressionSession);
            CFRelease(decompressionSession);
            decompressionSession = nullptr;
        }
        
        if (videoFormatDescription) {
            CFRelease(videoFormatDescription);
            videoFormatDescription = nullptr;
        }
    }
    
};

XVideoDecoderByVTB::XVideoDecoderByVTB(const std::string& filename): mFilename(filename), mIsReadyForData(false), mPtsMax(-1), mLastClock(-1) {
    
}

XVideoDecoderByVTB::~XVideoDecoderByVTB() {
    
}

int XVideoDecoderByVTB::open() {
    
    if (mCtx) {
        mCtx.reset();
    }
    
    mCtx = std::make_shared<DecoderContext>();
    
    int ret;
    AVFormatContext* ic = nullptr;
    
    ret = avformat_open_input(&ic, mFilename.data(), nullptr, nullptr);
    if (ret < 0) {
        return ret;
    }
    mCtx->formatCtx = std::shared_ptr<AVFormatContext>(ic, InputFormatDeleter());
    
    ret = avformat_find_stream_info(ic, nullptr);
    if (ret < 0) {
        return ret;
    }
    
    for (int i = 0; i < ic->nb_streams; ++i) {
        mCtx->formatCtx->streams[i]->discard = AVDISCARD_ALL;
    }
    
    int index = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (index < 0) {
        return AVERROR_STREAM_NOT_FOUND;
    }
    mCtx->videoIndex = index;
    
    mCtx->formatCtx->streams[index]->discard = AVDISCARD_DEFAULT;
    
    ret = createVTDecompressionSession();
    if (ret < 0) {
        return ret;
    }
    
    mImageList = std::make_shared<ImageList>();
    
    return 0;
}

int XVideoDecoderByVTB::parseVideoStream() {
    
    if (!mCtx || !mCtx->formatCtx || mCtx->videoIndex < 0) {
        return AVERROR(EPERM);
    }
    
    AVCodecParameters* codecpar = mCtx->formatCtx->streams[mCtx->videoIndex]->codecpar;
    int vpsIndex = 0;
    int spsIndex = 0;
    int ppsIndex = 0;
    
    if (codecpar->codec_id == AV_CODEC_ID_H264) {
        spsIndex = 8;
        mCtx->spsSize = static_cast<size_t>(codecpar->extradata[spsIndex - 2] + codecpar->extradata[spsIndex - 1]);

        ppsIndex = static_cast<int>(spsIndex + mCtx->spsSize + 2 + 1);
        mCtx->ppsSize = static_cast<size_t>(codecpar->extradata[ppsIndex - 2] + codecpar->extradata[ppsIndex - 1]);
    } else if (codecpar->codec_id == AV_CODEC_ID_HEVC) {
        int i = 23;
        for (;;) {
            if (vpsIndex > 0 && spsIndex > 0 && ppsIndex > 0) {
                break;
            }
            switch (codecpar->extradata[i] & 0x3F) {
                case 32: {
                    vpsIndex = i + 5;
                    mCtx->vpsSize = static_cast<size_t>(codecpar->extradata[vpsIndex - 2] + codecpar->extradata[vpsIndex - 1]);
                    i = static_cast<int>(vpsIndex + mCtx->vpsSize);
                } break;
                case 33: {
                    spsIndex = i + 5;
                    mCtx->spsSize = static_cast<size_t>(codecpar->extradata[spsIndex - 2] + codecpar->extradata[spsIndex - 1]);
                    i = static_cast<int>(spsIndex + mCtx->spsSize);
                } break;
                case 34: {
                    ppsIndex = i + 5;
                    mCtx->ppsSize = static_cast<size_t>(codecpar->extradata[ppsIndex - 2] + codecpar->extradata[ppsIndex - 1]);
                    i = static_cast<int>(ppsIndex + mCtx->ppsSize);
                } break;
                default:
                    break;
            }
        }
    }
    
    mCtx->vps = new uint8_t[mCtx->vpsSize];
    memcpy(mCtx->vps, &codecpar->extradata[vpsIndex], mCtx->vpsSize);
    mCtx->sps = new uint8_t[mCtx->spsSize];
    memcpy(mCtx->sps, &codecpar->extradata[spsIndex], mCtx->spsSize);
    mCtx->pps = new uint8_t[mCtx->ppsSize];
    memcpy(mCtx->pps, &codecpar->extradata[ppsIndex], mCtx->ppsSize);
    
    return 0;
}

int XVideoDecoderByVTB::createVideoFormatDescription() {
    int ret;
    OSStatus status = 0;
    size_t parameterSetCount;
    int NALUnitHeaderLength = 4;
    
    ret = parseVideoStream();
    if (ret < 0) {
        return ret;
    }
    
    AVCodecParameters* codecpar = mCtx->formatCtx->streams[mCtx->videoIndex]->codecpar;
    if (codecpar->codec_id == AV_CODEC_ID_H264) {
        parameterSetCount = 2;
        const uint8_t* parameterSetPointers[2] = {mCtx->sps, mCtx->pps};
        size_t parameterSetSizes[2] = {mCtx->spsSize, mCtx->ppsSize};
        status = CMVideoFormatDescriptionCreateFromH264ParameterSets(kCFAllocatorDefault,
                                                                     parameterSetCount,
                                                                     parameterSetPointers,
                                                                     parameterSetSizes,
                                                                     NALUnitHeaderLength,
                                                                     &(mCtx->videoFormatDescription));
    } else if (codecpar->codec_id == AV_CODEC_ID_HEVC) {
        parameterSetCount = 3;
        const uint8_t* parameterSetPointers[3] = {mCtx->vps, mCtx->sps, mCtx->pps};
        size_t parameterSetSizes[3] = {mCtx->vpsSize, mCtx->spsSize, mCtx->ppsSize};
        
        if (@available(iOS 11.0, *)) {
            status = CMVideoFormatDescriptionCreateFromHEVCParameterSets(kCFAllocatorDefault,
                                                                         parameterSetCount,
                                                                         parameterSetPointers,
                                                                         parameterSetSizes,
                                                                         NALUnitHeaderLength,
                                                                         nullptr,
                                                                         &(mCtx->videoFormatDescription));
        } else {
            // Fallback on earlier versions
        }
    }
    
    if (status != noErr) {
        err2string(status);
        return status;
    }
    
    return 0;
}

int XVideoDecoderByVTB::createVTDecompressionSession() {
    int ret = createVideoFormatDescription();
    if (ret < 0) {
        return ret;
    }
    
    CFDictionaryRef videoDecoderSpecification = nullptr;
    
    CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(mCtx->videoFormatDescription);
    
    NSDictionary* pixelBufferAttributes = @{
        (id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange),
        (id)kCVPixelBufferWidthKey: @(dimensions.width),
        (id)kCVPixelBufferHeightKey: @(dimensions.height),
        (id)kCVPixelBufferOpenGLESCompatibilityKey:@(YES)
    };
    
    VTDecompressionOutputCallbackRecord outputCallback;
    outputCallback.decompressionOutputCallback = decompressionOutputCallback;
    outputCallback.decompressionOutputRefCon = this;
    
    OSStatus status = VTDecompressionSessionCreate(kCFAllocatorDefault,
                                                   mCtx->videoFormatDescription,
                                                   videoDecoderSpecification,
                                                   (__bridge CFDictionaryRef)pixelBufferAttributes,
                                                   &outputCallback,
                                                   &(mCtx->decompressionSession));
    if (status != noErr) {
        err2string(status);
        return status;
    }
    return 0;
}

std::shared_ptr<XImage> XVideoDecoderByVTB::getImage(int& code, long clock) {
    
    int ret;
    AVStream* st = mCtx->formatCtx->streams[mCtx->videoIndex];
    
    for (;;) {
        auto image = mImageList->get(clock);
        if (image) {
            return image;
        }
        
        mIsReadyForData = false;
        
        auto pkt = std::make_shared<Packet>();
        ret = av_read_frame(mCtx->formatCtx.get(), pkt->avpkt);
        if (ret < 0) {
            code = ret;
            return nullptr;
        } else {
            if (pkt->avpkt->stream_index == mCtx->videoIndex && !(st->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
                av_packet_rescale_ts(pkt->avpkt, st->time_base, {1, 1000});
                ret = decode(pkt->avpkt->data,
                             pkt->avpkt->size,
                             static_cast<long>(pkt->avpkt->pts),
                             static_cast<long>(pkt->avpkt->dts),
                             static_cast<long>(pkt->avpkt->duration));
//                NSLog(@"andy send pts:%lld dts: %lld ret: %d", pkt->avpkt->pts, pkt->avpkt->dts, ret);
            }
        }
    }
    
    return nullptr;
}

int XVideoDecoderByVTB::decode(uint8_t* data, size_t size, long pts, long dts, long duration) {
    CMBlockBufferRef blockBuffer = nullptr;
    OSStatus status;
    status = CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault,
                                                reinterpret_cast<void*>(data),
                                                size,
                                                kCFAllocatorNull,
                                                nullptr,
                                                0,
                                                size,
                                                0,
                                                &blockBuffer);
    if (status != noErr) {
        err2string(status);
        return status;
    }
    
    CMTime ptsTime = CMTimeMakeWithSeconds(static_cast<Float64>(pts), NSEC_PER_SEC);
    CMTime dtsTime = CMTimeMakeWithSeconds(static_cast<Float64>(dts), NSEC_PER_SEC);
    CMTime durationTime = CMTimeMakeWithSeconds(duration, NSEC_PER_SEC);
    CMSampleTimingInfo timeInfo = {durationTime, ptsTime, dtsTime};
    size_t sampleSizeArray[] = {size};
    CMSampleBufferRef sampleBuffer = nullptr;
    status = CMSampleBufferCreateReady(kCFAllocatorDefault,
                                       blockBuffer,
                                       mCtx->videoFormatDescription,
                                       1,
                                       1,
                                       &timeInfo,
                                       1,
                                       sampleSizeArray,
                                       &sampleBuffer);
    
    if (status != noErr) {
        CFRelease(blockBuffer);
        err2string(status);
        return status;
    }
    
    VTDecodeFrameFlags decodeFlags = kVTDecodeFrame_EnableAsynchronousDecompression;
    
    void* sourceFrameRefCon = nullptr;
    VTDecodeInfoFlags infoFlagsOut = kVTDecodeInfo_Asynchronous;
    status = VTDecompressionSessionDecodeFrame(mCtx->decompressionSession,
                                                        sampleBuffer,
                                                        decodeFlags,
                                                        sourceFrameRefCon,
                                                        &infoFlagsOut);
    
    CFRelease(blockBuffer);
    CFRelease(sampleBuffer);
    
    if (status != noErr) {
        err2string(status);
        return status;
    }
    return 0;
}

void XVideoDecoderByVTB::push(std::shared_ptr<XImage>&& image) {
    mImageList->put(std::forward<std::shared_ptr<XImage>>(image));
}

int XImage::ID_COUNTER = 1;

struct TempPacket {
    uint8_t* data = nullptr;
    int size = 0;
    double pts = -1.0f;
    double dts = -1.0f;
    double duration = 0.0f;
    
    TempPacket(uint8_t* data, int size, double pts, double dts, double duration) {
        this->data = reinterpret_cast<uint8_t*>(malloc(size));
        memcpy(this->data, data, size);
        this->size = size;
        this->pts = pts;
        this->dts = dts;
        this->duration = duration;
    }
    
    TempPacket(const TempPacket& that) {
        this->data = reinterpret_cast<uint8_t*>(malloc(that.size));
        memcpy(this->data, that.data, that.size);
        this->size = that.size;
        this->pts = that.pts;
        this->dts = that.dts;
        this->duration = that.duration;
    }
    
    TempPacket& operator=(const TempPacket& that) {
        if (this != &that) {
            this->data = reinterpret_cast<uint8_t*>(malloc(that.size));
            memcpy(this->data, that.data, that.size);
            this->size = that.size;
            this->pts = that.pts;
            this->dts = that.dts;
            this->duration = that.duration;
        }
        return *this;
    }
    
    TempPacket(TempPacket&& that) {
        this->data = that.data;
        this->size = that.size;
        this->pts = that.pts;
        this->dts = that.dts;
        this->duration = that.duration;
        
        that.data = nullptr;
        that.size = 0;
        that.pts = -1.0f;
        that.dts = -1.0f;
        that.duration = 0.0f;
    }
    
    TempPacket& operator=(TempPacket&& that) {
        if (this != &that) {
            this->data = that.data;
            this->size = that.size;
            this->pts = that.pts;
            this->dts = that.dts;
            this->duration = that.duration;
            
            that.data = nullptr;
            that.size = 0;
            that.pts = -1.0f;
            that.dts = -1.0f;
            that.duration = 0.0f;
        }
        return *this;
    }
    
    ~TempPacket() {
        if (this->data) {
            free(this->data);
            this->data = nullptr;
        }
        this->size = 0;
        this->pts = -1.0f;
        this->dts = -1.0f;
        this->duration = 0.0f;
    }
};

void decompressionOutputCallback(void *decompressionOutputRefCon,
                                 void *sourceFrameRefCon,
                                 OSStatus status,
                                 VTDecodeInfoFlags infoFlags,
                                 CVImageBufferRef imageBuffer,
                                 CMTime presentationTimeStamp,
                                 CMTime presentationDuration) {
    XVideoDecoderByVTB* decoder = static_cast<XVideoDecoderByVTB*>(decompressionOutputRefCon);
    if (status != noErr) {
        err2string(status);
        return;
    }

    if (imageBuffer) {
        OSStatus status;
        CMSampleTimingInfo timeInfo = {presentationTimeStamp, presentationDuration, kCMTimeInvalid};
        CMVideoFormatDescriptionRef formatDescription = nullptr;
        status = CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault,
                                                              imageBuffer,
                                                              &formatDescription);
        if (status != noErr) {
            err2string(status);
            return;
        }
        
        CMSampleBufferRef sampleBuffer = nullptr;
        status = CMSampleBufferCreateReadyWithImageBuffer(kCFAllocatorDefault,
                                                          imageBuffer,
                                                          formatDescription,
                                                          &timeInfo,
                                                          &sampleBuffer);
        
        CFRelease(formatDescription);
        if (status != noErr) {
            err2string(status);
            return;
        }
        
        auto image = std::make_shared<XImage>();
        CMSampleBufferCreateCopy(kCFAllocatorDefault, sampleBuffer, &image->sampleBuffer);
        CFRelease(sampleBuffer);
        image->pts = static_cast<long>(CMTimeGetSeconds(presentationTimeStamp));
        image->duration = static_cast<long>(CMTimeGetSeconds(presentationDuration));
        decoder->push(std::move(image));
//        NSLog(@"andy ------receive pts: %.2f", CMTimeGetSeconds(presentationTimeStamp));
    }
}

void err2string(OSStatus status) {
    switch (status) {
        case kCMFormatDescriptionError_InvalidParameter:
            NSLog(@"Invalid Argument");
            break;
            
        case kCMSampleBufferError_InvalidMediaFormat:
            NSLog(@"Invalid MediaFormat");
            break;
            
        case kCMSampleBufferError_RequiredParameterMissing:
            NSLog(@"Required Parameter Missing");
            break;
            
        case kVTVideoDecoderBadDataErr:
            NSLog(@"Video Decoder Bad Data");
            break;
            
        case kVTInvalidSessionErr:
            NSLog(@"Invalid VTDecompression Session");
            break;
            
        case kVTParameterErr:
            NSLog(@"Invalid Parameter");
            break;
            
        default:
            NSLog(@"Unkown Error");
            break;
    }
}
