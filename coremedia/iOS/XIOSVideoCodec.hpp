//
//  XIOSVideoCodec.hpp
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XIOSVIDEOCODEC_HPP
#define XIOSVIDEOCODEC_HPP

#include "XVideoCodec.hpp"

class XIOSVideoCodec : public XVideoCodec {
public:
    XIOSVideoCodec();
    
    ~XIOSVideoCodec() override;
    
    void setFilename(const std::string& filename) override;
    
    std::shared_ptr<XImage> getImage(long clock) override;
    
private:
    int open() override;
    
    int close() override;
};

#endif //XIOSVIDEOCODEC_HPP
