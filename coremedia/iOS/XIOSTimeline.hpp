//
//  XIOSTimeline.hpp
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XIOSTimeline_hpp
#define XIOSTimeline_hpp

#import <Foundation/Foundation.h>

@class XIOSTrack;

@interface XIOSTimeline : NSObject

- (void)addTrack:(XIOSTrack*)track;

@end

#endif /* XIOSTimeline_hpp */
