//
//  XIOSView.hpp
//  XRender
//
//  Created by Oogh on 2020/04/17.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XIOSView_hpp
#define XIOSView_hpp

#import <UIKit/UIKit.h>

typedef void (^OnProgressChangeCallback)(long current, long duration);

@interface XIOSView : UIView

@property (nonatomic, copy) OnProgressChangeCallback progressChangeCallback;

- (void)start;

- (void)seekTo:(long)targetPos;

- (void)pause;

- (void)stop;

@end

#endif /* XIOSView_hpp */
