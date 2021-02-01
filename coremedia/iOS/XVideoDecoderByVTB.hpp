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

@interface XVideoDecoderByVTB : NSObject

- (instancetype)initWithPath:(NSString*)path;

- (int)open;

//- (CVPixelBufferRef)getPixelBuffer:(CMTime)time;

- (CVPixelBufferRef)getPixelBuffer:(long)clock;

@end

#endif /* XVideoDecoderByVTB_h */
