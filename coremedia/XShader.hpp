//
// Created by Oogh on 2020/3/19.
//

#ifndef XSHADER_H
#define XSHADER_H

#include <string>
#include "XGLHeader.hpp"

class XShader {
public:
    XShader(std::string vertexFilePath, std::string fragmentFilePath);

    ~XShader();

    void use();

    void setBool(const std::string& name, bool value) const;

    void setInt(const std::string& name, int value) const;

    void setFloat(const std::string& name, float value) const;

private:
    void checkCompileError(GLuint shader, std::string type);

private:
    GLuint mProgram;

};

#endif // XSHADER_H
