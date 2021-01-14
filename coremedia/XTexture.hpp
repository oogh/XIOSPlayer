//
// Created by Oogh on 2020/11/1.
//

#ifndef XTEXTURE_HPP
#define XTEXTURE_HPP

#include "XShader.hpp"

class XTexture {
public:
    XTexture(int id, int width, int height);

    ~XTexture();

    bool drawable() const;

    void create();

    int getId() const;

    void setPixels(uint8_t* pixels, int width, int height);

    void draw();

private:
    static std::string sVertexFilePath;
    static std::string sFragmentFilePath;

private:
    int mId;
    bool mDrawable;

    std::unique_ptr<XShader> mShader;
    GLuint VBO;
    GLuint VAO;
    GLuint EBO;
    GLuint mTexture;
    
    uint8_t* mPixels;
    int mWidth;
    int mHeight;
};


#endif // XTEXTURE_HPP
