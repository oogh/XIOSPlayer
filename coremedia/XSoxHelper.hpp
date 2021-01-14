//
// Created by Oogh on 2020/8/14.
//

#ifndef XRENDER_XSOXHELPER_HPP
#define XRENDER_XSOXHELPER_HPP

#include <cinttypes>
#include <cstdio>
#include "XSoxHeader.hpp"

class XSoxHelper {
public:
    XSoxHelper();

    ~XSoxHelper();
    
    void setOutputFile(FILE* fp);
    
    void setSampleRate(int rate);
    
    void setChannels(int channels);
    
    void setSampleCount(int samples);
    
    void setPrecision(int precision);

    /**
     * @brief 执行音频特效处理
     * @param dst 输出参数，指向输出数据缓冲区的指针
     * @param src 采样源数据
     * @param srcLen 采样个数
     * @return 实际输出的采样个数
     */
    int process(uint8_t** dst, uint8_t* src, int srcLen);
    
private:
    static const size_t MAX_SAMPLES = 2048;
    
private:
    FILE *mOutFile = nullptr;
    
    sox_sample_t mSamples[MAX_SAMPLES];
    
    int mSampleRate;
    int mSampleCount;
    int mChannels;
    int mPrecision;
    
};


#endif //XRENDER_XSOXHELPER_HPP
