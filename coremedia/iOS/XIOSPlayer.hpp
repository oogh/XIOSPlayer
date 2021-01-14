//
//  XIOSPlayer.hpp
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XIOSPlayer_hpp
#define XIOSPlayer_hpp

#import <Foundation/Foundation.h>

@class XIOSTimeline;
@class XIOSView;

@interface XIOSPlayer : NSObject

- (instancetype)init;

- (void)setTimeline:(XIOSTimeline*)timeline;

- (void)setDisplayView:(XIOSView*)view;

- (void)prepare;

- (void)start;

- (void)seekTo:(long)targetPos;

- (void)pause;

- (void)stop;

@end

#endif /* XIOSPlayer_hpp */
