//
// Created by Oogh on 2020/11/4.
//

#include "XAudioUtils.hpp"

#define ADJUST_VOLUME(s, v) (s = ( s * v) / 128)

int XAudioUtils::mix(uint8_t* dst, const uint8_t* src, int format, int len, int volume) {
    if (!dst || !src || len <= 0) {
        return -1;
    }

    switch (format) {
        case AUDIO_FMT_S16: {
            int16_t src1, src2;
            int dst_sample;
            const int max_audioval = ((1 << (16 - 1)) - 1);
            const int min_audioval = -(1 << (16 - 1));

            len /= 2;
            while (len--) {
                src1 = ((src[1]) << 8 | src[0]);
                ADJUST_VOLUME(src1, volume);
                src2 = ((dst[1]) << 8 | dst[0]);
                src += 2;
                dst_sample = src1 + src2;
                if (dst_sample > max_audioval) {
                    dst_sample = max_audioval;
                } else if (dst_sample < min_audioval) {
                    dst_sample = min_audioval;
                }
                dst[0] = dst_sample & 0xFF;
                dst_sample >>= 8;
                dst[1] = dst_sample & 0xFF;
                dst += 2;
            }
        } break; // AV_SAMPLE_FMT_S16

        default:
            break;
    }
    return 0;
}