//
//  XFFProducer.cpp
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#include "XFFProducer.hpp"
#include "XException.hpp"
#include "XThreadUtils.hpp"
#include "XPacketQueue.hpp"
#include "XFrameQueue.hpp"
#include "XImageQueue.hpp"
#include "XPlatform.hpp"
#include "XTimeCounter.hpp"
#include "XLogger.hpp"
#include "XSoxHelper.hpp"

#include "libyuv.h"

#ifdef USE_HARDWARE_DECODER
AVPixelFormat gHWPixelFormat = AV_PIX_FMT_NONE;
#endif

FILE* fp = nullptr;

XFFProducer::XFFProducer()
        : mVideoIndex(-1), mAudioIndex(-1), mAbortReq(false), mSeekReq(false), mSeekTargetPos(-1),
          mLastReqClock(LONG_MAX), mLastFrameClock(LONG_MAX), mAVFormatSeeked(false), mPauseReq(false), mStatus(0), mSampleData(nullptr),
          mSampleDataIndex(0), mSampleBufferSize(0), mSampleBufferSizeMax(0), mBFrameIndex(0) {
}

XFFProducer::~XFFProducer() {
    if (!mAbortReq) {
        stop();
    }
}

void XFFProducer::setInput(const std::string &filename) {
    XProducable::setInput(filename);
    int ret = openInFile();
    if (ret < 0) {
        throw XException(av_err2str(ret));
    }
    closeInFile();
}

void XFFProducer::start() {

    if (!mFormatCtx) {
        int ret = openInFile();
        if (ret < 0) {
            return;
        }
    }

    if (!mReadTid) {
        mReadTid = std::make_unique<std::thread>([this] { readWorkThread(this); });
    }
}

void XFFProducer::seekTo(long targetPos) {

    mPauseReq = true;
    if (mVideoPacketQueue) {
        mVideoPacketQueue->flush();
    }
    if (mAudioPacketQueue) {
        mAudioPacketQueue->flush();
    }

    if (mImageQueue) {
        mImageQueue->flush();
    }

    mStatus &= ~(S_READ_END | S_VIDEO_END | S_AUDIO_END);
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mContinueReadCond.notify_one();
        mContinueVideoCond.notify_one();
        mContinueAudioCond.notify_one();
    }

    mAVFormatSeeked = false;
    mSeekReq = true;
    mSeekTargetPos = targetPos;

    mPauseReq = false;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mContinueReadCond.notify_one();
        mContinueVideoCond.notify_one();
        mContinueAudioCond.notify_one();
    }

}

int XFFProducer::seekFileTo(long targetPos) {
    int ret = -1;
    if (mFormatCtx && mVideoIndex >= 0) {
        int64_t target = av_rescale(static_cast<int64_t>(targetPos), AV_TIME_BASE, 1000);
        ret = avformat_seek_file(mFormatCtx.get(), -1, INT64_MIN, target, INT64_MAX, 0);
        if (ret < 0) {
            LOGE("[GDFrameRetriever] avformat_seek_file(target: %ld) failed: %s\n", targetPos, av_err2str(ret));
        }
    }
    return ret;
}

std::shared_ptr<XImage> XFFProducer::peekImage(long clock) {
    if (!mImageQueue) {
        return nullptr;
    }

    if (mLastReqClock > clock || (mLastReqClock == LONG_MAX && clock > 0)) {
//        seekTo(clock);
    }
    mLastReqClock = clock;

    for (;;) {
        auto image = mImageQueue->peekReadable();
        if (mSeekReq) {
            mSeekReq = false;
        }
        if (image->pts > clock) {
            return nullptr;
        } else if (image->pts <= clock && clock <= (image->pts + image->duration)) {
            return image;
        } else {
            mImageQueue->next();
            continue;
        }
    }

    return nullptr;
}

void XFFProducer::endCurrentImageUse() {
    if (!mImageQueue) {
        return;
    }

    mImageQueue->next();
}

int XFFProducer::readSamples(uint8_t* buffer, int length) {
    if (!mSampleQueue) {
        return 0;
    }
    
    if (mStatus & S_AUDIO_END && mSampleQueue->used() <= 0) {
        return -1;
    }
    
    int len = mSampleQueue->read(buffer, length);
    
    return len;
}

void XFFProducer::stop() {
    XProducable::stop();

    {
        std::lock_guard<std::mutex> lock(mMutex);
        mAbortReq = true;
        mContinueReadCond.notify_one();
    }

    if (mReadTid && mReadTid->joinable()) {
        mReadTid->join();
    }
}

int XFFProducer::openInFile() {
    AVFormatContext *ic = avformat_alloc_context();
    if (!ic) {
        LOGE("[XFFProducer] avformat_alloc_context failed!\n");
        return AVERROR(ENOMEM);
    }
    mFormatCtx = std::unique_ptr<AVFormatContext, InputFormatDeleter>(ic);

    int ret = avformat_open_input(&ic, mFilename.data(), nullptr, nullptr);
    if (ret < 0) {
        LOGE("[XFFProducer] avformat_open_input failed: %s\n",
             av_err2str(ret));
        return ret;
    }

    ret = avformat_find_stream_info(ic, nullptr);
    if (ret < 0) {
        LOGE("[XFFProducer] avformat_find_stream_info failed: %s\n",
             av_err2str(ret));
        return ret;
    }

    if (!mDisableVideo) {
        int videoIndex = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (videoIndex >= 0) {
            mVideoIndex = videoIndex;
            ret = openVideoCodec();
            if (ret < 0) {
                return ret;
            }
            
            findKeyTimestamps();
            
            // TODO(oogh): 2020/09/17 #isValidPacket() 接口中需要使用到 mVideoCodecCtx 变量，这里关闭的话，会偶现出现崩溃问题
            // closeVideoCodec();
        }
    }

    if (!mDisableAudio) {
        int audioIndex = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (audioIndex >= 0) {
            mAudioIndex = audioIndex;
            ret = openAudioCodec();
            if (ret < 0) {
                return ret;
            }
            closeAudioCodec();
        }
    }

    return 0;
}

int XFFProducer::openVideoCodec() {
    if (mDisableVideo || mVideoIndex < 0 || !mFormatCtx) {
        return -1;
    }

    AVStream *stream = mFormatCtx->streams[mVideoIndex];
    AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);

#if PLATFORM_ANDROID && defined(USE_HARDWARE_DECODER)
    if (mProduceMode == PRODUCE_MODE_HARDWARE) {
        if (codec->id == AV_CODEC_ID_H264) {
            codec = avcodec_find_decoder_by_name("h264_mediacodec");
        } else if (codec->id == AV_CODEC_ID_HEVC) {
            codec = avcodec_find_decoder_by_name("hevc_mediacodec");
        }
    }
#endif

    if (!codec) {
        codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec) {
            LOGE("[XFFProducer] avcodec_find_decoder failed: cannot find decoder %s\n",
                 avcodec_get_name(codec->id));
            return AVERROR_DECODER_NOT_FOUND;
        }
    }

#ifdef USE_HARDWARE_DECODER
    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    if (mProduceMode == PRODUCE_MODE_HARDWARE) {
#if PLATFORM_ANDROID
        type = av_hwdevice_find_type_by_name("mediacodec");
#elif PLATFORM_IOS || PLATFORM_MAC
        type = av_hwdevice_find_type_by_name("videotoolbox");
#endif
        if (type == AV_HWDEVICE_TYPE_NONE) {
            LOGW("[XFFProducer] Available device types: ");
            while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
                LOGW(" %s", av_hwdevice_get_type_name(type));
            }
            LOGW("\n");
            return -1;
        }

        for (int i = 0;; ++i) {
            const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
            if (!config) {
                LOGE("Decoder %s does not support device type %s.\n", codec->name,
                     av_hwdevice_get_type_name(type));
                return -1;
            }
            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                config->device_type == type) {
                gHWPixelFormat = config->pix_fmt;
                break;
            }
        }
    }
#endif

    AVCodecContext *avctx = avcodec_alloc_context3(codec);
    if (!avctx) {
        LOGE("[XFFProducer] avcodec_alloc_context3 failed!\n");
        return AVERROR(ENOMEM);
    }
    mVideoCodecCtx = std::unique_ptr<AVCodecContext, CodecDeleter>(avctx);

    int ret = avcodec_parameters_to_context(avctx, stream->codecpar);
    if (ret < 0) {
        LOGE("[XFFProducer] avcodec_parameters_to_context failed: %s\n",
             av_err2str(ret));
        return ret;
    }

#ifdef USE_HARDWARE_DECODER
    if (mProduceMode == PRODUCE_MODE_HARDWARE) {
        avctx->get_format = [](AVCodecContext *ctx,
                               const enum AVPixelFormat *pixelFormat) -> AVPixelFormat {
            const enum AVPixelFormat *p;
            for (p = pixelFormat; *p != AV_PIX_FMT_NONE; p++) {
                if (*p == gHWPixelFormat) {
                    return *p;
                }
            }
            LOGE("[XFFProducer] Cannot get HW surface format\n");
            return AV_PIX_FMT_NONE;
        };

        AVBufferRef *deviceCtx = nullptr;
        ret = av_hwdevice_ctx_create(&deviceCtx, type, nullptr, nullptr, 0);
        if (ret < 0) {
            LOGE("[XFFProducer] av_hwdevice_ctx_create failed: %s\n", av_err2str(ret));
            return ret;
        }
        // TODO(oogh): 2020/07/01 需要通过 av_buffer_unref 释放
        avctx->hw_device_ctx = av_buffer_ref(deviceCtx);
    }
#endif

    ret = avcodec_open2(avctx, nullptr, nullptr);
    if (ret < 0) {
        LOGE("[XFFProducer] avcodec_open2 failed: %s\n", av_err2str(ret));
        return ret;
    }
    return 0;
}

int XFFProducer::openAudioCodec() {
    if (mDisableAudio || mAudioIndex < 0 || !mFormatCtx) {
        return -1;
    }

    AVCodecContext *avctx = avcodec_alloc_context3(nullptr);
    if (!avctx) {
        LOGE("[XFFProducer] avcodec_alloc_context3 failed!\n");
        return AVERROR(ENOMEM);
    }
    mAudioCodecCtx = std::unique_ptr<AVCodecContext, CodecDeleter>(avctx);

    AVStream *stream = mFormatCtx->streams[mAudioIndex];
    int ret = avcodec_parameters_to_context(avctx, stream->codecpar);
    if (ret < 0) {
        LOGE("[XFFProducer] avcodec_parameters_to_context failed: %s\n",
             av_err2str(ret));
        return ret;
    }

    AVCodec *codec = avcodec_find_decoder(avctx->codec_id);
    if (!codec) {
        LOGE("[XFFProducer] avcodec_find_decoder failed: cannot find decoder %s\n",
             avcodec_get_name(avctx->codec_id));
        return AVERROR_DECODER_NOT_FOUND;
    }

    ret = avcodec_open2(avctx, codec, nullptr);
    if (ret < 0) {
        LOGE("[XFFProducer] avcodec_open2 failed: %s\n", av_err2str(ret));
        return ret;
    }
    
    if (mSwrContext) {
        mSwrContext.reset();
    }

    // 重采样环境配置
    if (!mSwrContext) {
        SwrContext* swr = swr_alloc();
        if (!swr) {
            return AVERROR(ENOMEM);
        }
        mSwrContext = std::unique_ptr<SwrContext,SwrContextDeleter>(swr);
    }
    
    uint64_t channelLayout = (avctx->channel_layout && avctx->channels == av_get_channel_layout_nb_channels(avctx->channel_layout)) ? avctx->channel_layout : av_get_default_channel_layout(avctx->channels);

    av_opt_set_int(mSwrContext.get(), "in_channel_layout", channelLayout, 0);
    av_opt_set_int(mSwrContext.get(), "in_sample_rate", avctx->sample_rate, 0);
    av_opt_set_sample_fmt(mSwrContext.get(), "in_sample_fmt", avctx->sample_fmt, 0);

    av_opt_set_int(mSwrContext.get(), "out_channel_layout", DST_CHANNEL_LAYOUT, 0);
    av_opt_set_int(mSwrContext.get(), "out_sample_rate", DST_SAMPLE_RATE, 0);
    av_opt_set_sample_fmt(mSwrContext.get(), "out_sample_fmt", DST_SAMPLE_FMT, 0);

    ret = swr_init(mSwrContext.get());
    if (ret < 0) {
        LOGE("[XFFProducer] swr_init failed: %s\n", av_err2str(ret));
        return ret;
    }
    
    mDstSampleCountMax = 0;
    if (mSampleData) {
        av_freep(&mSampleData);
    }

    // 申请目标缓冲区内存空间
    int srcSampleCount = avctx->frame_size > 0 ? avctx->frame_size : 1024;
    int dstSampleCount = static_cast<int>(av_rescale_rnd(srcSampleCount, DST_SAMPLE_RATE, avctx->sample_rate, AV_ROUND_UP));
    mDstSampleCountMax = dstSampleCount;

    int dstChannels = av_get_channel_layout_nb_channels(DST_CHANNEL_LAYOUT);
    ret = av_samples_alloc(&mSampleData, nullptr, dstChannels, dstSampleCount, DST_SAMPLE_FMT, 0);
    if (ret < 0) {
        LOGE("[XFFProducer] av_samples_alloc_array_and_samples failed: %s\n", av_err2str(ret));
        return AVERROR(ENOMEM);
    }

    return 0;
}

void XFFProducer::closeVideoCodec() {
    if (mVideoCodecCtx) {
        mVideoCodecCtx.reset();
    }
}

void XFFProducer::closeAudioCodec() {
    if (mAudioCodecCtx) {
        mAudioCodecCtx.reset();
    }
}

void XFFProducer::closeInFile() {
    closeVideoCodec();
    closeAudioCodec();
    if (mFormatCtx) {
        mFormatCtx.reset();
    }
}

void XFFProducer::readWorkThread(void *opaque) {
    XThreadUtils::configThreadName("readWorkThread");
    LOGI("[XFFProducer] readWorkThread ++++\n");
    XFFProducer *producer = reinterpret_cast<XFFProducer *>(opaque);
    if (!producer || !mFormatCtx) {
        return;
    }

    AVFormatContext *ic = producer->mFormatCtx.get();
    XPacketQueue *videoQ = nullptr;
    XPacketQueue *audioQ = nullptr;

    // video work thread
    if (!producer->mDisableVideo && producer->mVideoIndex >= 0) {
        if (!producer->mVideoPacketQueue) {
            producer->mVideoPacketQueue = std::make_unique<XPacketQueue>();
            videoQ = producer->mVideoPacketQueue.get();
        }

        if (!producer->mVideoTid) {
            producer->mVideoTid = std::make_unique<std::thread>(
                    [this, producer] { videoWorkThread(producer); });
        }
    }

    // audio work thread
    if (!producer->mDisableAudio && producer->mAudioIndex >= 0) {
        if (!producer->mAudioPacketQueue) {
            producer->mAudioPacketQueue = std::make_unique<XPacketQueue>();
            audioQ = producer->mAudioPacketQueue.get();
        }

        if (!producer->mAudioTid) {
            producer->mAudioTid = std::make_unique<std::thread>(
                    [this, producer] { audioWorkThread(producer); });
        }
    }

    int ret;
    long pts = 0;
    long duration = 0;
    for (;;) {
        if (producer->mAbortReq) {
            break;
        }

        if (producer->mPauseReq) {
            std::unique_lock<std::mutex> lock(mMutex);
            producer->mContinueReadCond.wait(lock);
            continue;
        }

        if (producer->mSeekReq && !producer->mAVFormatSeeked) {
            long ts = static_cast<long>(av_rescale(producer->mSeekTargetPos, AV_TIME_BASE, 1000));
            ret = avformat_seek_file(ic, -1, INT64_MIN, ts, INT64_MAX, 0);
            if (ret < 0) {
                LOGE("[XFFProducer] avformat_seek_file failed: %s\n", av_err2str(ret));
                break;
            }
            producer->mAVFormatSeeked = true;
        }

        auto pkt = std::make_shared<Packet>();
        ret = av_read_frame(ic, pkt->avpkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF && (producer->mStatus & S_READ_END) != S_READ_END) {
                if (videoQ) {
                    videoQ->putNullPacket(producer->mVideoIndex);
                }
                if (audioQ) {
                    audioQ->putNullPacket(producer->mAudioIndex);
                }
                producer->mStatus |= S_READ_END;
            }
            std::unique_lock<std::mutex> lock(producer->mMutex);
            mContinueReadCond.wait(lock);
            continue;
        }

        pts = static_cast<long>(pkt->avpkt->pts *
                                av_q2d(ic->streams[pkt->avpkt->stream_index]->time_base) * 1000);
        duration = static_cast<long>(pkt->avpkt->duration *
                                     av_q2d(ic->streams[pkt->avpkt->stream_index]->time_base) *
                                     1000);
        if (pkt->avpkt->stream_index == producer->mVideoIndex) {
            if (videoQ) {
                videoQ->put(pkt);
            }
        } else if (pkt->avpkt->stream_index == producer->mAudioIndex) {
            if (audioQ) {
                audioQ->put(pkt);
            }
        }
    }

    if (producer->mVideoTid && producer->mVideoTid->joinable()) {
        producer->mVideoTid->join();
    }

    if (producer->mAudioTid && producer->mAudioTid->joinable()) {
        producer->mAudioTid->join();
    }

    LOGI("[XFFProducer] readWorkThread ----\n");
}

void XFFProducer::videoWorkThread(void *opaque) {
    XThreadUtils::configThreadName("videoWorkThread");
    LOGI("[XFFProducer] videoWorkThread ++++\n");
    XFFProducer *producer = reinterpret_cast<XFFProducer *>(opaque);
    if (!producer) {
        return;
    }

    if (!producer->mVideoCodecCtx) {
        int ret = producer->openVideoCodec();
        if (ret < 0) {
            return;
        }
    }

    if (!producer->mImageQueue) {
        producer->mImageQueue = std::make_unique<XImageQueue>();
    }

    int ret;
    for (;;) {
        if (producer->mAbortReq) {
            break;
        }

        if (producer->mPauseReq) {
            std::unique_lock<std::mutex> lock(producer->mMutex);
            producer->mContinueVideoCond.wait(lock);
            continue;
        }

        ret = producer->decodeVideoFrame();
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                if ((producer->mStatus & S_READ_END) && !(producer->mStatus & S_VIDEO_END)) {
                    producer->mStatus |= S_VIDEO_END;
                }
                std::unique_lock<std::mutex> lock(producer->mMutex);
                mContinueVideoCond.wait(lock);
                continue;
            }
            break;
        }
    }

    LOGI("[XFFProducer] videoWorkThread ----\n");
}

void XFFProducer::audioWorkThread(void *opaque) {
    XThreadUtils::configThreadName("audioWorkThread");
    LOGI("[XFFProducer] audioWorkThread ++++\n");
    XFFProducer *producer = reinterpret_cast<XFFProducer *>(opaque);
    if (!producer) {
        return;
    }
    
    if (!producer->mAudioCodecCtx) {
        int ret = producer->openAudioCodec();
        if (ret < 0) {
            return;
        }
    }

    if (!producer->mSampleQueue) {
        producer->mSampleQueue = std::make_unique<XSampleQueue>(SAMPLE_QUEUE_SIZE);
    }

    int ret, available, writeSize, min;
    for (;;) {
        if (producer->mAbortReq) {
            break;
        }

        if (producer->mPauseReq) {
            std::unique_lock<std::mutex> lock(producer->mMutex);
            producer->mContinueAudioCond.wait(lock);
            continue;
        }

        // 判断SampleQueue中剩余的可用内存空间大小
        available = producer->mSampleQueue->available();
        if (available <= 0) {
            continue;
        } else {
            if (producer->mSampleBufferSize > 0) {
                min = std::min(producer->mSampleBufferSize, available);
                writeSize = producer->mSampleQueue->write(producer->mSampleData, min);
                producer->mSampleData += writeSize;
                producer->mSampleDataIndex += writeSize;
                producer->mSampleBufferSize -= writeSize;
            } else {
                // 当且仅当SampleQueue中没有任何数据的时候，才去进行解码操作
                if (producer->mSampleDataIndex >= producer->mSampleBufferSizeMax) {
                    producer->mSampleData -= producer->mSampleBufferSizeMax;
                    producer->mSampleDataIndex = 0;
                }
                ret = producer->decodeAudioFrame();
                if (ret < 0) {
                    if (ret == AVERROR_EOF) {
                        if (fp) {
                            fclose(fp);
                        }
                        if ((producer->mStatus & S_READ_END) && !(producer->mStatus & S_AUDIO_END)) {
                            producer->mStatus |= S_AUDIO_END;
                            producer->mSampleQueue->signal();
                        }
                        std::unique_lock<std::mutex> lock(producer->mMutex);
                        producer->mContinueAudioCond.wait(lock);
                        continue;
                    }
                    break;
                }
            }
        }
    }

    LOGI("[XFFProducer] audioWorkThread ----\n");
}

int XFFProducer::decodeVideoFrame() {
    
    if (!mFormatCtx || mVideoIndex < 0) {
        return -1;
    }
    
    int ret;

    if (!mVideoCodecCtx) {
        ret = openVideoCodec();
        if (ret < 0) {
            return ret;
        }
    }

    ret = AVERROR(EAGAIN);
    long pts = -1;
    long duration = -1;
    for (;;) {

        // receive frame
        do {
            auto frame = std::make_shared<Frame>();
            ret = avcodec_receive_frame(mVideoCodecCtx.get(), frame->avframe);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                return ret;
            }

            if (ret == AVERROR_EOF) {
                avcodec_flush_buffers(mVideoCodecCtx.get());
                return ret;
            }

            if (ret >= 0) {
                AVStream *stream = mFormatCtx->streams[mVideoIndex];
                pts = static_cast<long>(frame->avframe->pts * av_q2d(stream->time_base) * 1000);
                duration = static_cast<long>(frame->avframe->pkt_duration *
                                             av_q2d(stream->time_base) * 1000);
                if (mSeekReq) {
                    if (!(pts <= mSeekTargetPos && mSeekTargetPos < pts + duration)) {
                        continue;
                    }
                }
#ifdef USE_HARDWARE_DECODER
                if (frame->avframe->format == gHWPixelFormat) {
                    auto cpuFrame = std::make_shared<Frame>();
                    ret = av_hwframe_transfer_data(cpuFrame->avframe, frame->avframe, 0);
                    if (ret < 0) {
                        LOGE("[XFFProducer] av_hwframe_transfer_data failed: %s\n",
                             av_err2str(ret));
                        return ret;
                    }
                    queueFrame(cpuFrame->avframe, pts, duration);
                } else {
#endif
                    queueFrame(frame->avframe, pts, duration);
#ifdef USE_HARDWARE_DECODER
                }
#endif
                return 1;
            }

            if (ret < 0 && ret != AVERROR(EAGAIN)) {
                LOGE("[XFFProducer] avcodec_receive_frame failed: %s\n", av_err2str(ret));
                return ret;
            }

        } while (ret != AVERROR(EAGAIN));

        // get packet
        std::shared_ptr<Packet> pkt;
        if (mLastPacket) {
            /* mLastPacket 是针对FFmpeg4.x版本的MediaCodec解码添加的逻辑。
             * 使用MediaCodec解码时，如果 avcodec_send_packet() 返回 AVERROR(EAGAIN)，则需要把这个
             * AVPacket给缓存起来，否则该AVPacket可能会被丢弃，从而因为丢帧导致花屏等异常问题
             */
            if (mLastPacket->avpkt->data != nullptr && mLastPacket->avpkt->size != 0) {
                pkt = std::make_shared<Packet>();
                ret = av_packet_ref(pkt->avpkt, mLastPacket->avpkt);
                if (ret < 0) {
                    LOGE("[XFFProducer] av_packet_ref failed: %s\n", av_err2str(ret));
                    break;
                }
            }
        } else {
            pkt = mVideoPacketQueue->get();
        }

        if (!pkt) {
            return -1;
        }

        // send packet
        ret = avcodec_send_packet(mVideoCodecCtx.get(), pkt->avpkt);
        if (ret == AVERROR(EAGAIN)) {
            if (pkt->avpkt->data != nullptr && pkt->avpkt->size != 0) {
                if (!mLastPacket) {
                    mLastPacket = std::make_shared<Packet>();
                }
                ret = av_packet_ref(mLastPacket->avpkt, pkt->avpkt);
                if (ret < 0) {
                    LOGE("[XFFProducer] av_packet_ref failed: %s\n", av_err2str(ret));
                    break;
                }
            }
            continue;
        }

        if (ret >= 0) {
            if (mLastPacket) {
                mLastPacket.reset();
            }
        }
    }
    return 0;
}

int XFFProducer::decodeAudioFrame() {
    if (!mFormatCtx || mAudioIndex < 0) {
        return -1;
    }
    
    int ret;
    
    if (!mAudioCodecCtx) {
        ret = openAudioCodec();
        if (ret < 0) {
            return ret;
        }
    }
    
    for (;;) {
        
        auto frame = std::make_shared<Frame>();
        ret = avcodec_receive_frame(mAudioCodecCtx.get(), frame->avframe);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            LOGE("[XFFProducer] avcodec_receive_frame failed: %s\n", av_err2str(ret));
            return ret;
        }
        
        if (ret == AVERROR_EOF) {
            return ret;
        }
        
        if (ret >= 0) {
            int len = sampleConvert(frame->avframe);
            return len;
        }
        
        auto pkt = mAudioPacketQueue->get();
        if (!pkt) {
            return -1;
        }
        
        ret = avcodec_send_packet(mAudioCodecCtx.get(), pkt->avpkt);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            LOGE("[XFFProducer] avcodec_send_packet failed: %s\n", av_err2str(ret));
            return ret;
        }
    }
    
    return 0;
}

void XFFProducer::queueFrame(AVFrame *frame, long pts, long duration) {

    auto image = mImageQueue->peekWritable();
    if (!image) {
        return;
    }

    image->width = frame->width;
    image->height = frame->height;
    image->pts = pts;
    image->duration = duration;
    image->format = DST_PIX_FMT;
    frameConvert(image, frame);

    mImageQueue->push();
}

void XFFProducer::frameConvert(std::shared_ptr<XImage> dst, AVFrame *src) {
    if (!dst->pixels[0]) {
        av_image_alloc(dst->pixels, dst->linesize, dst->width, dst->height,
                       static_cast<AVPixelFormat>(dst->format), 1);
    }
    switch (src->format) {
        case AV_PIX_FMT_YUV420P:
        case AV_PIX_FMT_YUVJ420P: {
            libyuv::I420ToABGR(src->data[0], src->linesize[0],
                               src->data[1], src->linesize[1],
                               src->data[2], src->linesize[2],
                               dst->pixels[0], dst->linesize[0],
                               src->width, src->height);
        }
            break;

        case AV_PIX_FMT_NV21: {
            libyuv::NV21ToABGR(src->data[0], src->linesize[0],
                               src->data[1], src->linesize[1],
                               dst->pixels[0], dst->linesize[0],
                               src->width, src->height);
        }
            break;

        case AV_PIX_FMT_NV12: {
            libyuv::NV12ToABGR(src->data[0], src->linesize[0],
                               src->data[1], src->linesize[1],
                               dst->pixels[0], dst->linesize[0],
                               src->width, src->height);
        }
            break;

        case AV_PIX_FMT_YUV422P:
        case AV_PIX_FMT_YUVJ422P: {
            libyuv::I422ToABGR(src->data[0], src->linesize[0],
                               src->data[1], src->linesize[1],
                               src->data[2], src->linesize[2],
                               dst->pixels[0], dst->linesize[0],
                               src->width, src->height);
        }
            break;

        case AV_PIX_FMT_YUV444P:
        case AV_PIX_FMT_YUVJ444P: {
            libyuv::I444ToABGR(src->data[0], src->linesize[0],
                               src->data[1], src->linesize[1],
                               src->data[2], src->linesize[2],
                               dst->pixels[0], dst->linesize[0],
                               src->width, src->height);
        }
            break;

        default: {
            SwsContext *sws = nullptr;
            if (!mSwsContext) {
                sws = sws_getContext(src->width, src->height,
                                     static_cast<AVPixelFormat>(src->format),
                                     dst->width, dst->height, DST_PIX_FMT,
                                     SWS_FAST_BILINEAR,
                                     nullptr, nullptr, nullptr);
                if (!sws) {
                    LOGE("[XFFProducer] sws_getContext failed!\n");
                    return;
                }
                mSwsContext = std::unique_ptr<SwsContext, SwsContextDeleter>(sws);
            } else {
                sws = mSwsContext.get();
            }

            sws_scale(sws, src->data, src->linesize, 0, src->height, dst->pixels, dst->linesize);
        }
            break;
    }
}

int XFFProducer::sampleConvert(AVFrame* frame) {
    
    if (!fp) {
        fp = fopen("/Users/andy/output.pcm", "wb+");
    }
    
    if (mSampleData) {
        av_freep(&mSampleData);
    }
    
    // 1. 分配目标缓冲区内存空间
    int dstChannels = av_get_channel_layout_nb_channels(DST_CHANNEL_LAYOUT);
    int dstSampleCount = static_cast<int>(av_rescale_rnd(swr_get_delay(mSwrContext.get(), frame->sample_rate) + frame->nb_samples, DST_SAMPLE_RATE, frame->sample_rate, AV_ROUND_UP));
    mSampleBufferSize = av_samples_alloc(&mSampleData, nullptr, dstChannels, dstSampleCount, DST_SAMPLE_FMT, 1);
    if (mSampleBufferSize < 0) {
        LOGE("[XFFProducer] av_samples_alloc failed!");
        return AVERROR(ENOMEM);
    }

    // 2. 执行重采样
    int count = swr_convert(mSwrContext.get(), &mSampleData, dstSampleCount, (const uint8_t **)frame->data, frame->nb_samples);
    if (count < 0) {
        LOGE("[XFFProducer] swr_convert failed: %s\n", av_err2str(count));
        return count;
    }

    // 3. 重采样完成后，计算重采样后的数据大小
    int size = av_samples_get_buffer_size(nullptr, dstChannels, count, DST_SAMPLE_FMT, 1);
    if (size < 0) {
        LOGE("[XFFProducer] av_samples_get_buffer_size failed: %s\n", av_err2str(size));
        return size;
    }

    mSampleBufferSize = size;
    mSampleBufferSizeMax = size;

    return mSampleBufferSize;
}

bool XFFProducer::isValidPacket(AVPacket* pkt) {
    AVStream *st = mFormatCtx->streams[mVideoIndex];
    AVCodecParserContext* parser = st->parser;
    AVCodecContext* avctx = mVideoCodecCtx.get();
    AVPacket *packet = av_packet_alloc();
    av_init_packet(packet);
    av_parser_parse2(parser, avctx, &packet->data, &packet->size, pkt->data, pkt->size, pkt->pts, pkt->dts, pkt->pos);
    av_packet_free(&packet);
    if (parser->pict_type == AV_PICTURE_TYPE_B) {
        mBFrameIndex++;
        if (!(mBFrameIndex & 1)) {
            return false;
        }
    } else if (parser->pict_type == AV_PICTURE_TYPE_I || parser->pict_type == AV_PICTURE_TYPE_P) {
        mBFrameIndex = 0;
    }
    return true;
}

bool XFFProducer::checkIfNeedSeek(long clock) {
    
    if (clock < mLastFrameClock) {
        return true;
    }
    
    auto clockIter = std::lower_bound(mKeyTimestamps.begin(), mKeyTimestamps.end(), clock);
    auto lastClockIter = std::lower_bound(mKeyTimestamps.begin(), mKeyTimestamps.end(), mLastFrameClock);
    if (clockIter != lastClockIter) {
        return true;
    }

    return false;
}

long XFFProducer::getOriginalDuration() const {
    long duration = 0;
    if (mFormatCtx && mFormatCtx->duration != AV_NOPTS_VALUE) {
        duration = static_cast<long>(
                (mFormatCtx->duration + (mFormatCtx->duration <= INT64_MAX - 5000 ? 5000 : 0)) /
                1000);
    }
    return duration;
}

int XFFProducer::getOriginalWidth() const {
    int width = 0;
    if (mFormatCtx) {
        int index = av_find_best_stream(mFormatCtx.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (index >= 0) {
            width = mFormatCtx->streams[index]->codecpar->width;
        }
    }
    return width;
}

int XFFProducer::getOriginalHeight() const {
    int height = 0;
    if (mFormatCtx) {
        int index = av_find_best_stream(mFormatCtx.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (index >= 0) {
            height = mFormatCtx->streams[index]->codecpar->height;
        }
    }
    return height;
}

int XFFProducer::getOriginalRotation() const {
    if (!mFormatCtx || mVideoIndex < 0) {
        return -1;
    }

    uint8_t *displayMatrix = av_stream_get_side_data(mFormatCtx->streams[mVideoIndex], AV_PKT_DATA_DISPLAYMATRIX, nullptr);
    double theta = 0;
    if (displayMatrix) {
        theta = av_display_rotation_get(reinterpret_cast<const int32_t *>(displayMatrix));
    }

    theta -= 360 * floor(theta / 360 + 0.9 / 360);

    if (theta < 0) {
        theta += 360;
    }

    return static_cast<int>(theta);
}

int XFFProducer::getOriginalFrameRate() const {
    if (!mFormatCtx || mVideoIndex < 0) {
        return -1;
    }

    AVRational frameRate = mFormatCtx->streams[mVideoIndex]->avg_frame_rate;
    
    return frameRate.num / frameRate.den;
}

void XFFProducer::findKeyTimestamps() {
    if (!mFormatCtx || mVideoIndex < 0) {
        return;
    }

    int ret = seekFileTo(0);
    if (ret < 0) {
        return;
    }

    AVStream *stream = mFormatCtx->streams[mVideoIndex];
    /* 获取关键帧不能使用 AVDISCARD_NOKEY
       因为有部分特殊素材，它们内部的pts异常，会导致筛选到的并不是关键帧，同一个素材不同discard得到的数据如下：
       使用 NOKEY 得到的关键帧是 [0, 1962, 4096 ...](错误情况，可通过 avformat_seek_file() 接口验证)
       使用 BIDIR 得到的关键帧是 [0, 2020, 4096 ...](正确)
       总结：获取关键帧时间列表可通过 AVDISCARD_BIDIR 和 AV_PKT_FLAG_KEY 来筛选 */
    stream->discard = AVDISCARD_BIDIR;

    while (ret != AVERROR_EOF) {
        auto pkt = std::make_shared<Packet>();
        ret = av_read_frame(mFormatCtx.get(), pkt->avpkt);
        if (ret >= 0 && pkt->avpkt->stream_index == mVideoIndex && pkt->avpkt->flags & AV_PKT_FLAG_KEY) {
            av_packet_rescale_ts(pkt->avpkt, stream->time_base, {1, 1000});
            mKeyTimestamps.push_back(static_cast<long>(pkt->avpkt->pts));
        }
    }

    stream->discard = AVDISCARD_DEFAULT;
    ret = seekFileTo(0);
    if (ret < 0) {
        return;
    }
}
