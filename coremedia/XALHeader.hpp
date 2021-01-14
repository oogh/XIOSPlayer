//
// Created by Oogh on 2020/8/14.
//

#ifndef XALHEADER_HPP
#define XALHEADER_HPP

#include "XPlatform.hpp"

#if PLATFORM_IOS || PLATFORM_MAC
#include <OpenAL/OpenAL.h>
#elif PLATFORM_ANDROID
#include <AL/al.h>
#include <AL/alc.h>
#endif

extern const char* alError2Str(ALenum error);


#endif // XALHEADER_HPP
