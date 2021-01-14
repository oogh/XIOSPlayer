//
// Created by Oogh on 2020/11/4.
//

#ifndef XRENDER_XTRACK_HPP
#define XRENDER_XTRACK_HPP

#include <string>

#include "XSample.hpp"
#include "XImage.hpp"

class XTimeline;
class XVideoCodec;
class XAudioCodec;

class XTrack {
public:
    XTrack();

    ~XTrack();

    void setTimeline(const std::shared_ptr<XTimeline>& timeline);

    void setFilename(const std::string& filename);

    void setDelay(long delay);

    void setClipStartTime(long startTime);

    void setClipEndTime(long endTime);

    int getId() const;

    std::string getFilename() const;

    long getDelay() const;

    long getClipStartTime() const;

    long getClipEndTime() const;

    long getClipDuration() const;

    std::shared_ptr<XSample> getSample(long clock, int length);

    std::shared_ptr<XImage> getImage(long clock);

private:
    static int ID_GENERATOR;

private:
    std::weak_ptr<XTimeline> mTimeline;
    std::string mFilename;
    int mId;
    long mDelay;
    long mClipStartTime;
    long mClipEndTime;

    std::unique_ptr<XAudioCodec> mAudioCodec;
    std::unique_ptr<XVideoCodec> mVideoCodec;
};


#endif //XRENDER_XTRACK_HPP
