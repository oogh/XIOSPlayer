//
//  XIOSMediaCore.mm
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#import "XIOSMediaCore.hpp"
#include "XMediaCore.hpp"

static InitParams sParams;

@implementation XIOSMediaCore

#pragma mark - Public

+ (void)setup:(InitParams)params {
    sParams = params;
    XMediaCore::getInstance().setShaderPath(params.shaderPath.UTF8String);
}

+ (NSString*)getResPath {
    return sParams.resPath;
}

+ (NSString*)getShaderPath {
    return sParams.shaderPath;
}

@end
