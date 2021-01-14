//
//  XIOSView.hpp
//  XIOSPlayer
//
//  Created by Andy on 2021/1/14.
//

#ifndef XIOSView_h
#define XIOSView_h

#import <UIKit/UIKit.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

@interface XIOSView : UIView

@property GLfloat preferredRotation;
@property CGSize presentationRect;
@property GLfloat chromaThreshold;
@property GLfloat lumaThreshold;

- (void)setupGL;
- (void)displayPixelBuffer:(CVPixelBufferRef)pixelBuffer;

@end

#endif /* XIOSView_h */
