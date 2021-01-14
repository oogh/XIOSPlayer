//
//  XIOSViewInternal.hpp
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XIOSView_Internal_hpp
#define XIOSView_Internal_hpp

#import "XIOSView.hpp"
#include "XRender.hpp"

@interface XIOSView()
- (std::shared_ptr<XRender>)getNativeRender;
@end

#endif /* XIOSView_Internal_hpp */
