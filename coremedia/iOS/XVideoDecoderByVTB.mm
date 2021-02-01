//
//  XVideoDecoderByVTB.m
//  XIOSPlayer
//
//  Created by oogh on 2021/1/14.
//

#import <VideoToolbox/VideoToolbox.h>
#import "XVideoDecoderByVTB.hpp"
#include "XFFHeader.hpp"
#include <memory>
#include <string>
#include <list>
#include <mutex>

#define PRINT_IMAGE_ALLOC 0

struct XImage {
    CMSampleBufferRef sampleBuffer = nullptr;
    long pts = -1L;
    long duration = 0L;
    int imgId = 0;
    static int ID_COUNTER;
    
    XImage() {
        imgId = ID_COUNTER++;
#if PRINT_IMAGE_ALLOC
        NSLog(@"XImage() id: %d", imgId);
#endif
    }
    
    XImage(const XImage& that) {
        CMSampleBufferCreateCopy(kCFAllocatorDefault, that.sampleBuffer, &sampleBuffer);
        this->pts = that.pts;
        this->duration = that.duration;
#if PRINT_IMAGE_ALLOC
        NSLog(@"XImage(const XImage& that) id: %d", imgId);
#endif
    }
    
    XImage& operator=(const XImage& that) {
        if (this != &that) {
            CMSampleBufferCreateCopy(kCFAllocatorDefault, that.sampleBuffer, &sampleBuffer);
            this->pts = that.pts;
            this->duration = that.duration;
        }
#if PRINT_IMAGE_ALLOC
        NSLog(@"XImage& operator=(const XImage& that) id: %d", imgId);
#endif
        return *this;
    }
    
    XImage(XImage&& that) {
        this->sampleBuffer = that.sampleBuffer;
        this->pts = that.pts;
        this->duration = that.duration;

        that.sampleBuffer = nullptr;
        that.pts = -1L;
        that.duration = 0L;
#if PRINT_IMAGE_ALLOC
        NSLog(@"XImage(XImage&& that) id: %d", imgId);
#endif
    }
    
    XImage& operator=(XImage&& that) {
        if (this != &that) {
            this->sampleBuffer = that.sampleBuffer;
            this->pts = that.pts;
            this->duration = that.duration;
            
            that.sampleBuffer = nullptr;
            that.pts = -1L;
            that.duration = 0L;
        }
#if PRINT_IMAGE_ALLOC
        NSLog(@"XImage& operator=(XImage&& that) id: %d", imgId);
#endif
        return *this;
    }
    
    ~XImage() {
        if (sampleBuffer) {
            CFRelease(sampleBuffer);
            sampleBuffer = nullptr;
        }
        pts = -1L;
        duration = 0L;
#if PRINT_IMAGE_ALLOC
        NSLog(@"~XImage() id: %d", imgId);
#endif
    }
};

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

void err2string(OSStatus status);

@interface XVideoDecoderByVTB() {
    NSString* _path;
    std::unique_ptr<AVFormatContext, InputFormatDeleter> _formatCtx;
    int _videoIndex;
    
    CMVideoFormatDescriptionRef videoFormatDescription;
    VTDecompressionSessionRef decompressionSession;
    
    int _width;
    int _height;
    
    int _reorderSize;
    bool _hasEnoughFrame;
    
    std::unique_ptr<TempPacket> _tempPkt;
    
    std::list<std::shared_ptr<XImage>> _imageList;
    std::mutex _mutex;
    std::condition_variable _producerCond;
    std::condition_variable _consumerCond;
    
    std::shared_ptr<XImage> _lastImage;
    long _lastClock;
}

@end

@implementation XVideoDecoderByVTB

#pragma mark -- LifeCycle
- (instancetype)initWithPath:(NSString*)path {
    if (self = [super init]) {
        _path = path;
        _reorderSize = 8;
        _hasEnoughFrame = false;
        _lastClock = -1L;
    }
    return self;
}

- (void)dealloc {
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


#pragma mark -- Public

void decompressionOutputCallback(void *decompressionOutputRefCon,
                                 void *sourceFrameRefCon,
                                 OSStatus status,
                                 VTDecodeInfoFlags infoFlags,
                                 CVImageBufferRef imageBuffer,
                                 CMTime presentationTimeStamp,
                                 CMTime presentationDuration) {
    if (status != noErr) {
        err2string(status);
        return;
    }
    
    XVideoDecoderByVTB* decoder = (__bridge XVideoDecoderByVTB*)decompressionOutputRefCon;
    
    [decoder pushBack:CVPixelBufferRetain(imageBuffer) pts:presentationTimeStamp duration:presentationDuration];
}

- (CVPixelBufferRef)getPixelBuffer:(long)clock {
    if (clock == _lastClock) {
        if (_lastImage) {
            CMSampleBufferRef sampleBuffer = nullptr;
            CMSampleBufferCreateCopy(kCFAllocatorDefault, _lastImage->sampleBuffer, &sampleBuffer);
            return CMSampleBufferGetImageBuffer(sampleBuffer);
        }
    }
    _lastClock = clock;
    if (_imageList.size() >= _reorderSize) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto iter = _imageList.begin();
        for (;;) {
            auto prev = *iter;
            if (clock < prev->pts) {
                if (_lastImage &&
                    _lastImage->pts <= clock &&
                    clock < _lastImage->pts + _lastImage->duration) {
                    CMSampleBufferRef sampleBuffer = nullptr;
                    CMSampleBufferCreateCopy(kCFAllocatorDefault, _lastImage->sampleBuffer, &sampleBuffer);
                    return CMSampleBufferGetImageBuffer(sampleBuffer);
                } else {
                    // TODO 执行seek，获取新的帧
                }
            } else if (prev->pts <= clock && clock < prev->pts + prev->duration) {
                _lastImage = prev;
                _imageList.erase(iter++);
                CMSampleBufferRef sampleBuffer = nullptr;
                CMSampleBufferCreateCopy(kCFAllocatorDefault, _lastImage->sampleBuffer, &sampleBuffer);
                return CMSampleBufferGetImageBuffer(sampleBuffer);
            } else {
                _lastImage = prev;
                _imageList.erase(iter++);
                if (iter != _imageList.end()) {
                    auto next = *iter;
                    if (clock < next->pts) {
                        CMSampleBufferRef sampleBuffer = nullptr;
                        CMSampleBufferCreateCopy(kCFAllocatorDefault, _lastImage->sampleBuffer, &sampleBuffer);
                        return CMSampleBufferGetImageBuffer(sampleBuffer);
                    } else if (next->pts <= clock && clock < next->pts + next->duration) {
                        _lastImage = next;
                        _imageList.erase(iter++);
                        CMSampleBufferRef sampleBuffer = nullptr;
                        CMSampleBufferCreateCopy(kCFAllocatorDefault, _lastImage->sampleBuffer, &sampleBuffer);
                        return CMSampleBufferGetImageBuffer(sampleBuffer);
                    } else {
                        continue;
                    }
                } else {
                    _lastImage = prev;
                    CMSampleBufferRef sampleBuffer = nullptr;
                    CMSampleBufferCreateCopy(kCFAllocatorDefault, _lastImage->sampleBuffer, &sampleBuffer);
                    return CMSampleBufferGetImageBuffer(sampleBuffer);
                }
            }
        }
    } else {
        int ret;
        if (_tempPkt) {
            ret = [self sendPacket:_tempPkt->data
                              size:_tempPkt->size
                               pts:static_cast<double>(_tempPkt->pts)
                               dts:static_cast<double>(_tempPkt->dts)
                          duration:static_cast<double>(_tempPkt->duration)];
            if (ret >= 0) {
                _tempPkt = nullptr;
            }
        } else {
            AVPacket pkt;
            ret = av_read_frame(_formatCtx.get(), &pkt);
            if (ret < 0) {
                NSLog(@"andy av_read_frame() failed: %s", av_err2str(ret));
                return nullptr;
            }
            if (pkt.data && pkt.stream_index == _videoIndex) {
                av_packet_rescale_ts(&pkt, _formatCtx->streams[_videoIndex]->time_base, {1, 1000});
                ret = [self sendPacket:pkt.data
                                  size:pkt.size
                                   pts:static_cast<double>(pkt.pts)
                                   dts:static_cast<double>(pkt.dts)
                              duration:static_cast<double>(pkt.duration)];
                if (ret == -11) {
                    _tempPkt = std::make_unique<TempPacket>(pkt.data,
                                                            pkt.size,
                                                            static_cast<double>(pkt.pts),
                                                            static_cast<double>(pkt.dts),
                                                            static_cast<double>(pkt.duration));
                }
            }
            av_packet_unref(&pkt);
        }
    }
    
    return nullptr;
}

#pragma mark -- Private

- (int)open {
    
    AVFormatContext* ic = nullptr;
    int ret;
    
    ret = avformat_open_input(&ic, _path.UTF8String, nullptr, nullptr);
    if (ret < 0) {
        return ret;
    }
    _formatCtx = std::unique_ptr<AVFormatContext, InputFormatDeleter>(ic);
    
    ret = avformat_find_stream_info(ic, nullptr);
    if (ret < 0) {
        return ret;
    }
    
    _videoIndex = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (_videoIndex < 0) {
        return AVERROR_STREAM_NOT_FOUND;
    }
    
    AVStream* st = ic->streams[_videoIndex];
    AVCodecParameters* codecpar = st->codecpar;
    
    int spsIndex = 0;
    int ppsIndex = 0;
    size_t spsLength = 0;
    size_t ppsLength = 0;
    
    for (int i = 2; i < codecpar->extradata_size; ++i) {
        if (codecpar->extradata[i] == 0x67
            && codecpar->extradata[i - 2] == 0x00) {
            spsIndex = i;
            spsLength = codecpar->extradata[i - 1];
        }
        
        if (codecpar->extradata[i] == 0x68
            && codecpar->extradata[i - 2] == 0x00) {
            ppsIndex = i;
            ppsLength = codecpar->extradata[i - 1];
        }
    }
    
    uint8_t* sps = new uint8_t[spsLength];
    memcpy(sps, &codecpar->extradata[spsIndex], spsLength);
    uint8_t* pps = new uint8_t[ppsLength];
    memcpy(pps, &codecpar->extradata[ppsIndex], ppsLength);
    
    [self setupVideoFormatDescription:sps spsLength:spsLength pps:pps ppsLength:ppsLength];
    [self setupVideoToolBox];
    
    return 0;
}

- (int)setupVideoFormatDescription:(uint8_t*)sps
                         spsLength:(size_t)spsLength
                               pps:(uint8_t*)pps
                         ppsLength:(size_t)ppsLength {
    OSStatus status;
    size_t parameterSetCount = 2;
    const uint8_t* parameterSetPointers[2] = {sps, pps};
    size_t parameterSetSizes[2] = {spsLength, ppsLength};
    int NALUnitHeaderLength = 4;
    
    status = CMVideoFormatDescriptionCreateFromH264ParameterSets(kCFAllocatorDefault,
                                                                 parameterSetCount,
                                                                 parameterSetPointers,
                                                                 parameterSetSizes,
                                                                 NALUnitHeaderLength,
                                                                 &videoFormatDescription);
    if (status != noErr) {
        err2string(status);
        return status;
    }
    
    return 0;
}

- (int)setupVideoToolBox {
    CFDictionaryRef videoDecoderSpecification = nullptr;
    
    CMVideoDimensions dimensions = CMVideoFormatDescriptionGetDimensions(videoFormatDescription);
    _width = dimensions.width;
    _height = dimensions.height;
    
    NSDictionary* pixelBufferAttributes = @{
        (id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange),
        (id)kCVPixelBufferWidthKey: @(_width),
        (id)kCVPixelBufferHeightKey: @(_height),
        (id)kCVPixelBufferOpenGLESCompatibilityKey:@(YES)
    };
    
    VTDecompressionOutputCallbackRecord outputCallback;
    outputCallback.decompressionOutputCallback = decompressionOutputCallback;
    outputCallback.decompressionOutputRefCon = (__bridge void *)self;
    
    OSStatus status = VTDecompressionSessionCreate(kCFAllocatorDefault,
                                                   videoFormatDescription,
                                                   videoDecoderSpecification,
                                                   (__bridge CFDictionaryRef)pixelBufferAttributes,
                                                   &outputCallback,
                                                   &decompressionSession);
    if (status != noErr) {
        err2string(status);
        return status;
    }
    return 0;
}


#pragma mark -- Private

- (int)sendPacket:(uint8_t*)data size:(size_t)size pts:(double)pts dts:(double)dts duration:(double)duration {
    
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_imageList.size() >= _reorderSize) {
            return -11;
        }
    }

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
    
    CMTime ptsTime = CMTimeMakeWithSeconds(pts, NSEC_PER_SEC);
    CMTime dtsTime = CMTimeMakeWithSeconds(dts, NSEC_PER_SEC);
    CMTime durationTime = CMTimeMakeWithSeconds(duration, NSEC_PER_SEC);
    CMSampleTimingInfo timeInfo = {durationTime, ptsTime, dtsTime};
    size_t sampleSizeArray[] = {size};
    CMSampleBufferRef sampleBuffer = nullptr;
    status = CMSampleBufferCreateReady(kCFAllocatorDefault,
                                       blockBuffer,
                                       videoFormatDescription,
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
    
    VTDecodeFrameFlags decodeFlags = kVTDecodeFrame_EnableAsynchronousDecompression | kVTDecodeFrame_EnableTemporalProcessing;
    
    void* sourceFrameRefCon = nullptr;
    VTDecodeInfoFlags infoFlagsOut = kVTDecodeInfo_Asynchronous | kVTDecodeInfo_FrameDropped;
    status = VTDecompressionSessionDecodeFrame(decompressionSession,
                                                        sampleBuffer,
                                                        decodeFlags,
                                                        sourceFrameRefCon,
                                                        &infoFlagsOut);
    if (status != noErr) {
        CFRelease(blockBuffer);
        CFRelease(sampleBuffer);
        err2string(status);
        return status;
    }
    
    CFRelease(blockBuffer);
    CFRelease(sampleBuffer);
    return 0;
}

- (int)receiveFrame:(XImage&)image {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_imageList.size() < _reorderSize) {
        return -11;
    }

    auto front = _imageList.front();
    image = *front;
    _imageList.pop_front();
    return 0;
}

- (void)pushBack:(CVImageBufferRef)imageBuffer pts:(CMTime)pts duration:(CMTime)duration {
    
    if (imageBuffer) {
        OSStatus status;
        CMSampleTimingInfo timeInfo = {pts, duration, kCMTimeInvalid};
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
        image->pts = static_cast<long>(CMTimeGetSeconds(pts));
        image->duration = static_cast<long>(CMTimeGetSeconds(duration));
        
        std::lock_guard<std::mutex> lock(_mutex);
        if (_imageList.size() <= 0) {
            _imageList.emplace_back(std::move(image));
        } else {
            std::list<std::shared_ptr<XImage>>::iterator iter;
            for (iter = _imageList.begin(); iter != _imageList.end(); ++iter) {
                auto temp = *iter;
                if (image->pts < temp->pts) {
                    break;
                }
            }
            
            if (iter != _imageList.end()) {
                _imageList.insert(iter, std::move(image));
            } else {
                _imageList.emplace_back(std::move(image));
            }
        }
    }
}

@end

void err2string(OSStatus status) {
    switch (status) {
        case kCMFormatDescriptionBridgeError_InvalidParameter:
            NSLog(@"Invalid Argument");
            break;
            
        case kCMSampleBufferError_InvalidMediaFormat:
            NSLog(@"Invalid MediaFormat");
            break;
            
        case kCMSampleBufferError_RequiredParameterMissing:
            NSLog(@"Required Parameter Missing");
            break;
            
        default:
            NSLog(@"Unkown Error");
            break;
    }
}
