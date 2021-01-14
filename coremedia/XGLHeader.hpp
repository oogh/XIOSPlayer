//
// Created by Oogh on 2020/3/19.
//

#ifndef ANDROIDDEMO_XGLHEADER_H
#define ANDROIDDEMO_XGLHEADER_H

#include "XPlatform.hpp"

#if PLATFORM_ANDROID
    #include <GLES3/gl3.h>
    #include <GLES3/gl3ext.h>
#elif PLATFORM_IOS
    #import <OpenGLES/ES3/gl.h>
#elif PLATFORM_MAC
    #include <GLFW/glfw3.h>
    #include <glad/glad.h>
#endif

#endif //ANDROIDDEMO_XGLHEADER_H
