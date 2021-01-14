//
// Created by Oogh on 2020/11/4.
//

#include "XTimeline.hpp"
#include "XTrack.hpp"
#include "XLogger.hpp"
#include "XAudioUtils.hpp"

XTimeline::XTimeline()
: mUsedSample(0), mDuration(0) {

}

XTimeline::~XTimeline() {

}

int XTimeline::addTrack(std::shared_ptr<XTrack> track) {
    std::lock_guard<std::mutex> lock(mMutex);
    track->setTimeline(shared_from_this());
    mTrackList.emplace_back(track);
    updateDuration();
    return static_cast<int>(mTrackList.size());
}

int XTimeline::removeTrack(int id) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto iter = std::find_if(mTrackList.begin(), mTrackList.end(), [id](std::shared_ptr<XTrack> track) {
        return track->getId() == id;
    });
    mTrackList.erase(iter);
    updateDuration();
    return mTrackList.size();
}

std::vector<std::shared_ptr<XImage>> XTimeline::getImage(long clock) {
    std::vector<std::shared_ptr<XImage>> images;
    images.reserve(mTrackList.size());

    for (auto& track: mTrackList) {
        auto image = track->getImage(clock);
        images.emplace_back(std::move(image));
    }

    return images;
}

std::shared_ptr<XSample> XTimeline::getSample(int length) {
    std::shared_ptr<XSample> sample = std::make_shared<XSample>(length);

    for (auto& track: mTrackList) {
        auto trackSample = track->getSample(getClock(), length);
        if (trackSample) {
            XAudioUtils::mix(sample->data, trackSample->data, AUDIO_FMT_S16, length, 128);
        }
    }
    if (sample) {
        mUsedSample += length;
    }

    return sample;
}

long XTimeline::getClock() {
    std::lock_guard<std::mutex> lock(mMutex);
    long clock = static_cast<long>(mUsedSample * 1.0 / 44100 / 2 / 2 * 1000);
//    LOGD("[XTimeline] current play clock: %ld\n", clock);
    return clock;
}

bool XTimeline::isCompleted() {
    return getClock() >= mDuration;
}

void XTimeline::updateDuration() {
    for (auto& track: mTrackList) {
        mDuration = std::max(mDuration, track->getDelay() + track->getClipDuration());
    }
}

