//
//  ViewController.m
//  XIOSPlayer
//
//  Created by oogh on 2021/1/14.
//

#import "ViewController.h"
#import "XVideoDecoderByVTB.hpp"
#import "XIOSView.hpp"

@interface ViewController () {
    XVideoDecoderByVTB* _decoder;
    CADisplayLink* _displayLink;
    long clock;
}
@property (weak, nonatomic) IBOutlet XIOSView *displayView;
@property (weak, nonatomic) IBOutlet UISlider *slider;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [self.displayView setupGL];
    
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(refreshDisplay:)];
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    if (@available(iOS 10.0, *)) {
//        _displayLink.preferredFramesPerSecond = 60;
    } else {
        // Fallback on earlier versions
    }
    [_displayLink setPaused:YES];
    
    NSString* filePath = [[NSBundle mainBundle] pathForResource:@"douyin_700x1240" ofType:@"mp4"];
    _decoder = [[XVideoDecoderByVTB alloc] initWithPath:filePath];
    
    self->clock = 0;
}

- (IBAction)onOpenClicked:(UIButton *)sender {
    if ([_decoder open] < 0) {
        NSLog(@"decoder open failed");
        return;
    }
    
    self.slider.maximumValue = [_decoder getDuration];
    
    [_displayLink setPaused:NO];
    
//    for (long clock = 0; clock < 352000; clock += 100) {
//        CVPixelBufferRef pixelBuffer = [_decoder getPixelBuffer:clock];
//        if (pixelBuffer) {
//            [self.displayView displayPixelBuffer:pixelBuffer];
//        }
//    }
}

- (IBAction)onProgressChanged:(UISlider *)sender {
    self->clock = (long)sender.value;
}

- (void)refreshDisplay:(CADisplayLink*)link {
    CVPixelBufferRef pixelBuffer = [_decoder getPixelBuffer:self->clock];
    if (pixelBuffer) {
        [self.displayView displayPixelBuffer:pixelBuffer];
        self.slider.value = self->clock;
    }
}

@end
