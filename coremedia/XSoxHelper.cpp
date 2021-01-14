//
// Created by Oogh on 2020/8/14.
//

#include "XSoxHelper.hpp"
#include "XLogger.hpp"
#include "XException.hpp"

XSoxHelper::XSoxHelper() {
    
}

XSoxHelper::~XSoxHelper() {

}

int XSoxHelper::process(uint8_t **dst, uint8_t *src, int srcSize) {
    int result = 0;
    
    if (sox_init() != SOX_SUCCESS) {
        LOGE("[XSoxHelper] sox_init() failed!");
        return -1;
    }
    
    sox_signalinfo_t signalInfo;
    signalInfo.channels = mChannels;
    signalInfo.length = mSampleCount * mChannels;
    signalInfo.precision = mPrecision;
    signalInfo.rate = mSampleRate;
    signalInfo.mult = nullptr;
    
    sox_encodinginfo_t encodingInfo;
    encodingInfo.encoding = SOX_ENCODING_SIGN2;
    encodingInfo.bits_per_sample = 16;
    encodingInfo.opposite_endian = sox_false;
    encodingInfo.reverse_bits = sox_option_no;
    encodingInfo.reverse_bytes = sox_option_no;
    encodingInfo.reverse_nibbles = sox_option_no;
    encodingInfo.compression = 0;

    sox_format_t* in = sox_open_mem_read(src, srcSize, &signalInfo, &encodingInfo, "raw");
    if (!in) {
        LOGE("[XSoxHelper] sox_open_mem_read() failed!\n");
        return -1;
    }
    
    uint8_t* buffer = nullptr;
    size_t bufferSize = 0;
    sox_format_t* out = sox_open_memstream_write((char **)dst, &bufferSize, &signalInfo, &encodingInfo, "raw", nullptr);
    if (!out) {
        LOGE("[XSoxHelper] sox_open_memstream_write() failed!\n");
        return -1;
    }
    
    sox_effects_chain_t* chains = sox_create_effects_chain(&in->encoding, &out->encoding);
    if (!chains) {
        LOGE("[XSoxHelper] sox_create_effects_chain() failed!\n");
        return -1;
    }
    
    char* argv[4];

    // input effect
    sox_effect_t* input = sox_create_effect(sox_find_effect("input"));
    if (!input) {
        LOGE("[XSoxHelper] sox_create_effect(input) failed!\n");
        return -1;
    }
    argv[0] = {reinterpret_cast<char*>(in)};
    if (sox_effect_options(input, 1, argv) != SOX_SUCCESS) {
        LOGE("[XSoxHelper] sox_effect_options(input) failed!\n");
        return -1;
    }
    if (sox_add_effect(chains, input, &in->signal, &out->signal) != SOX_SUCCESS) {
        LOGE("[XSoxHelper] sox_add_effect(input) failed!\n");
        return -1;
    }

    // pitch effect
//    sox_effect_t* pitch = sox_create_effect(sox_find_effect("pitch"));
//    if (!pitch) {
//        LOGE("[XSoxHelper] sox_create_effect(pitch) failed!\n");
//        return -1;
//    }
//    argv[0] = "1450";
//    argv[1] = "120";
//    argv[2] = "30";
//    argv[3] = "15";
//    if (sox_effect_options(pitch, 4, argv) != SOX_SUCCESS) {
//        LOGE("[XSoxHelper] sox_effect_options(pitch) failed!\n");
//        return -1;
//    }
//
//    if (sox_add_effect(chains, pitch, &in->signal, &out->signal) != SOX_SUCCESS) {
//        LOGE("[XSoxHelper] sox_add_effect(pitch) failed!\n");
//        return -1;
//    }
    
    // vol effect
    sox_effect_t* vol = sox_create_effect(sox_find_effect("vol"));
    if (!vol) {
        LOGE("[XSoxHelper] sox_create_effect(vol) failed!\n");
        return -1;
    }
    argv[0] = "1.0";
    if (sox_effect_options(vol, 1, argv) != SOX_SUCCESS) {
        LOGE("[XSoxHelper] sox_effect_options(vol) failed!\n");
        return -1;
    }

    if (sox_add_effect(chains, vol, &in->signal, &out->signal) != SOX_SUCCESS) {
        LOGE("[XSoxHelper] sox_add_effect(vol) failed!\n");
        return -1;
    }
    
    // rate effect
    sox_effect_t* rate = sox_create_effect(sox_find_effect("rate"));
    if (!rate) {
        LOGE("[XSoxHelper] sox_create_effect(rate) failed!\n");
        return -1;
    }

    argv[0] = "-m";
    if (sox_effect_options(rate, 1, argv) != SOX_SUCCESS) {
        LOGE("[XSoxHelper] sox_effect_options(rate) failed!\n");
        return -1;
    }

    if (sox_add_effect(chains, rate, &in->signal, &out->signal) != SOX_SUCCESS) {
        LOGE("[XSoxHelper] sox_add_effect(rate) failed!\n");
        return -1;
    }

    // output effect
    sox_effect_t* output = sox_create_effect(sox_find_effect("output"));
    if (!output) {
        LOGE("[XSoxHelper] sox_create_effect(output) failed!\n");
        return -1;
    }
    argv[0] = {reinterpret_cast<char*>(out)};
    if (sox_effect_options(output, 1, argv) != SOX_SUCCESS) {
        LOGE("[XSoxHelper] sox_effect_options(output) failed!\n");
        return -1;
    }
    if (sox_add_effect(chains, output, &in->signal, &out->signal) != SOX_SUCCESS) {
        LOGE("[XSoxHelper] sox_add_effect(output) failed!\n");
        return -1;
    }

    sox_flow_effects(chains, nullptr, nullptr);
    
    int soxRead, soxWrite;
    do {
        soxRead = sox_read(in, mSamples, MAX_SAMPLES);
        soxWrite = sox_write(out, mSamples, soxRead);
        if (soxRead != soxWrite) {
            LOGE("[XSoxHelper] soxRead(%d) != soxWrite(%d)\n", soxRead, soxWrite);
            break;
        }
    } while (soxRead > 0);
    
    sox_delete_effects_chain(chains);
    
    
    sox_close(out);
    sox_close(in);
    
    sox_quit();
    
    result = bufferSize;
    
    return result;
}

void XSoxHelper::setOutputFile(FILE* fp) {
    if (!mOutFile) {
        mOutFile = fp;
    }
}

void XSoxHelper::setSampleRate(int rate) {
    mSampleRate = rate;
}

void XSoxHelper::setChannels(int channels) {
    mChannels = channels;
}

void XSoxHelper::setSampleCount(int samples) {
    mSampleCount = samples;
}

void XSoxHelper::setPrecision(int precision) {
    mPrecision = precision;
}
