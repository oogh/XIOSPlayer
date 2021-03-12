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

#define PRINT_IMAGE_ALLOC_LOG 0
#define PRINT_PLAY_LOG 0

struct XImage {
    CMSampleBufferRef sampleBuffer = nullptr;
    long pts = -1L;
    long duration = 0L;
    int imgId = 0;
    static int ID_COUNTER;
    
    XImage() {
        imgId = ID_COUNTER++;
#if PRINT_IMAGE_ALLOC_LOG
        NSLog(@"XImage() id: %d", imgId);
#endif
    }
    
    XImage(const XImage& that) {
        CMSampleBufferCreateCopy(kCFAllocatorDefault, that.sampleBuffer, &sampleBuffer);
        this->pts = that.pts;
        this->duration = that.duration;
#if PRINT_IMAGE_ALLOC_LOG
        NSLog(@"XImage(const XImage& that) id: %d", imgId);
#endif
    }
    
    XImage& operator=(const XImage& that) {
        if (this != &that) {
            CMSampleBufferCreateCopy(kCFAllocatorDefault, that.sampleBuffer, &sampleBuffer);
            this->pts = that.pts;
            this->duration = that.duration;
        }
#if PRINT_IMAGE_ALLOC_LOG
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
#if PRINT_IMAGE_ALLOC_LOG
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
#if PRINT_IMAGE_ALLOC_LOG
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
#if PRINT_IMAGE_ALLOC_LOG
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
    
    std::unique_ptr<TempPacket> _tempPkt;
    
    std::list<std::shared_ptr<XImage>> _imageList;
    std::mutex _mutex;
    std::condition_variable _producerCond;
    std::condition_variable _consumerCond;
    
    std::shared_ptr<XImage> _lastImage;
    long _lastClock;
    
    std::shared_ptr<XImage> _tempImage;
    
    bool _ready;
    bool _sorted;
    long _prevMaxClock;
    
    long _duration;
    
    CFAbsoluteTime _lastTime;
    
    int _count;
    bool _seekReq;
    
    CFAbsoluteTime _totalTime;
    bool _eof;
}

@end

@implementation XVideoDecoderByVTB

#pragma mark -- LifeCycle
- (instancetype)initWithPath:(NSString*)path {
    if (self = [super init]) {
        _path = path;
        _reorderSize = 9;
        _lastClock = -1L;
        
        _ready = false;
        _sorted = false;
        _prevMaxClock = 0L;
        
        _duration = 0L;
        _count = 0;
        
        _seekReq = false;
        _totalTime = 0.0f;
        _eof = false;
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
    XVideoDecoderByVTB* decoder = (__bridge XVideoDecoderByVTB*)decompressionOutputRefCon;
    if (status != noErr) {
//        if (status == kVTVideoDecoderBadDataErr) {
//            [decoder resetVTDecompressionSession];
//        }
        err2string(status);
        return;
    }
    CFAbsoluteTime d = CFAbsoluteTimeGetCurrent() - decoder->_lastTime;
    if (d < 1000) {
        decoder->_totalTime += d;
        decoder->_count++;
    }
    if (!decoder->_eof) {
        NSLog(@"andy callback 耗时: %f, count: %d", d * 1000.0, decoder->_count);
    }
    decoder->_lastTime = CFAbsoluteTimeGetCurrent();

#if PRINT_IMAGE_ALLOC_LOG
    NSLog(@"andy pts: %.2f", CMTimeGetSeconds(presentationTimeStamp));
#endif
    [decoder pushBack:CVPixelBufferRetain(imageBuffer) pts:presentationTimeStamp duration:presentationDuration];
}

- (int)decodeVideoFrame {
    int ret;
    
    do {
        if (_tempPkt) {
            ret = [self decodeVideo:_tempPkt->data
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
                _eof = true;
                NSLog(@"andy av_read_frame() failed: %s", av_err2str(ret));
                NSLog(@"andy 总耗时: %f 总次数: %d 平均耗时: %f", _totalTime * 1000, _count, (_totalTime * 1.0 / _count) * 1000);
                return ret;
            }
            if (pkt.data && pkt.stream_index == _videoIndex) {
                av_packet_rescale_ts(&pkt, _formatCtx->streams[_videoIndex]->time_base, {1, 1000});
                ret = [self decodeVideo:pkt.data
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
    } while (ret != AVERROR_EOF);
    
    return 0;
}

- (CVPixelBufferRef)seekToTime:(long)clock {
    if (_seekReq) {
        return nullptr;
    }
    
    _seekReq = true;
    
    if (clock == _lastClock) {
        if (_lastImage) {
            CMSampleBufferRef sampleBuffer = nullptr;
            CMSampleBufferCreateCopy(kCFAllocatorDefault, _lastImage->sampleBuffer, &sampleBuffer);
            _seekReq = false;
            return CMSampleBufferGetImageBuffer(sampleBuffer);
        }
    }
    
    if (clock < _lastClock) {
        int64_t time = av_rescale(clock, AV_TIME_BASE, 1000);
        int ret = avformat_seek_file(_formatCtx.get(), -1, INT64_MIN, time, INT64_MAX, 0);
        if (ret < 0) {
            return nullptr;
        }
    }
    
    _lastClock = clock;
    
    for (;;) {
        if (_imageList.empty()) {
            int ret;
            if (_tempPkt) {
                ret = [self decodeVideo:_tempPkt->data
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
                    ret = [self decodeVideo:pkt.data
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
        } else {
            auto image = std::move(_imageList.front());
            _imageList.pop_front();
            if (image->pts <= clock && clock <= image->pts + image->duration) {
                _lastImage = image;
                CMSampleBufferRef sampleBuffer = nullptr;
                CMSampleBufferCreateCopy(kCFAllocatorDefault, _lastImage->sampleBuffer, &sampleBuffer);
                _seekReq = false;
                return CMSampleBufferGetImageBuffer(sampleBuffer);
            }
        }
    }
    
    return nil;
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
    
    if (_ready) {
        if (!_sorted) {
            _imageList.sort([](std::shared_ptr<XImage> prev, std::shared_ptr<XImage> next) {
                return prev->pts < next->pts;
            });
            _sorted = true;
#if PRINT_PLAY_LOG
            NSLog(@"andy -- image count: %lu --", _imageList.size());
#endif
        }
        
        auto image = std::move(_imageList.front());
        _imageList.pop_front();
        _lastImage = image;
        
        if (_imageList.empty()) {
            _ready = false;
            _sorted = false;
        }
        
        CMSampleBufferRef sampleBuffer = nullptr;
        CMSampleBufferCreateCopy(kCFAllocatorDefault, _lastImage->sampleBuffer, &sampleBuffer);
#if PRINT_PLAY_LOG
        NSLog(@"andy pts: %ld", image->pts);
#endif
        return CMSampleBufferGetImageBuffer(sampleBuffer);
    }
    
    if (!_ready) {
        if (_tempImage) {
            _imageList.emplace_back(std::move(_tempImage));
        } else {
            int ret;
            if (_tempPkt) {
                ret = [self decodeVideo:_tempPkt->data
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
                    ret = [self decodeVideo:pkt.data
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
    _duration = static_cast<long>(av_rescale_q(st->duration, st->time_base, {1, 1000}));
    AVCodecParameters* codecpar = st->codecpar;
    
    int vpsIndex = 0;
    int spsIndex = 0;
    int ppsIndex = 0;
    size_t vpsLength = 0;
    size_t spsLength = 0;
    size_t ppsLength = 0;
    
    if (codecpar->codec_id == AV_CODEC_ID_H264) {
        spsIndex = 8;
        spsLength = static_cast<size_t>(codecpar->extradata[spsIndex - 2] + codecpar->extradata[spsIndex - 1]);

        ppsIndex = static_cast<int>(spsIndex + spsLength + 2 + 1);
        ppsLength = static_cast<size_t>(codecpar->extradata[ppsIndex - 2] + codecpar->extradata[ppsIndex - 1]);
    } else if (codecpar->codec_id == AV_CODEC_ID_HEVC) {
        int i = 23;
        for (;;) {
            if (vpsIndex > 0 && spsIndex > 0 && ppsIndex > 0) {
                break;
            }
            switch (codecpar->extradata[i] & 0x3F) {
                case 32: {
                    vpsIndex = i + 5;
                    vpsLength = static_cast<size_t>(codecpar->extradata[vpsIndex - 2] + codecpar->extradata[vpsIndex - 1]);
                    i = static_cast<int>(vpsIndex + vpsLength);
                } break;
                case 33: {
                    spsIndex = i + 5;
                    spsLength = static_cast<size_t>(codecpar->extradata[spsIndex - 2] + codecpar->extradata[spsIndex - 1]);
                    i = static_cast<int>(spsIndex + spsLength);
                } break;
                case 34: {
                    ppsIndex = i + 5;
                    ppsLength = static_cast<size_t>(codecpar->extradata[ppsIndex - 2] + codecpar->extradata[ppsIndex - 1]);
                    i = static_cast<int>(ppsIndex + ppsLength);
                } break;
                default:
                    break;
            }
        }
    }
    
    uint8_t* sps = new uint8_t[spsLength];
    memcpy(sps, &codecpar->extradata[spsIndex], spsLength);
    uint8_t* pps = new uint8_t[ppsLength];
    memcpy(pps, &codecpar->extradata[ppsIndex], ppsLength);
    
    if (vpsLength > 0) {
        uint8_t* vps = new uint8_t[vpsLength];
        memcpy(vps, &codecpar->extradata[vpsIndex], vpsLength);
        [self setupHEVCVideoFormatDescription:vps vpsLength:vpsLength sps:sps spsLength:spsLength pps:pps ppsLength:ppsLength];
    } else {
        [self setupH264VideoFormatDescription:sps spsLength:spsLength pps:pps ppsLength:ppsLength];
    }
    
    [self createVTDecompressionSession];
    
    return 0;
}

- (int)setupH264VideoFormatDescription:(uint8_t*)sps
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

- (int)setupHEVCVideoFormatDescription:(uint8_t*)vps
                             vpsLength:(size_t)vpsLength
                                   sps:(uint8_t*)sps
                             spsLength:(size_t)spsLength
                               pps:(uint8_t*)pps
                         ppsLength:(size_t)ppsLength {
    OSStatus status = 0;
    size_t parameterSetCount = 3;
    const uint8_t* parameterSetPointers[3] = {vps, sps, pps};
    size_t parameterSetSizes[3] = {vpsLength, spsLength, ppsLength};
    int NALUnitHeaderLength = 4;
    
    if (@available(iOS 11.0, *)) {
        status = CMVideoFormatDescriptionCreateFromHEVCParameterSets(kCFAllocatorDefault,
                                                                     parameterSetCount,
                                                                     parameterSetPointers,
                                                                     parameterSetSizes,
                                                                     NALUnitHeaderLength,
                                                                     nullptr,
                                                                     &videoFormatDescription);
    } else {
        // Fallback on earlier versions
    }
    if (status != noErr) {
        err2string(status);
        return status;
    }
    
    return 0;
}

- (int)createVTDecompressionSession {
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

- (void)destroyVTDecompressionSession {
    if (decompressionSession != nil) {
        VTDecompressionSessionInvalidate(decompressionSession);
        CFRelease(decompressionSession);
    }
}

- (int)resetVTDecompressionSession {
    [self destroyVTDecompressionSession];
    return [self createVTDecompressionSession];
}

- (long)getDuration {
    return _duration;
}


#pragma mark -- Private

- (int)decodeVideo:(uint8_t*)data size:(size_t)size pts:(double)pts dts:(double)dts duration:(double)duration {
    
//    {
//        std::lock_guard<std::mutex> lock(_mutex);
//        if (_imageList.size() >= _reorderSize) {
//            return -11;
//        }
//    }

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
//        NSLog(@"andy pushback pts: %ld", image->pts);
        _imageList.emplace_back(std::move(image));
//        if (_imageList.empty()) {
//            _imageList.emplace_back(std::move(image));
//            _prevMaxClock = static_cast<long>(CMTimeGetSeconds(pts));
//        } else {
//            if (image->pts > _prevMaxClock) {
//                _ready = true;
//                _tempImage = image;
//                _prevMaxClock = image->pts;
//            } else {
//                _imageList.emplace_back(std::move(image));
//            }
//        }
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
            
        case kVTVideoDecoderBadDataErr:
            NSLog(@"Video Decoder Bad Data");
            break;
            
        case kVTInvalidSessionErr:
            NSLog(@"Invalid VTDecompression Session");
            break;
            
        default:
            NSLog(@"Unkown Error");
            break;
    }
}
