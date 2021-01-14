//
//  XIOSMediaCore.hpp
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XIOSMediaCore_hpp
#define XIOSMediaCore_hpp

#import <Foundation/Foundation.h>

struct InitParams {
    NSString* resPath;
    NSString* shaderPath;
};

typedef struct InitParams InitParams;

@interface XIOSMediaCore : NSObject

+ (void)setup:(InitParams)params;

+ (NSString*)getResPath;

@end

#endif /* XIOSMediaCore_hpp */
