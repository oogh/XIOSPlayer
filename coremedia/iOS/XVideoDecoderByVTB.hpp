//
//  XVideoDecoderByVTB.hpp
//  XIOSPlayer
//
//  Created by oogh on 2021/1/14.
//

#ifndef XVideoDecoderByVTB_h
#define XVideoDecoderByVTB_h

#import <Foundation/Foundation.h>
#import <CoreMedia/CoreMedia.h>

typedef void(^OnOpenBlock)(long duration);

@interface XVideoDecoderByVTB : NSObject

- (instancetype)initWithPath:(NSString*)path;

- (int)open;

- (long)getDuration;

//- (CVPixelBufferRef)getPixelBuffer:(CMTime)time;

- (CVPixelBufferRef)getPixelBuffer:(long)clock;

@end

#endif /* XVideoDecoderByVTB_h */
