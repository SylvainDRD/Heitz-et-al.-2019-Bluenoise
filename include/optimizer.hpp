#pragma once

#include <utils.hpp>

#include <random>
#include <utility>


class Optimizer {
public:
    Optimizer(GLuint maskSize, GLuint pixelDimension);

    void run() const {
        glUseProgram(m_program);

        glCopyImageSubData(m_indexTextures.second, GL_TEXTURE_2D, 0, 0, 0, 0, m_indexTextures.first, GL_TEXTURE_2D, 0,
                           0, 0, 0, m_maskSize, m_maskSize, 1);

        // Store the xor key location/maskSize²
        std::uniform_int_distribution<uint> distribution(0, m_maskSize * m_maskSize - 1);
        glUniform1ui(glGetUniformLocation(m_program, "xorKey"), distribution(m_generator));

        glDispatchCompute(m_workGroupCount, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    GLuint indexTexture() const { return m_indexTextures.first; }

    void freeGLRessources();

    // Export the latest mask as PPM
    void exportMaskAsPPM(const char *filename) const;

    // Export the latest mask as a header
    void exportMaskAsHeader(const char *filename) const;

private:
    GLuint m_program;

    GLuint m_permutationsSSBO;

    mutable std::pair<GLuint, GLuint> m_indexTextures;

    GLuint m_distanceMatrixSSBO;

    std::vector<GLfloat> m_whitenoise;

    const GLuint m_workGroupCount;

    const GLuint m_maskSize;

    const GLuint m_pixelDimension;

    mutable std::mt19937 m_generator;

    // Precompute the permutations that will be tested by the compute shader and store them in the SSBO
    void precomputePermutations();

    void setupTextures();

    GLuint generateIndexTexture(const void *data);

    GLuint generateDistanceMatrix();
};
