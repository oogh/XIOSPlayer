//
// Created by Oogh on 2020/3/19.
//

#include <chrono>
#include "XRender.hpp"
#include "XException.hpp"
#include "XImageUtil.hpp"
#include "XImage.hpp"
#include "XThreadUtils.hpp"
#include "XLogger.hpp"
#include "XTimeCounter.hpp"
#include "XFFProducer.hpp"
#include "XTexture.hpp"

XRender::XRender(): mTextureWidth(0), mTextureHeight(0), mTargetPos(0), mAbortReq(false), mPauseReq(true) {
    
}

XRender::~XRender() {
    if (mRefreshTid && mRefreshTid->joinable()) {
        mRefreshTid->join();
    }
    mRefreshTid.reset();
}


void XRender::update(const std::vector<std::shared_ptr<XImage>>& images) {
    std::lock_guard<std::mutex> lock(mMutex);
    for (auto& image: images) {
        auto iter = std::find_if(mTextureList.begin(), mTextureList.end(), [image](const std::shared_ptr<XTexture>& texture) {
            if (image == nullptr) {
                return false;
            }
            return image->textureId == texture->getId();
        });

        std::shared_ptr<XTexture> texture;
        if (iter != mTextureList.end()) {
            texture = *iter;
        } else {
            if (image) {
                texture = std::make_shared<XTexture>(image->textureId, image->width, image->height);
                mTextureList.emplace_back(texture);
            }
        }
        
        if (image && image->pixels[0]) { // RGB系列
            texture->setPixels(image->pixels[0], image->linesize[0] / 4, image->height);
        } else if (image && image->pixels[3]) { // CMSampleBufferRef
            // CMSampleBufferRef -> Texture
            
        }
        
    }
    mContinueRefreshCond.notify_one();
}

void XRender::onSurfaceCreated() {

}

void XRender::onSurfaceChanged(int width, int height) {
    // 居中显示
    // glViewport(width / 4, height / 4, width / 2, height / 2);
    
    // 全画布显示
    glViewport(0, 0, width, height);
}

void XRender::onDrawFrame() {
    std::for_each(mTextureList.begin(), mTextureList.end(), [](std::shared_ptr<XTexture>& texture) {
        if (!texture->drawable()) {
            texture->create();
        }
        texture->draw();
    });
}

void XRender::stop() {
    std::lock_guard<std::mutex> lock(mMutex);
    mAbortReq = true;
    mContinueRefreshCond.notify_one();
    
    if (mRefreshTid && mRefreshTid->joinable()) {
        mRefreshTid->join();
    }
}
