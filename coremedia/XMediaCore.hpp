//
//  XMediaCore.hpp
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XMediaCore_hpp
#define XMediaCore_hpp

#include <string>

class XMediaCore {
public:
    static XMediaCore& getInstance() {
        static XMediaCore instance;
        return instance;
    }
    
    void setShaderPath(const std::string& shaderPath) {
        mShaderPath = shaderPath;
    }
    
    std::string getShaderPath() const {
        return mShaderPath;
    }
    
private:
    XMediaCore() = default;
    
    ~XMediaCore() = default;

    XMediaCore(const XMediaCore&)= delete;
    
    XMediaCore& operator=(const XMediaCore&)= delete;
    
private:
    std::string mShaderPath;
};

#endif //XMediaCore_hpp
