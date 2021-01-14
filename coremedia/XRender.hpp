//
// Created by Oogh on 2020/3/19.
//

#ifndef ANDROIDDEMO_XRENDER_H
#define ANDROIDDEMO_XRENDER_H

#include <memory>
#include <string>
#include <mutex>
#include <list>
#include "XGLHeader.hpp"
#include "XRenderObserver.hpp"

class XFFProducer;
class XTexture;
class XImage;

class XRender : public XRenderObserver {
    using OnProgressChangeCallback = std::function<void(long current, long duration)>;
public:
    XRender();

    ~XRender();
    
    void onSurfaceCreated();

    void onSurfaceChanged(int width, int height);

    void onDrawFrame();

    void stop();
    
public:
    void update(const std::vector<std::shared_ptr<XImage>>& images) override;
    
private:
    int mTextureWidth;
    int mTextureHeight;

    std::unique_ptr<XFFProducer> mProducer;

    long mTargetPos;
    
    std::unique_ptr<std::thread> mRefreshTid;
    
    bool mPauseReq;
    
    bool mAbortReq;
    
    std::mutex mMutex;
    std::condition_variable mContinueRefreshCond;
    
    OnProgressChangeCallback mProgressChangeCallback;

    std::list<std::shared_ptr<XTexture>> mTextureList;
};
#endif //ANDROIDDEMO_XRENDER_H
