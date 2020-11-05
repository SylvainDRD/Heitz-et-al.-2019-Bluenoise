#include <display.hpp>
#include <optimizer.hpp>

#include <random>


Display::Display(GLuint displayTexture)
    : m_program(buildShaders({PROJECT_ROOT "shaders/display.vert", PROJECT_ROOT "shaders/display.frag"},
                             {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER}, {})) {
    generateScreenquad();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(m_program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, displayTexture);

    glUniform1i(glGetUniformLocation(m_program, "display"), 0);
}

void Display::freeGLRessources() {
    glDeleteVertexArrays(1, &m_screenquadVAO);
    glDeleteProgram(m_program);
}

void Display::draw() const {
    glUseProgram(m_program);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
}

void Display::generateScreenquad() {
    glGenVertexArrays(1, &m_screenquadVAO);
    glBindVertexArray(m_screenquadVAO);

    GLfloat vertices[12] = {-1.f, -1.f, 0.f, 1.f, 1.f, 0.f, -1.f, 1.f, 0.f, 1.f, -1.f, 0.f};
    GLubyte indices[6] = {0, 1, 2, 0, 3, 1};

    GLuint indexBuffer;
    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);
}
