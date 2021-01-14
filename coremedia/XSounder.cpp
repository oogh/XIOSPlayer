//
// Created by Oogh on 2020/8/30.
//

#include "XSounder.hpp"
#include "XLogger.hpp"
#include <cstdlib>
#include <cstring>
#include <thread>

const char *alError2Str(ALenum error) {
    switch (error) {
        case AL_INVALID_NAME:
            return "AL_INVALID_NAME : Invalid Name parameter passed to AL call";
        case AL_INVALID_ENUM:
            return "AL_INVALID_ENUM : Invalid parameter passed to AL call";
        case AL_INVALID_VALUE:
            return "AL_INVALID_VALUE : Invalid enum parameter value";
        case AL_INVALID_OPERATION:
            return "AL_INVALID_OPERATION : Illegal call";
        case AL_OUT_OF_MEMORY:
            return "AL_OUT_OF_MEMORY : No mojo";
        default:
            return "Unknown error code";
    }
    return "";
}

XSounder::XSounder()
: mDevice(nullptr), mContext(nullptr) {
    init();
}

XSounder::~XSounder() {
    deinit();
}

void XSounder::start() {
    // Rewind the source position and clear the buffer queue
    alSourceRewind(mSource00);
    alSourcei(mSource00, AL_BUFFER, 0);

    // 2. fill the buffer queue
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        uint8_t* data = reinterpret_cast<uint8_t*>(malloc(BYTES_PRE_BUFFER));
        memset(data, '\0', BYTES_PRE_BUFFER);
        alBufferData(mBuffers[i], AL_FORMAT_STEREO16, data, BYTES_PRE_BUFFER, 44100);
        if (data) {
            free(data);
            data = nullptr;
        }
    }

    // 3. queue buffer and start to playing
    alSourceQueueBuffers(mSource00, NUM_BUFFERS, mBuffers);
    alSourcePlay(mSource00);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        LOGE("[XSounder] alBufferData() failed: %s\n", alError2Str(error));
        return;
    }

}

void XSounder::pause() {
    alSourcePause(mSource00);
}

void XSounder::stop() {
    alSourceStop(mSource00);
}

int XSounder::updateAudio(uint8_t *data, int size) {

    ALenum error = AL_NO_ERROR;
    ALint processed, state, queued;
    alGetSourcei(mSource00, AL_SOURCE_STATE, &state);
    alGetSourcei(mSource00, AL_BUFFERS_PROCESSED, &processed);
    alGetSourcei(mSource00, AL_BUFFERS_QUEUED, &queued);
    
    while (processed <= 0) {
        if (queued <= 0) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        alGetSourcei(mSource00, AL_BUFFERS_PROCESSED, &processed);
    }
    
    while (processed > 0) {
        alGetSourcei(mSource00, AL_BUFFERS_QUEUED, &queued);
        ALuint bufferId;
        alSourceUnqueueBuffers(mSource00, 1, &bufferId);
        if ((error = alGetError()) != AL_NO_ERROR) {
            LOGE("[XSounder] alSourceUnqueueBuffers() failed: %s\n", alError2Str(error));
            return -1;
        }
        processed--;

        /* Read the next chunk of data, refill the buffer, and queue it
         * back on the source */
        alBufferData(bufferId, AL_FORMAT_STEREO16, data, size, 44100);
        if ((error = alGetError()) != AL_NO_ERROR) {
            LOGE("[XSounder] alBufferData() failed: %s\n", alError2Str(error));
            return -1;
        }

        alSourceQueueBuffers(mSource00, 1, &bufferId);
        if ((error = alGetError()) != AL_NO_ERROR) {
            LOGE("[XSounder] alSourceQueueBuffers() failed: %s\n", alError2Str(error));
            return -1;
        }
    }

    /* Make sure the source hasn't underrun */
    if (state != AL_PLAYING && state != AL_PAUSED) {
        ALint queued;

        /* If no buffers are queued, playback is finished */
        alGetSourcei(mSource00, AL_BUFFERS_QUEUED, &queued);
        if (queued == 0) {
            return 0;
        }

        alSourcePlay(mSource00);
        if ((error = alGetError()) != AL_NO_ERROR) {
            LOGE("[XSounder] alSourcePlay() failed: %s\n", alError2Str(error));
            return -1;
        }
    }
    
    alGetSourcei(mSource00, AL_BUFFERS_QUEUED, &queued);
    return queued;
}

bool XSounder::init() {
    // 1. 打开默认音频设备
    mDevice = alcOpenDevice(nullptr);
    if (!mDevice) {
        LOGE("[XALPlayer] alcOpenDevice failed: %s\n", alError2Str(alGetError()));
        return false;
    }

    // 2. 创建音频上下文环境
    mContext = alcCreateContext(mDevice, nullptr);
    if (!mContext) {
        LOGE("[XALPlayer] alcCreateContext failed: %s\n", alError2Str(alGetError()));
        return false;
    }

    // 3. 激活音频上下文环境
    if (!alcMakeContextCurrent(mContext)) {
        LOGE("[XALPlayer] alcMakeContextCurrent failed: %s\n", alError2Str(alGetError()));
        return false;
    }

    // 4. 创建音频Buffers
    alGenBuffers(NUM_BUFFERS, mBuffers);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        LOGE("[XALPlayer] alGenBuffers failed: %s\n", alError2Str(alGetError()));
        return false;
    }

    // 5. 创建音频源
    alGenSources(1, &mSource00);
    error = alGetError();
    if (error != AL_NO_ERROR) {
        LOGE("[XALPlayer] alGenSources failed: %s\n", alError2Str(alGetError()));
        return false;
    }

    // 6. 配置音源参数
    alSourcei(mSource00, AL_LOOPING, AL_FALSE);
    alSourcef(mSource00, AL_SOURCE_TYPE, AL_STREAMING);
    alSourcef(mSource00, AL_GAIN, 1.0f);
    alSpeedOfSound(1.0);
    alDopplerVelocity(1.0);
    alDopplerFactor(1.0);
    alSourcef(mSource00, AL_PITCH, 1);
    alSourcef(mSource00, AL_GAIN, 1);
    alSource3f(mSource00, AL_POSITION, 0, 0, 0);
    alSource3f(mSource00, AL_VELOCITY, 0, 0, 0);

    return true;
}

void XSounder::deinit() {
    alDeleteSources(1, &mSource00);

    alDeleteBuffers(NUM_BUFFERS, mBuffers);

    alcMakeContextCurrent(nullptr);

    if (mContext) {
        alcDestroyContext(mContext);
        mContext = nullptr;
    }

    if (mDevice) {
        alcCloseDevice(mDevice);
        mDevice = nullptr;
    }
}
