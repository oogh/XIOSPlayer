//
//  XFFHeader.h
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XEXPORTER_XFFHEADER_H
#define XEXPORTER_XFFHEADER_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/jni.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/time.h>
#include <libavutil/timestamp.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavutil/display.h>
}

struct InputFormatDeleter {
    void operator()(AVFormatContext* ic) {
        avformat_close_input(&ic);
    }
};

struct OutputFormatDeleter {
    void operator()(AVFormatContext* ic) {
        avformat_free_context(ic);
    }
};

struct CodecDeleter {
    void operator()(AVCodecContext* avctx) {
        avcodec_free_context(&avctx);
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* sws) {
        sws_freeContext(sws);
    }
};

struct SwrContextDeleter {
    void operator()(SwrContext* swr) {
        swr_free(&swr);
    }
};

struct Packet {
    AVPacket *avpkt = nullptr;

    Packet() {
        avpkt = av_packet_alloc();
        av_init_packet(avpkt);
        avpkt->data = nullptr;
        avpkt->size = 0;
    }

    ~Packet() {
        if (avpkt) {
            av_packet_free(&avpkt);
        }
    }
};

struct Frame {
    AVFrame* avframe = nullptr;

    Frame() {
        avframe = av_frame_alloc();
    }

    ~Frame() {
        if (avframe) {
            av_frame_free(&avframe);
        }
    }
};

#endif //XEXPORTER_XFFHEADER_H
