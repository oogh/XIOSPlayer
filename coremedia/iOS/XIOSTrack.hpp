//
//  XIOSTrack.hpp
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XIOSTrack_hpp
#define XIOSTrack_hpp

#import <Foundation/Foundation.h>

@interface XIOSTrack : NSObject

- (instancetype)init;

- (void)setFilename:(NSString*)filename;

- (void)setDelay:(long)delay;

- (void)setClipStartTime:(long)startTime;

- (void)setClipEndTime:(long)endTime;

@end

#endif /* XIOSTrack_hpp */
