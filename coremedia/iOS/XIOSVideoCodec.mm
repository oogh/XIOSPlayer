//
//  XIOSVideoCodec.mm
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#import "XIOSVideoCodec.hpp"
#include "XWeakProxy.hpp"
#include "XLogger.hpp"

#import <AVFoundation/AVFoundation.h>

struct XVideoCodecContext {
    AVPlayer* player = nil;
    AVPlayerItem* playerItem = nil;
    AVPlayerItemVideoOutput* videoOutput = nil;
};

XIOSVideoCodec::XIOSVideoCodec() {
    mContext = std::make_shared<XVideoCodecContext>();
}

XIOSVideoCodec::~XIOSVideoCodec() {
    
}

void XIOSVideoCodec::setFilename(const std::string& filename) {
    mFilename = filename;
}

std::shared_ptr<XImage> XIOSVideoCodec::getImage(long clock) {
    CMTime time = CMTimeMakeWithSeconds(clock / 1000.0, NSEC_PER_SEC);
    LOGI("[XIOSVideoCodec] getImage clock: %.4f\n", CMTimeGetSeconds(time));
//    [mContext->player seekToTime:time
//                 toleranceBefore:kCMTimeZero
//                  toleranceAfter:kCMTimeZero];
    CMTime outputItemTime = [mContext->videoOutput itemTimeForHostTime:CACurrentMediaTime()];
    if ([mContext->videoOutput hasNewPixelBufferForItemTime:outputItemTime]) {
        CVPixelBufferRef pixelBuffer = [mContext->videoOutput copyPixelBufferForItemTime:outputItemTime itemTimeForDisplay:nullptr];
        
        if (pixelBuffer) {
            auto image = std::make_shared<XImage>();
            image->width = static_cast<int>(CVPixelBufferGetWidth(pixelBuffer));
            image->height = static_cast<int>(CVPixelBufferGetHeight(pixelBuffer));
            image->linesize[0] = static_cast<int>(CVPixelBufferGetBytesPerRow(pixelBuffer));
            CVPixelBufferLockBaseAddress(pixelBuffer, 0);
            size_t size = static_cast<size_t>(image->linesize[0] * image->height);
            image->pixels[0] = reinterpret_cast<uint8_t*>(malloc(size));
            uint8_t* src = reinterpret_cast<uint8_t*>(CVPixelBufferGetBaseAddress(pixelBuffer));
            memcpy(image->pixels[0], src, size);
            CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
            CFRelease(pixelBuffer);
            return image;
        }
    }
    return nullptr;
}

int XIOSVideoCodec::open() {
    mContext->player = [[AVPlayer alloc] init];
    
    NSDictionary* pixelBufferAttributes = @{
        (id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)
    };
    mContext->videoOutput = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:pixelBufferAttributes];
    
    AVURLAsset* asset = [AVURLAsset assetWithURL:[NSURL fileURLWithPath:[NSString stringWithCString:mFilename.data() encoding:[NSString defaultCStringEncoding]]]];
    AVMutableComposition* composition = [AVMutableComposition composition];
    [asset loadValuesAsynchronouslyForKeys:@[@"tracks", @"duration"] completionHandler:^{
        dispatch_async(dispatch_get_main_queue(), ^{
            AVAssetTrack* track = [asset tracksWithMediaType:AVMediaTypeVideo].firstObject;
            AVMutableCompositionTrack* videoTrack = [composition addMutableTrackWithMediaType:AVMediaTypeVideo preferredTrackID:kCMPersistentTrackID_Invalid];
            
            NSError* err = nullptr;
            [videoTrack insertTimeRange:CMTimeRangeMake(kCMTimeZero, asset.duration)
                                ofTrack:track
                                 atTime:kCMTimeZero
                                  error:&err];
            mContext->playerItem = [AVPlayerItem playerItemWithAsset:composition.copy];
            [mContext->playerItem addOutput:mContext->videoOutput];
            [mContext->videoOutput requestNotificationOfMediaDataChangeWithAdvanceInterval:0.03f];
            [mContext->player replaceCurrentItemWithPlayerItem:mContext->playerItem];
            [mContext->player play];
        });
    }];
    return 0;
}

int XIOSVideoCodec::close() {
    return 0;
}
