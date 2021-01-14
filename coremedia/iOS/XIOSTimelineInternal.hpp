//
//  XIOSTimelineInternal.hpp
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XIOSTimeline_Internal_hpp
#define XIOSTimeline_Internal_hpp

#import "XIOSTimeline.hpp"
#include "XTimeline.hpp"

@interface XIOSTimeline()
- (std::shared_ptr<XTimeline>)getNativeTimeline;
@end

#endif /* XIOSTimeline_Internal_hpp */
