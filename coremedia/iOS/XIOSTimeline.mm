//
//  XIOSTimeline.mm
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#import "XIOSTimeline.hpp"
#import "XIOSTimelineInternal.hpp"
#import "XIOSTrackInternal.hpp"
#import "XIOSTrack.hpp"
#include "XTimeline.hpp"

@interface XIOSTimeline()
{
    std::shared_ptr<XTimeline> _timeline;
}
@end

@implementation XIOSTimeline

#pragma mark - Life Cycle
- (instancetype)init {
    if (self = [super init]) {
        _timeline = std::make_shared<XTimeline>();
    }
    return self;
}

- (void)dealloc {
    
}

#pragma mark - Public
- (void)addTrack:(XIOSTrack*)track {
    _timeline->addTrack([track getNativeTrack]);
}

- (std::shared_ptr<XTimeline>)getNativeTimeline {
    return _timeline;
}

@end
