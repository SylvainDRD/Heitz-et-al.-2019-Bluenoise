#pragma once

#include <utils.hpp>


class Display {
public:
    Display(uint maskSize, uint pixelDimension, GLuint indexTexture);

    // Display texture
    void draw() const {
        glUseProgram(m_program);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
    }

    void freeGLRessources();

private:
    GLuint m_program;

    GLuint m_screenquadVAO;

    void generateScreenquad();
};
