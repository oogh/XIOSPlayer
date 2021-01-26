//
//  XIOSView.hpp
//  XIOSPlayer
//
//  Created by oogh on 2021/1/14.
//

#ifndef XIOSView_h
#define XIOSView_h

#import <UIKit/UIKit.h>
#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>

@interface XIOSView : UIView

@property GLfloat preferredRotation;

- (instancetype)initWithCoder:(NSCoder *)aDecoder;

- (void)setupGL;

- (void)displayPixelBuffer:(CVPixelBufferRef)pixelBuffer;

@end

#endif /* XIOSView_h */
