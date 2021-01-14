//
//  XIOSTrackInternal.hpp
//  XRender
//
//  Created by Oogh on 2020/12/18.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XIOSTrack_Internal_hpp
#define XIOSTrack_Internal_hpp

#import "XIOSTrack.hpp"
#include "XTrack.hpp"

@interface XIOSTrack()
- (std::shared_ptr<XTrack>)getNativeTrack;
@end

#endif /* XIOSTrack_Internal_hpp */
