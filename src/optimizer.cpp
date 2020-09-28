#include <optimizer.hpp>


// Swap attempts count = Pixel count / (2 * swapAttemptsDivisor)
const uint swapAttemptsDivisor = 2;

Optimizer::Optimizer(GLuint maskSize, GLuint pixelDimension)
    : m_program(buildShaders({PROJECT_ROOT "shaders/optimizer.comp"}, {GL_COMPUTE_SHADER}, {{"MASK_SIZE", maskSize}})),
      m_whitenoise(maskSize * maskSize * pixelDimension),
      m_workGroupCount(maskSize * maskSize / (2 * swapAttemptsDivisor)), m_maskSize(maskSize),
      m_pixelDimension(pixelDimension) {
    std::random_device rd;
    m_generator.seed(rd());

    precomputePermutations();
    setupTextures();
}

void Optimizer::freeGLRessources() {
    glDeleteBuffers(1, &m_permutationsSSBO);
    glDeleteTextures(1, &m_indexTextures.first);
    glDeleteTextures(1, &m_indexTextures.second);
    glDeleteTextures(1, &m_distanceMatrixSSBO);
    glDeleteProgram(m_program);
}

void Optimizer::exportMaskAsPPM(const char *filename) const {
    GLuint *indices = new GLuint[m_maskSize * m_maskSize];

    glBindTexture(GL_TEXTURE_2D, m_indexTextures.first);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, indices);

    std::ofstream file;
    file.open(filename, std::ios_base::binary);
    file << "P6" << '\n';
    file << m_maskSize << ' ' << m_maskSize << '\n';
    file << "255" << '\n';

    for(uint i = 0; i < m_maskSize * m_maskSize; ++i) {
        unsigned char pixel[3];

        if(m_pixelDimension == 1) {
            pixel[0] = m_whitenoise[indices[i] * m_pixelDimension] * 255;
            pixel[1] = m_whitenoise[indices[i] * m_pixelDimension] * 255;
            pixel[2] = m_whitenoise[indices[i] * m_pixelDimension] * 255;
        } else {
            pixel[0] = m_whitenoise[indices[i] * m_pixelDimension] * 255;
            pixel[1] = m_whitenoise[indices[i] * m_pixelDimension + 1] * 255;
            pixel[2] = m_whitenoise[indices[i] * m_pixelDimension + 2] * 255;
        }

        file.write(reinterpret_cast<const char *>(&pixel), 3);
    }

    delete[] indices;
}

void Optimizer::exportMaskAsHeader(const char *filename) const {
    GLuint *mask = new GLuint[m_maskSize * m_maskSize];

    glBindTexture(GL_TEXTURE_2D, m_indexTextures.first);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, mask);

    // std::ofstream file;
    // file.open(filename);

    for(uint i = 0; i < m_maskSize * m_maskSize; ++i) {
    }

    delete[] mask;
}

void Optimizer::precomputePermutations() {
    const uint pixelCount = m_maskSize * m_maskSize;
    const uint permutationArrayEffectiveSize = pixelCount / swapAttemptsDivisor;

    GLuint *permutations = new GLuint[pixelCount];

    for(uint i = 0; i < pixelCount; ++i)
        permutations[i] = i;

    for(uint i = 0; i < permutationArrayEffectiveSize; ++i) {
        std::uniform_int_distribution<uint> distribution(i, pixelCount - 1);

        std::swap(permutations[i], permutations[distribution(m_generator)]);
    }

    glGenBuffers(1, &m_permutationsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_permutationsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * permutationArrayEffectiveSize, permutations,
                 GL_STATIC_DRAW);

    GLuint blockID = glGetProgramResourceIndex(m_program, GL_SHADER_STORAGE_BLOCK, "SwapData");
    glShaderStorageBlockBinding(m_program, blockID, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_permutationsSSBO);

    delete[] permutations;
}

void Optimizer::setupTextures() {
    const uint pixelCount = m_maskSize * m_maskSize;

    GLuint *data = new GLuint[pixelCount];

    for(uint i = 0; i < pixelCount; ++i)
        data[i] = i;

    m_indexTextures.first = generateIndexTexture(nullptr);
    glBindImageTexture(0, m_indexTextures.first, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);

    m_indexTextures.second = generateIndexTexture(data);
    glBindImageTexture(1, m_indexTextures.second, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);

    delete[] data;

    m_distanceMatrixSSBO = generateDistanceMatrix();
}

GLuint Optimizer::generateIndexTexture(const void *data) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, m_maskSize, m_maskSize, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, data);

    return texture;
}

GLfloat squaredL2Norm(GLfloat *v1, GLfloat *v2, GLuint dimension) {
    GLfloat l2sq = 0.f;

    for(GLuint i = 0; i < dimension; ++i) {
        GLfloat diff = v1[i] - v2[i];
        l2sq += diff * diff;
    }

    return l2sq;
}

GLuint Optimizer::generateDistanceMatrix() {
    const uint pixelCount = m_maskSize * m_maskSize;

    std::uniform_real_distribution<GLfloat> distribution;
    for(uint i = 0; i < pixelCount * m_pixelDimension; ++i)
        m_whitenoise[i] = distribution(m_generator);

    GLfloat *distanceMatrix = new GLfloat[pixelCount * pixelCount];

    for(uint i = 0; i < pixelCount; ++i) {
        for(uint j = i; j < pixelCount; ++j) {
            GLfloat distance = squaredL2Norm(m_whitenoise.data() + i * m_pixelDimension,
                                             m_whitenoise.data() + j * m_pixelDimension, m_pixelDimension);

            distanceMatrix[i * pixelCount + j] = distance;
            distanceMatrix[j * pixelCount + i] = distance;
        }
    }

    GLuint distanceMatrixSSBO;
    glGenBuffers(1, &distanceMatrixSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, distanceMatrixSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLfloat) * pixelCount * pixelCount, distanceMatrix,
                 GL_STATIC_DRAW);

    GLuint blockID = glGetProgramResourceIndex(m_program, GL_SHADER_STORAGE_BLOCK, "DistanceData");
    glShaderStorageBlockBinding(m_program, blockID, 1);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, distanceMatrixSSBO);

    delete[] distanceMatrix;

    return distanceMatrixSSBO;
}
