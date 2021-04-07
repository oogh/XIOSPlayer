//
//  ViewController.m
//  XIOSPlayer
//
//  Created by oogh on 2021/1/14.
//

#import "ViewController.h"
#import "XVideoDecoderByVTB.hpp"
#import "XIOSView.hpp"
#import <mach/mach_time.h>

@interface ViewController () {
    XVideoDecoderByVTB* _decoder;
    CADisplayLink* _displayLink;
    long clock;
    bool _useDisplayLink;
    bool _opened;
}
@property (weak, nonatomic) IBOutlet XIOSView *displayView;
@property (weak, nonatomic) IBOutlet UISlider *slider;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    _useDisplayLink = false;
    [self.displayView setupGL];
    
    if (_useDisplayLink) {
        _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(refreshDisplay:)];
        [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        if (@available(iOS 10.0, *)) {
            _displayLink.preferredFramesPerSecond = 60;
        } else {
            // Fallback on earlier versions
        }
        [_displayLink setPaused:YES];
    }
    _opened = false;
    
//    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"douyin_700x1240" ofType:@"mp4"];
//    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"xsmax_heiping" ofType:@"mp4"];
//    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"4k_1" ofType:@"mp4"];
//    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"4k" ofType:@"mp4"];
//    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"IMG_5685" ofType:@"MOV"];
//    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"kadun" ofType:@"mp4"];
    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"4k" ofType:@"mov"];
//    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"zhangwenhong" ofType:@"mp4"];
//    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"1080P" ofType:@"mp4"];
//    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"7p_4k" ofType:@"mov"];
//    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"2k" ofType:@"mp4"];
//    _decoder = [[XVideoDecoderByVTB alloc] initWithPath:filePath];
//    if (!_opened) {
//        if ([_decoder open] < 0) {
//            NSLog(@"decoder open failed");
//            return;
//        }
//        self.slider.maximumValue = [_decoder getDuration];
//        _opened = true;
//    }
//
//    self->clock = 0;
}

- (IBAction)onOpenClicked:(UIButton *)sender {
//    if (!_opened) {
//        if ([_decoder open] < 0) {
//            NSLog(@"decoder open failed");
//            return;
//        }
//        self.slider.maximumValue = [_decoder getDuration];
//        _opened = true;
//    }
//
////    if (_useDisplayLink) {
////        [_displayLink setPaused:NO];
////    } else {
//        [_decoder decodeVideoFrame];
////        for (long clock = 0; clock < 352000; clock += 100) {
////            CVPixelBufferRef pixelBuffer = [_decoder getPixelBuffer:clock];
////            if (pixelBuffer) {
////                [self.displayView displayPixelBuffer:pixelBuffer];
////                CVPixelBufferRelease(pixelBuffer);
////            }
////        }
////    }
}

- (IBAction)onProgressChanged:(UISlider *)sender {
//    self->clock = (long)sender.value;
//    NSLog(@"andy seek to %ld", self->clock);
////    [_decoder seekTo:self->clock];
//    CVPixelBufferRef pixelBuffer = [_decoder seekToTime:clock];
//    if (pixelBuffer) {
//        [self.displayView displayPixelBuffer:pixelBuffer];
//        CVPixelBufferRelease(pixelBuffer);
//    }
}

- (IBAction)onSeekToClicked:(UIButton *)sender {
    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"douyin_700x1240" ofType:@"mp4"];
    auto decoder = std::make_unique<XVideoDecoderByVTB>(filePath.UTF8String);
    int ret = decoder->open();

    for (long clock = 0; clock < 50000; clock += 40) {
        
        
        auto image = decoder->getImage(ret, clock);
        
        if (image) {
            CVPixelBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(image->sampleBuffer);
            if (pixelBuffer) {
                CFAbsoluteTime startTime = CFAbsoluteTimeGetCurrent();
                [self.displayView displayPixelBuffer:pixelBuffer];
                CFAbsoluteTime duration = (CFAbsoluteTimeGetCurrent() - startTime);
//                NSLog(@"andy clock: %ld, pts: %ld, run: %lf", clock, image->pts, duration * 1000.0);
            }
            
        } else {
            NSLog(@"andy clock: %ld, image: nullptr", clock);
        }
    }
}


- (void)refreshDisplay:(CADisplayLink*)link {
//    CVPixelBufferRef pixelBuffer = [_decoder getPixelBuffer:self->clock];
//    if (pixelBuffer) {
//        [self.displayView displayPixelBuffer:pixelBuffer];
//        self.slider.value = self->clock;
//    }
}

@end
