//
//  XVideoDecoderByVTB.hpp
//  XIOSPlayer
//
//  Created by oogh on 2021/1/14.
//

#ifndef XVideoDecoderByVTB_h
#define XVideoDecoderByVTB_h
#include <string>
#include <memory>
#include <list>
#include <mutex>
#import <VideoToolbox/VideoToolbox.h>

struct XImage {
    CMSampleBufferRef sampleBuffer = nullptr;
    long pts = -1L;
    long duration = 0L;
    int imgId = 0;
    static int ID_COUNTER;
    
    XImage() {
        
    }
    
    XImage(const XImage& that) {
        CMSampleBufferCreateCopy(kCFAllocatorDefault, that.sampleBuffer, &sampleBuffer);
        this->pts = that.pts;
        this->duration = that.duration;
    }
    
    XImage& operator=(const XImage& that) {
        if (this != &that) {
            CMSampleBufferCreateCopy(kCFAllocatorDefault, that.sampleBuffer, &sampleBuffer);
            this->pts = that.pts;
            this->duration = that.duration;
        }
        return *this;
    }
    
    XImage(XImage&& that) {
        this->sampleBuffer = that.sampleBuffer;
        this->pts = that.pts;
        this->duration = that.duration;

        that.sampleBuffer = nullptr;
        that.pts = -1L;
        that.duration = 0L;
    }
    
    XImage& operator=(XImage&& that) {
        if (this != &that) {
            this->sampleBuffer = that.sampleBuffer;
            this->pts = that.pts;
            this->duration = that.duration;
            
            that.sampleBuffer = nullptr;
            that.pts = -1L;
            that.duration = 0L;
        }
        return *this;
    }
    
    ~XImage() {
        if (sampleBuffer) {
            CFRelease(sampleBuffer);
            sampleBuffer = nullptr;
        }
        pts = -1L;
        duration = 0L;
    }
};

class ImageList {
public:
    ImageList(): mIsReady(false), mMaxPts(-1) {
        
    }
    
    ~ImageList() {
        mIsReady = false;
        std::list<std::shared_ptr<XImage>>().swap(mImageList);
    }
    
    int put(std::shared_ptr<XImage>&& image) {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mCachedImage) {
            mImageList.emplace_back(std::move(mCachedImage));
        }
        
        if (mImageList.empty()) {
            mIsReady = true;
            mImageList.emplace_back(std::forward<std::shared_ptr<XImage>>(image));
        } else {
            if (image->pts > mMaxPts) {
                mMaxPts = image->pts;
                mCachedImage = image;
                mImageList.sort([] (const std::shared_ptr<XImage>& prev, const std::shared_ptr<XImage>& next) {
                    return prev->pts < next->pts;
                });
                mIsReady = true;
            } else {
                mImageList.emplace_back(std::forward<std::shared_ptr<XImage>>(image));
            }
        }
        return static_cast<int>(mImageList.size());
    }
    
    std::shared_ptr<XImage> get(long clock) {
        if (mLastImage && mLastImage->pts <= clock && clock < mLastImage->pts + mLastImage->duration) {
            return mLastImage;
        }
        
        if (!mIsReady) {
            return nullptr;
        }
        
        std::lock_guard<std::mutex> lock(mMutex);
        for (auto iter = mImageList.begin(); iter != mImageList.end();) {
            auto image = *iter;
            if (image->pts <= clock && clock < image->pts + image->duration) {
                mLastImage = image;
                mImageList.erase(iter++);
                return image;
            } else  {
                if (mLastImage && mLastImage->pts < clock && clock < image->pts) {
                    return mLastImage;
                }
                iter++;
            }
            
        }
        
        mIsReady = false;
        
        return nullptr;
    }
    
private:
    std::list<std::shared_ptr<XImage>> mImageList;
    std::mutex mMutex;
    std::atomic_bool mIsReady;
    std::shared_ptr<XImage> mCachedImage;
    long mMaxPts;
    std::shared_ptr<XImage> mLastImage;
};

struct DecoderContext;

class XVideoDecoderByVTB {
public:
    explicit XVideoDecoderByVTB(const std::string& filename);
    
    ~XVideoDecoderByVTB();
    
    int open();
    
    std::shared_ptr<XImage> getImage(int& code, long clock);

    void close();
    
    void push(std::shared_ptr<XImage>&& image);
    
private:
    int parseVideoStream();
    
    int createVideoFormatDescription();
    
    int createVTDecompressionSession();
    
    int decode(uint8_t* data, size_t size, long pts, long dts, long duration);
    
public:
    std::mutex mMutex;
    
private:
    std::shared_ptr<DecoderContext> mCtx;
    std::string mFilename;
    std::shared_ptr<XImage> mTempImage;
    std::shared_ptr<ImageList> mImageList;
    bool mIsReadyForData;
    long mPtsMax;
    
    long mLastClock;
};

#endif /* XVideoDecoderByVTB_h */
