//
//  XProducable.h
//  XExporter
//
//  Created by Oogh on 2020/3/19.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XEXPORTER_XPRODUCABLE_H
#define XEXPORTER_XPRODUCABLE_H

#include <memory>
#include <string>

class XImage;
class XSample;

///< 生产模式
enum ProduceMode {
    ///< 软解码（默认值）
    PRODUCE_MODE_SOFTWARE = 0,

    ///< 硬解码
    PRODUCE_MODE_HARDWARE
};

class XProducable {
public:
    XProducable();

    virtual ~XProducable();

    virtual void setDisableVideo(bool disabled);

    virtual void setDisableAudio(bool disabled);

    virtual void setInput(const std::string& filename);

    virtual void setProduceMode(ProduceMode mode);

    virtual void start();

    virtual std::shared_ptr<XImage> peekImage(long clock);
    
    virtual void endCurrentImageUse();

    virtual int readSamples(uint8_t* buffer, int length);

    virtual void stop();
    
public:
    virtual long getOriginalDuration() const;
    
    virtual int getOriginalWidth() const;
    
    virtual int getOriginalHeight() const;
    
    virtual int getOriginalRotation() const;
    
    virtual int getOriginalFrameRate() const;

protected:
    std::string mFilename;

    bool mDisableVideo;

    bool mDisableAudio;

    ProduceMode mProduceMode;
};


#endif //XEXPORTER_XPRODUCABLE_H
