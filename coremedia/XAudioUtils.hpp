//
// Created by Oogh on 2020/11/4.
//

#ifndef XRENDER_XAUDIOUTILS_HPP
#define XRENDER_XAUDIOUTILS_HPP

#include <cinttypes>

enum {
    AUDIO_FMT_NONE = 0,
    AUDIO_FMT_S16 = 1
};

class XAudioUtils {
public:
    static int mix(uint8_t* dst, const uint8_t* src, int format, int len, int volume);
};


#endif //XRENDER_XAUDIOUTILS_HPP
