//
//  XView.mm
//  XRender
//
//  Created by Oogh on 2020/04/17.
//  Copyright © 2020 Oogh. All rights reserved.
//

#import "XIOSView.hpp"
#import "XIOSViewInternal.hpp"
#include "XGLHeader.hpp"
#include "XRender.hpp"
#import "XWeakProxy.hpp"

@interface XIOSView()
{
    CAEAGLLayer* _glLayer;
    EAGLContext* _glContext;
    GLuint _frameBuffer;
    GLuint _renderBuffer;
    std::shared_ptr<XRender> _render;
    CADisplayLink* _displayLink;
}
@end

@implementation XIOSView

#pragma mark - Life Cycle
- (instancetype)initWithCoder:(NSCoder *)coder {
    self = [super initWithCoder:coder];
    if (self) {
        [self setup];
    }
    return self;
}

- (void)dealloc {
    if (_frameBuffer) {
        glDeleteFramebuffers(1, &_frameBuffer);
        _frameBuffer = 0;
    }
    
    if (_renderBuffer) {
        glDeleteRenderbuffers(1, &_renderBuffer);
        _renderBuffer = 0;
    }
    
    _glContext = nil;
}

#pragma mark - Override
+ (Class)layerClass {
    return [CAEAGLLayer class];
}

#pragma mark - Public

- (void)start {
    if (!_displayLink) {
        _displayLink = [CADisplayLink displayLinkWithTarget:[[XWeakProxy alloc] initWithTarget:self]
                                                       selector:@selector(drawFrame)];
        [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
    }
    _displayLink.paused = NO;
}

- (void)seekTo:(long)targetPos {
//    _render->seekTo(targetPos);
}

- (void)pause {
    if (_displayLink) {
        _displayLink.paused = YES;
    }
    
//    _render->pause();
}

- (void)stop {
    if (_displayLink) {
        [_displayLink invalidate];
        _displayLink = nil;
    }
}

- (std::shared_ptr<XRender>)getNativeRender {
    return _render;
}

#pragma mark - Private
- (void)setup {
    // 1. setup layer
    _glLayer = (CAEAGLLayer*) self.layer;
    _glLayer.opaque = YES;
    _glLayer.contentsScale = [UIScreen mainScreen].scale;
    _glLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                   [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
    
    // 2. setup context
    if (!_glContext) {
        _glContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
    }
    NSAssert(_glContext && [EAGLContext setCurrentContext:_glContext], @"[XView] EAGLContext error!");
    
    // 3. setup render buffer
    if (_renderBuffer) {
        glDeleteRenderbuffers(1, &_renderBuffer);
        _renderBuffer = 0;
    }
    glGenRenderbuffers(1, &_renderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
    [_glContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:_glLayer];
    
    // 4. setup frame buffer
    if (_frameBuffer) {
        glDeleteFramebuffers(1, &_frameBuffer);
        _frameBuffer = 0;
    }
    glGenFramebuffers(1, &_frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, _renderBuffer);

    // 5. setup native render
    _render = std::make_shared<XRender>();
    _render->onSurfaceCreated();
    CGFloat scale = [UIScreen mainScreen].scale;
    _render->onSurfaceChanged([self bounds].size.width * scale, [self bounds].size.height * scale);
    
    [self setupCallback];
    

    // 6. setup display link
    if (!_displayLink) {
        _displayLink = [CADisplayLink displayLinkWithTarget:[[XWeakProxy alloc] initWithTarget:self]
                                                       selector:@selector(drawFrame)];
        [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
    }
    
    [self start];
}

- (void)setupCallback {
    __weak typeof(self) weakSelf = self;
//    _render->setOnProgressChangeCallback(^(long current, long duration) {
//        __strong typeof(weakSelf) strongSelf = weakSelf;
//        dispatch_async(dispatch_get_main_queue(), ^{
//            strongSelf.progressChangeCallback(current, duration);
//        });
//    });
}

- (void)drawFrame {

    _render->onDrawFrame();

    [_glContext presentRenderbuffer:GL_RENDERBUFFER];
}

@end
