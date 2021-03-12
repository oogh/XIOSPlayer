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

- (CVPixelBufferRef)getPixelBuffer:(long)clock;

- (CVPixelBufferRef)seekToTime:(long)clock;

- (int)decodeVideoFrame;

- (int)resetVTDecompressionSession;

@end

#endif /* XVideoDecoderByVTB_h */
