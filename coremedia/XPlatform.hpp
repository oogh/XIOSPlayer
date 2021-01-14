//
//  XPlatform.h
//  XRender
//
//  Created by Oogh on 2020/07/16.
//  Copyright © 2020 Oogh. All rights reserved.
//

#ifndef XEXPORTER_XPLATFORM_H
#define XEXPORTER_XPLATFORM_H

#ifdef _WIN32
   //define something for Windows (32-bit and 64-bit, this part is common)
   #ifdef _WIN64
      //define something for Windows (64-bit only)
   #else
      //define something for Windows (32-bit only)
   #endif
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR
        #define PLATFORM_IOS 1
        #define PLATFORM_IOS_SIMULATOR 1
    #elif TARGET_OS_IPHONE
        #define PLATFORM_IOS 1
        #define PLATFORM_IOS_IPHONE 1
    #elif TARGET_OS_MAC
        #define PLATFORM_MAC 1
    #else
    #   error "Unknown Apple platform"
    #endif
#elif __ANDROID__
    #define PLATFORM_ANDROID 1
#elif __linux__
    // linux
#elif __unix__ // all unices not caught above
    // Unix
#elif defined(_POSIX_VERSION)
    // POSIX
#else
#   error "Unknown compiler"
#endif

#ifndef PLATFORM_MAC
#define PLATFORM_MAC 0
#endif

#ifndef PLATFORM_IOS
#define PLATFORM_IOS 0
#endif

#ifndef PLATFORM_ANDROID
#define PLATFORM_ANDROID 0
#endif


#endif //XEXPORTER_XPLATFORM_H
