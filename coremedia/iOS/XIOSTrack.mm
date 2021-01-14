//
//  XIOSTrack.mm
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#import "XIOSTrack.hpp"
#import "XIOSTrackInternal.hpp"
#include "XTrack.hpp"

@interface XIOSTrack()
{
    std::shared_ptr<XTrack> _track;
}
@end

@implementation XIOSTrack

#pragma mark - Life Cycle
- (instancetype)init {
    if (self = [super init]) {
        _track = std::make_shared<XTrack>();
    }
    return self;
}

- (void)dealloc {
    if (_track) {
        _track.reset();
    }
}

#pragma mark - Public
- (void)setFilename:(NSString*)filename {
    _track->setFilename(filename.UTF8String);
}

- (void)setDelay:(long)delay {
    _track->setDelay(delay);
}

- (void)setClipStartTime:(long)startTime {
    _track->setClipStartTime(startTime);
}

- (void)setClipEndTime:(long)endTime {
    _track->setClipEndTime(endTime);
}

- (std::shared_ptr<XTrack>)getNativeTrack {
    return _track;
}

@end
