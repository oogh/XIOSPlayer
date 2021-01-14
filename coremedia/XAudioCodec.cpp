//
// Created by Oogh on 2020/9/18.
//

#include "XAudioCodec.hpp"
#include "XLogger.hpp"


XAudioCodec::XAudioCodec()
: mStatus(0), mAudioBuffer(nullptr), mAudioBufferIndex(0), mAudioBufferSize(0),  mLoop(false) {

}

XAudioCodec::~XAudioCodec() {

}

int XAudioCodec::setFilename(std::string filename) {
    mFilename = filename;
    return 0;
}

void XAudioCodec::setLoop(bool loop) {
    mLoop = loop;
}

int XAudioCodec::open() {
    if (mFilename.empty()) {
        return AVERROR(EINVAL);
    }

    AVFormatContext* ic = avformat_alloc_context();
    if (!ic) {
        return AVERROR(ENOMEM);
    }
    mFormatCtx = std::unique_ptr<AVFormatContext, InputFormatDeleter>(ic);

    int ret = avformat_open_input(&ic, mFilename.data(), nullptr, nullptr);
    if (ret < 0) {
        LOGE("[XAudioCodec] avformat_open_input failed: %s\n", av_err2str(ret));
        return ret;
    }

    ret = avformat_find_stream_info(ic, nullptr);
    if (ret < 0) {
        LOGE("[XAudioCodec] avformat_find_stream_info failed: %s\n", av_err2str(ret));
        return ret;
    }

    for (int i = 0; i < mFormatCtx->nb_streams; ++i) {
        mFormatCtx->streams[i]->discard = AVDISCARD_ALL;
    }

    int index = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (index < 0) {
        return AVERROR_STREAM_NOT_FOUND;
    }
    mIndex = index;
    AVStream* stream = mFormatCtx->streams[index];
    stream->discard = AVDISCARD_DEFAULT;

    AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        LOGE("[XAudioCodec] avcodec_find_decoder(%s) failed!\n", avcodec_get_name(stream->codecpar->codec_id));
        return AVERROR_DECODER_NOT_FOUND;
    }

    AVCodecContext* avctx = avcodec_alloc_context3(decoder);
    if (!avctx) {
        LOGE("[XAudioCodec] avcodec_alloc_context3 failed!\n");
        return AVERROR(ENOMEM);
    }
    mCodecCtx = std::unique_ptr<AVCodecContext, CodecDeleter>(avctx);

    ret = avcodec_parameters_to_context(avctx, stream->codecpar);
    if (ret < 0) {
        LOGE("[XAudioCodec] avcodec_parameters_to_context failed: %s\n",
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
        LOGE("[XAudioCodec] avcodec_open2 failed: %s\n", av_err2str(ret));
        return ret;
    }

    // 重采样环境配置
    SwrContext* swr = swr_alloc();
    if (!swr) {
        return AVERROR(ENOMEM);
    }
    mSwrContext = std::unique_ptr<SwrContext,SwrContextDeleter>(swr);

    uint64_t channelLayout = (avctx->channel_layout &&
                              avctx->channels == av_get_channel_layout_nb_channels(avctx->channel_layout))
                             ? avctx->channel_layout : av_get_default_channel_layout(avctx->channels);

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

    if (mAudioBuffer) {
        av_freep(&mAudioBuffer);
    }

    // 申请目标缓冲区内存空间
    int srcSampleCount = avctx->frame_size > 0 ? avctx->frame_size : 1024;
    int dstSampleCount = static_cast<int>(av_rescale_rnd(srcSampleCount, DST_SAMPLE_RATE, avctx->sample_rate, AV_ROUND_UP));

    int dstChannels = av_get_channel_layout_nb_channels(DST_CHANNEL_LAYOUT);
    ret = av_samples_alloc(&mAudioBuffer, nullptr, dstChannels, dstSampleCount, DST_SAMPLE_FMT, 0);
    if (ret < 0) {
        LOGE("[XFFProducer] av_samples_alloc_array_and_samples failed: %s\n", av_err2str(ret));
        return AVERROR(ENOMEM);
    }
    return 0;
}

int XAudioCodec::seekFileTo(long target) {
    if (!mFormatCtx || mIndex < 0) {
        return AVERROR(EINVAL);
    }

    mStatus &= ~(S_READ_EOF | S_AUDIO_DECODE_EOF);

    int64_t ts = av_rescale(target, AV_TIME_BASE, 1000);
    int ret = avformat_seek_file(mFormatCtx.get(), -1, INT64_MIN, ts, INT64_MAX, 0);
    if (ret < 0) {
        LOGE("[XAudioCodec] avformat_seek_file failed: %s\n", av_err2str(ret));
        return ret;
    }

    return 0;
}

std::shared_ptr<Packet> XAudioCodec::getPacket() {
    if (!mFormatCtx || mIndex < 0) {
        return nullptr;
    }

    int ret;
    do {
        auto pkt = std::make_shared<Packet>();
        ret = av_read_frame(mFormatCtx.get(), pkt->avpkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF && mLoop) {
                ret = seekFileTo(0);
                if (ret < 0) {
                    return nullptr;
                }
                continue;
            }
            return nullptr;
        }

        if (pkt->avpkt->stream_index == mIndex) {
            return pkt;
        }

    } while (ret >= 0);

    return nullptr;
}

int XAudioCodec::decodeFrame() {
    int ret = AVERROR(EAGAIN);
    for (;;) {
        auto frame = std::make_shared<Frame>();
        ret = avcodec_receive_frame(mCodecCtx.get(), frame->avframe);
        if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
            LOGE("[XAudioCodec] avcodec_receive_frame failed: %s\n", av_err2str(ret));
            return ret;
        }

        if (ret == AVERROR_EOF) {
            // TODO(oogh): 2020/09/22 视频解码完成
            mStatus |= S_AUDIO_DECODE_EOF;
            return ret;
        }

        if (ret >= 0) {
            int len = sampleConvert(frame->avframe);
            return len;
        }

        auto pkt = std::make_shared<Packet>();
        ret = av_read_frame(mFormatCtx.get(), pkt->avpkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                if (!(mStatus & S_READ_EOF)) {
                    ret = avcodec_send_packet(mCodecCtx.get(), nullptr);
                    if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                        LOGE("[XAudioCodec] avcodec_send_packet failed: %s\n", av_err2str(ret));
                        return ret;
                    }

                    if (mLoop) {
                        ret = seekFileTo(0);
                        if (ret < 0) {
                            return ret;
                        }
                    } else {
                        mStatus |= S_READ_EOF;
                    }
                }
                continue;
            }
            return ret;
        }

        if (pkt->avpkt->stream_index == mIndex) {
            av_packet_rescale_ts(pkt->avpkt, mFormatCtx->streams[mIndex]->time_base, {1, 1000});
            ret = avcodec_send_packet(mCodecCtx.get(), pkt->avpkt);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                LOGE("[XAudioCodec] avcodec_send_packet failed: %s\n", av_err2str(ret));
                return ret;
            }
        }
    }
    return 0;
}

std::shared_ptr<XSample> XAudioCodec::getSample(int length) {
    auto sample = std::make_shared<XSample>(length);
    int available, writeLen;
    while (length > 0) {
        if (mAudioBufferIndex >= mAudioBufferSize) {
            if (decodeFrame() == AVERROR_EOF) {
                return sample;
            }
        }

        available = mAudioBufferSize - mAudioBufferIndex;
        writeLen = std::min(available, length);

        memcpy(sample->data, mAudioBuffer + mAudioBufferIndex, writeLen);
        mAudioBufferIndex += writeLen;
        length -= writeLen;
    }

    return sample;
}

int XAudioCodec::sampleConvert(AVFrame *src) {
    if (mAudioBuffer) {
        av_freep(&mAudioBuffer);
    }

    // 1. 分配目标缓冲区内存空间
    int dstChannels = av_get_channel_layout_nb_channels(DST_CHANNEL_LAYOUT);
    int dstSampleCount = static_cast<int>(av_rescale_rnd(swr_get_delay(mSwrContext.get(), src->sample_rate) + src->nb_samples, DST_SAMPLE_RATE, src->sample_rate, AV_ROUND_UP));
    mAudioBufferSize = av_samples_alloc(&mAudioBuffer, nullptr, dstChannels, dstSampleCount, DST_SAMPLE_FMT, 1);
    if (mAudioBufferSize < 0) {
        LOGE("[XFFProducer] av_samples_alloc failed!");
        return AVERROR(ENOMEM);
    }

    // 2. 执行重采样
    int count = swr_convert(mSwrContext.get(), &mAudioBuffer, dstSampleCount, (const uint8_t **)src->data, src->nb_samples);
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

    mAudioBufferIndex = 0;
    mAudioBufferSize = size;

    return mAudioBufferSize;
}

void XAudioCodec::close() {

    if (mAudioBuffer) {
        av_freep(&mAudioBuffer);
    }
    mAudioBufferIndex = 0;
    mAudioBufferSize = 0;

    if (mSwrContext) {
        mSwrContext.reset();
    }

    if (mCodecCtx) {
        mCodecCtx.reset();
    }

    if (mFormatCtx) {
        mFormatCtx.reset();
    }
}






