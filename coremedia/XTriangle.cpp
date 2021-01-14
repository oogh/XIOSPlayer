//
// Created by Oogh on 2020/10/26.
//

#include "XTriangle.hpp"

std::string XTriangle::sVertexFilePath = "/Users/oogh/Workspace/XRender/Resources/shaders/triangle.vs";
std::string XTriangle::sFragmentFilePath = "/Users/oogh/Workspace/XRender/Resources/shaders/triangle.fs";

XTriangle::XTriangle() {
    mShader = std::make_unique<XShader>(sVertexFilePath, sFragmentFilePath);

    float vertices[] = {
            -0.5f, -0.5f, 0.0f, // left
            0.5f, -0.5f, 0.0f, // right
            0.0f, 0.5f, 0.0f  // top
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
}

XTriangle::~XTriangle() {

}

void XTriangle::draw() {
    mShader->use();
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
