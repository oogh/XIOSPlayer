//
//  XFFmpegVideoCodec.cpp
//  XRender
//
//  Created by Oogh on 2020/9/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#include "XFFmpegVideoCodec.hpp"
#include "XLogger.hpp"
#include "XPlatform.hpp"
#include "XThreadUtils.hpp"
#include "XImageQueue.hpp"
#include "libyuv.h"

AVPixelFormat XFFmpegVideoCodec::mHWPixelFormat = AV_PIX_FMT_NONE;

XFFmpegVideoCodec::XFFmpegVideoCodec()
: mBFrameIndex(0), mLastClock(LONG_MAX) {

}

XFFmpegVideoCodec::~XFFmpegVideoCodec() {

}

int XFFmpegVideoCodec::open() {
    if (mFilename.empty()) {
        return AVERROR(EINVAL);
    }

    AVFormatContext* ic = nullptr;

    int ret = avformat_open_input(&ic, mFilename.data(), nullptr, nullptr);
    if (ret < 0) {
        LOGE("[XVideoCodec] avformat_open_input failed: %s\n", av_err2str(ret));
        return ret;
    }
    mFormatCtx = std::unique_ptr<AVFormatContext, InputFormatDeleter>(ic);

    ret = avformat_find_stream_info(ic, nullptr);
    if (ret < 0) {
        LOGE("[XVideoCodec] avformat_find_stream_info failed: %s\n", av_err2str(ret));
        return ret;
    }

    for (int i = 0; i < mFormatCtx->nb_streams; ++i) {
        mFormatCtx->streams[i]->discard = AVDISCARD_ALL;
    }

    int index = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (index < 0) {
        return AVERROR_STREAM_NOT_FOUND;
    }
    mIndex = index;
    AVStream* stream = mFormatCtx->streams[index];
    stream->discard = AVDISCARD_DEFAULT;

    AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        LOGE("[XVideoCodec] avcodec_find_decoder(%s) failed!\n", avcodec_get_name(stream->codecpar->codec_id));
        return AVERROR_DECODER_NOT_FOUND;
    }

    AVCodecContext* avctx = avcodec_alloc_context3(decoder);
    if (!avctx) {
        LOGE("[XVideoCodec] avcodec_alloc_context3 failed!\n");
        return AVERROR(ENOMEM);
    }
    mCodecCtx = std::unique_ptr<AVCodecContext, CodecDeleter>(avctx);

    ret = avcodec_parameters_to_context(avctx, stream->codecpar);
    if (ret < 0) {
        LOGE("[XVideoCodec] avcodec_parameters_to_context failed: %s\n",
             av_err2str(ret));
        return ret;
    }

    AVDictionary* opts = nullptr;
    if (!av_dict_get(opts, "threads", nullptr, 0)) {
        av_dict_set(&opts, "threads", "auto", 0);
    }

    ret = avcodec_open2(avctx, nullptr, &opts);

    if (opts) {
        av_opt_free(opts);
    }

    if (ret < 0) {
        LOGE("[XVideoCodec] avcodec_open2 failed: %s\n", av_err2str(ret));
        return ret;
    }

    return 0;
}


bool XFFmpegVideoCodec::checkIsValidPacket(AVPacket *pkt) {
    AVStream *stream = mFormatCtx->streams[mIndex];
    AVPacket packet;
    av_parser_parse2(stream->parser, mCodecCtx.get(), &packet.data, &packet.size, pkt->data, pkt->size, pkt->pts, pkt->dts, pkt->pos);
    if (stream->parser->pict_type == AV_PICTURE_TYPE_B) {
        mBFrameIndex++;
        if (!(mBFrameIndex & 1)) {
            return false;
        }
    } else if (stream->parser->pict_type == AV_PICTURE_TYPE_I || stream->parser->pict_type == AV_PICTURE_TYPE_P) {
        mBFrameIndex = 0;
    }
    return true;
}

void XFFmpegVideoCodec::frameConvert(std::shared_ptr<XImage> dst, AVFrame *src) {
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
                    LOGE("[XVideoCodec] sws_getContext failed!\n");
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

std::shared_ptr<XImage> XFFmpegVideoCodec::getImage(long clock) {

    if (!mFormatCtx || mIndex < 0 || !mCodecCtx) {
        return nullptr;
    }

    if (mLastImage) {
        if (clock == mLastClock || mLastImage->pts <= clock && clock <= mLastImage->pts + mLastImage->duration) {
            return mLastImage;
        }
    }

    mLastClock = clock;

    AVStream* stream = mFormatCtx->streams[mIndex];
    int ret;
    for (;;) {
        auto frame = std::make_shared<Frame>();
        ret = avcodec_receive_frame(mCodecCtx.get(), frame->avframe);
        if (ret < 0 && ret != AVERROR(EAGAIN)) {
            if (ret != AVERROR_EOF) {
                LOGE("[XVideoCodec] avcodec_receive_frame failed: %s\n", av_err2str(ret));
            }
            return nullptr;
        }

        if (ret >= 0) {
            if (frame->avframe->pts <= clock) {
                mLastFrame = frame;
                if (clock <= frame->avframe->pts + frame->avframe->pkt_duration) {
                    auto image = std::make_shared<XImage>();
                    image->format = DST_PIX_FMT;
                    image->width = frame->avframe->width;
                    image->height = frame->avframe->height;
                    image->pts = static_cast<long>(frame->avframe->pts);
                    image->duration = static_cast<long>(frame->avframe->pkt_duration);
                    frameConvert(image, frame->avframe);
                    mLastImage = image;
                    return image;
                }
            } else if (clock > frame->avframe->pts + frame->avframe->pkt_duration && mLastFrame) {
                auto image = std::make_shared<XImage>();
                frameConvert(image, mLastFrame->avframe);
                mLastImage = image;
                return image;
            }
            continue;
        }

        auto pkt = std::make_shared<Packet>();
        ret = av_read_frame(mFormatCtx.get(), pkt->avpkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF && !(mStatus & S_READ_EOF)) {
                ret = avcodec_send_packet(mCodecCtx.get(), nullptr);
                if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                    LOGW("[XVideoCodec] avcodec_send_packet(nullptr) failed: %s\n", av_err2str(ret));
                }
                mStatus |= S_READ_EOF;
                continue;
            } else {
                LOGE("[XVideoCodec] av_read_frame failed: %s\n", av_err2str(ret));
            }
            return nullptr;
        } else {
            if (pkt->avpkt->stream_index == mIndex && !(stream->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
                av_packet_rescale_ts(pkt->avpkt, stream->time_base, {1, 1000});
                ret = avcodec_send_packet(mCodecCtx.get(), pkt->avpkt);
                if (ret < 0) {
                    LOGW("[XVideoCodec] avcodec_send_packet(pkt) failed: %s\n", av_err2str(ret));
                }
            }
        }
    }
    return nullptr;
}

int XFFmpegVideoCodec::close() {
    if (mCodecCtx) {
        mCodecCtx.reset();
    }
    return 0;
}


