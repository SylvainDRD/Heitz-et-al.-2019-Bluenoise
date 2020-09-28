#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <regex>

#include <glad/glad.h>

#define INVALID_ARGUMENTS -1
#define GLFW_INIT_ERROR -2
#define GLFW_WINDOW_ERROR -3
#define GL_LOAD_ERROR -4

#define AT "\n    --> at " << __FILE__ << " line " << __LINE__ << std::endl
#define GL_CHECK_ERROR()                                                                                               \
    {                                                                                                                  \
        GLenum error;                                                                                                  \
        while((error = glGetError()) != GL_NO_ERROR) {                                                                 \
            std::string errorMessage;                                                                                  \
            switch(error) {                                                                                            \
            case GL_INVALID_ENUM:                                                                                      \
                errorMessage = "invalid enum";                                                                         \
                break;                                                                                                 \
            case GL_INVALID_VALUE:                                                                                     \
                errorMessage = "invalid value";                                                                        \
                break;                                                                                                 \
            case GL_INVALID_OPERATION:                                                                                 \
                errorMessage = "invalid operation";                                                                    \
                break;                                                                                                 \
            case GL_INVALID_FRAMEBUFFER_OPERATION:                                                                     \
                errorMessage = "invalid framebuffer operation";                                                        \
                break;                                                                                                 \
            }                                                                                                          \
            std::cerr << "[OGL ERROR]: " << errorMessage << AT;                                                        \
        }                                                                                                              \
    }


using uint = unsigned;

static inline void checkCompileErrors(GLuint shader) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if(!success) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

        std::string log(length, ' ');

        glGetShaderInfoLog(shader, length, nullptr, &log[0]);

        std::cerr << "[ERROR] Shader compilation failed with the following:\n" + log << std::endl;
    }
}

static inline void checkLinkingErrors(GLuint program) {
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if(!success) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

        std::string log(length, ' ');
        glGetProgramInfoLog(program, length, nullptr, &log[0]);

        std::cerr << "[ERROR] Shader linking failed with the following:\n" + log << std::endl;
    }
}

inline GLuint buildShaders(const std::vector<std::string> &filenames, const std::vector<GLenum> &types,
                           const std::vector<std::pair<std::string, GLuint>> &defines) {
    GLuint program = glCreateProgram();

    for(int i = 0; i < (int)filenames.size(); ++i) {
        std::ifstream file(filenames[i]);
        std::stringstream stream;
        stream << file.rdbuf();

        std::string src(stream.str());

        // Set the defines in the shaders' source to the correct value
        for(const auto &define : defines) {
            std::regex expr("(" + define.first + " 1337)");
            src = std::regex_replace(src, expr, define.first + " " + std::to_string(define.second));
        }

        GLint size = src.size();
        const GLchar *const csrc = src.c_str();

        GLuint shader = glCreateShader(types[i]);

        glShaderSource(shader, 1, &csrc, &size);
        glCompileShader(shader);
        checkCompileErrors(shader);

        glAttachShader(program, shader);
        glDeleteShader(shader);
    }

    glLinkProgram(program);
    checkLinkingErrors(program);

    return program;
}
