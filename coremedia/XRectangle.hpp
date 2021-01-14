//
// Created by Oogh on 2020/10/31.
//

#ifndef XRENDER_XRECTANGLE_HPP
#define XRENDER_XRECTANGLE_HPP

#include "XShader.hpp"

class XRectangle {
public:
    XRectangle();

    ~XRectangle();

    void draw();

private:
    static std::string sVertexFilePath;
    static std::string sFragmentFilePath;

private:
    std::unique_ptr<XShader> mShader;
    GLuint VBO;
    GLuint VAO;
    GLuint EBO;
};


#endif //XRENDER_XRECTANGLE_HPP
