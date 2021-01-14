//
// Created by Oogh on 2020/8/30.
//

#ifndef XRENDER_XAUDIOPLAYER_HPP
#define XRENDER_XAUDIOPLAYER_HPP

#include <cinttypes>
#include "XALHeader.hpp"

class XSounder {
public:
    XSounder();

    ~XSounder();

    void start();

    void pause();

    void stop();

public:
    int updateAudio(uint8_t* data, int size);

private:
    bool init();

    void deinit();

private:
    static const int NUM_BUFFERS = 4;
    static const int BYTES_PRE_BUFFER = 4096;

private:
    ALCdevice* mDevice;
    ALCcontext* mContext;

    ALuint mBuffers[NUM_BUFFERS];
    ALuint mSource00;
};


#endif //XRENDER_XAUDIOPLAYER_HPP
