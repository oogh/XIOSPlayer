//
//  XFileProducer.h
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XEXPORTER_XFILEPRODUCER_H
#define XEXPORTER_XFILEPRODUCER_H

#include "XProducable.hpp"
#include <fstream>
#include "XImage.hpp"

class XFileProducer : public XProducable {

public:
    XFileProducer();

    ~XFileProducer() override;

    void setInput(const std::string& filename) override;

    std::shared_ptr<XImage> peekImage(long clock) override;

private:
    std::ifstream mFileStream;
    uint8_t* mRGBABuffer;
    int mRGBABufferSize;
    int mWidth;
    int mHeight;
};


#endif //XEXPORTER_XFILEPRODUCER_H
