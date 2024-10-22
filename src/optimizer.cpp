#include <optimizer.hpp>
#include <sobol_4096spp_256d.h>

#include <omp.h>


// Constants definition
constexpr int HeavisideCount = 1024;
constexpr int SwapAttemptsDivisor = 2; // Swap attempts count = Pixel count / (2 * swapAttemptsDivisor)
constexpr int WorkGroupCount = PixelCount / (32 * 2 * SwapAttemptsDivisor);

Optimizer::Optimizer(int spp)
    : m_program(buildShaders({PROJECT_ROOT "shaders/optimizer.comp"}, {GL_COMPUTE_SHADER},
                             {{"D", D}, {"MASK_SIZE", MaskSize}})),
      m_scrambles(D * PixelCount), m_spp(spp) {
    LOG << "Initializing the optimizer..." << std::endl;

    m_generator.seed(std::random_device{}());

    generatePermutationsSSBO();
    generateAtomicCounter();
    setupTextures();
}

void Optimizer::freeGLRessources() {
    glDeleteBuffers(1, &m_permutationsSSBO);
    glDeleteBuffers(1, &m_distanceMatrixSSBO);
    glDeleteBuffers(1, &m_atomicCounter);
    glDeleteTextures(1, &m_scramblesIn);
    glDeleteTextures(1, &m_scramblesOut);
    glDeleteTextures(1, &m_displayIn);
    glDeleteTextures(1, &m_displayOut);
    glDeleteProgram(m_program);
}

void Optimizer::run() const {
    glUseProgram(m_program);

    std::uniform_int_distribution<uint> distribution(0, MaskSize - 1);
    glUniform2i(glGetUniformLocation(m_program, "permutationScramble"), distribution(m_generator), distribution(m_generator));
    glDispatchCompute(WorkGroupCount, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

    glCopyImageSubData(m_scramblesOut, GL_TEXTURE_2D, 0, 0, 0, 0, m_scramblesIn, GL_TEXTURE_2D, 0, 0, 0, 0, MaskSize,
                       MaskSize, 1);
    glCopyImageSubData(m_displayOut, GL_TEXTURE_2D, 0, 0, 0, 0, m_displayIn, GL_TEXTURE_2D, 0, 0, 0, 0, MaskSize,
                       MaskSize, 1);
}

bool Optimizer::nextDimensions() {
    // Dump the scramble values in a buffer to export them later
    std::vector<GLuint> scrambles(4 * PixelCount);
    glBindTexture(GL_TEXTURE_2D, m_scramblesIn);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, scrambles.data());

    for(int i = 0; i < PixelCount; ++i) {
        m_scrambles[i * D + m_dimension] = scrambles[4 * i];
        m_scrambles[i * D + m_dimension + 1] = scrambles[4 * i + 1];
    }

    m_dimension += 2;
    if(m_dimension < D) {
        setupTextures();

        return true;
    }

    return false;
}

uint32_t Optimizer::acceptedSwapCount() const {
    GLuint swapCounter;

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounter);
    GLuint *ptr = (GLuint *)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
    swapCounter = *ptr;
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

    return (uint32_t)swapCounter;
}

GLuint Optimizer::displayTexture() const { return m_displayIn; }

void Optimizer::exportMaskAsHeader(const char *filename) const {
    std::ofstream file;
    file.open(filename);

    std::uniform_int_distribution<uint32_t> distribution;

    file << "#pragma once\n\n";
    file << "#include \"sobol_4096spp_256d.h\"\n\n\n";

    // Dump the scrambling keys
    file << "static const uint32_t scramblingKeys[" << MaskSize << "][" << MaskSize << "][" << 256 << "] = {\n";
    for(int i = 0; i < MaskSize; ++i) {
        file << "    {";
        for(int j = 0; j < MaskSize; ++j) {
            file << "{";

            int index = D * (i * MaskSize + j);
            for(int d = 0; d < D; ++d) {
                file << m_scrambles[index + d] << 'U';

                if(d != TotalD - 1)
                    file << ", ";
            }

            for(int d = D; d < TotalD; ++d) {
                file << distribution(m_generator) << 'U';

                if(d != TotalD - 1)
                    file << ", ";
            }
            file << "}";

            if(j != MaskSize - 1)
                file << ", ";
        }
        file << "}";

        if(i != MaskSize - 1)
            file << ",\n";
    }
    file << "\n};\n\n\n";

    // Dump the sampling function
    file << "float sample(int i, int j, int sampleID, int d) {\n";
    file << "    i = i & " << (MaskSize - 1) << ";\n";
    file << "    j = j & " << (MaskSize - 1) << ";\n";
    file << "    d = d & " << TotalD - 1 << ";\n\n";
    file << "    uint32_t scramble = scramblingKeys[i][j][d];\n";
    file << "    uint32_t sample = sequence[sampleID][d] ^ scramble;\n\n";
    file << "    return (sample + 0.5f) / " << (1ULL << 32) << "ULL;\n";
    file << "}\n\n";
}

void Optimizer::generatePermutationsSSBO() {
    const uint permutationArraySize = PixelCount / SwapAttemptsDivisor;

    std::vector<GLuint> permutations(PixelCount);

    for(uint i = 0; i < PixelCount; ++i)
        permutations[i] = i;

    for(uint i = 0; i < permutationArraySize; ++i) {
        std::uniform_int_distribution<uint> distribution(i, PixelCount - 1);

        std::swap(permutations[i], permutations[distribution(m_generator)]);
    }

    glGenBuffers(1, &m_permutationsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_permutationsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * permutationArraySize, permutations.data(), GL_STATIC_DRAW);

    GLuint blockID = glGetProgramResourceIndex(m_program, GL_SHADER_STORAGE_BLOCK, "SwapData");
    glShaderStorageBlockBinding(m_program, blockID, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_permutationsSSBO);
}

void Optimizer::generateAtomicCounter() {
    GLuint counter = 0;

    glGenBuffers(1, &m_atomicCounter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_atomicCounter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &counter, GL_DYNAMIC_READ);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, m_atomicCounter);
}

void Optimizer::setupTextures() {
    LOG << "Dimensions " << m_dimension + 1 << " and " << m_dimension + 2 << " out of " << D << ":\n";
    LOG << "Generating the scramble values... " << std::endl;
    std::vector<GLuint> scrambles(4 * PixelCount);

    std::uniform_int_distribution<GLuint> distribution;
    for(int i = 0; i < PixelCount; ++i) {
        scrambles[4 * i] = distribution(m_generator);
        scrambles[4 * i + 1] = distribution(m_generator);
        scrambles[4 * i + 2] = i;  // Index of the sequence to access the distance matrix
        scrambles[4 * i + 3] = 0U; // Padding
    }

    LOG << "Pre-integrating the heavisides and the display gaussian..." << std::endl;
    generateDistanceMatrix(scrambles.data());

    std::vector<GLfloat> result = preintegrateDisplay(scrambles.data());

    // Create the textures if they were never created
    // Else just update their content
    if(m_dimension == 0) {
        m_scramblesIn =
            generateTexture(GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, 0, GL_READ_ONLY, scrambles.data());
        m_scramblesOut =
            generateTexture(GL_RGBA32UI, GL_RGBA_INTEGER, GL_UNSIGNED_INT, 1, GL_WRITE_ONLY, scrambles.data());
        m_displayIn = generateTexture(GL_R32F, GL_RED, GL_FLOAT, 2, GL_READ_ONLY, result.data());
        m_displayOut = generateTexture(GL_R32F, GL_RED, GL_FLOAT, 3, GL_WRITE_ONLY, result.data());
    } else {
        glBindTexture(GL_TEXTURE_2D, m_scramblesIn);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, MaskSize, MaskSize, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT,
                     scrambles.data());
        glBindTexture(GL_TEXTURE_2D, m_scramblesOut);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, MaskSize, MaskSize, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT,
                     scrambles.data());

        glBindTexture(GL_TEXTURE_2D, m_displayIn);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, MaskSize, MaskSize, 0, GL_RED, GL_FLOAT, result.data());
        glBindTexture(GL_TEXTURE_2D, m_displayOut);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, MaskSize, MaskSize, 0, GL_RED, GL_FLOAT, result.data());
    }
}

GLuint Optimizer::generateTexture(GLenum internal_format, GLenum format, GLenum data_type, int image_unit,
                                  GLenum access, const void *data) const {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, MaskSize, MaskSize, 0, format, data_type, data);

    glBindImageTexture(image_unit, texture, 0, GL_FALSE, 0, access, internal_format);

    return texture;
}

GLfloat squaredL2Norm(GLfloat *v1, GLfloat *v2, int dimension) {
    GLfloat l2sq = 0.f;

    for(int i = 0; i < dimension; ++i) {
        GLfloat diff = v1[i] - v2[i];
        l2sq += diff * diff;
    }

    return l2sq;
}

void Optimizer::generateDistanceMatrix(GLuint *scrambles) {
    std::uniform_real_distribution<GLfloat> distribution;

    // A rotation vector + a point
    std::vector<Heaviside> heavisides(HeavisideCount);

    const float PI = 3.14159265359f;
    for(int i = 0; i < HeavisideCount; ++i) {
        float theta = 2 * PI * distribution(m_generator);

        heavisides[i].nx = std::cos(theta);
        heavisides[i].ny = std::sin(theta);
        heavisides[i].px = distribution(m_generator);
        heavisides[i].py = distribution(m_generator);
    }

    std::vector<float> estimates(PixelCount * HeavisideCount);
    std::vector<GLfloat> distanceMatrix(DistanceMatrixSize);

#pragma omp parallel
    {
#pragma omp for
        for(int i = 0; i < PixelCount; ++i) {
            int scrambleIndex = 4 * i;

            for(int j = 0; j < HeavisideCount; ++j) {
                int index = i * HeavisideCount + j;
                estimates[index] = integrateHeaviside(&scrambles[scrambleIndex], heavisides[j]);
            }
        }


#pragma omp for schedule(dynamic)
        for(int i = 0; i < PixelCount; ++i) {
            for(int j = i; j < PixelCount; ++j) {
                GLuint offset_i = i * HeavisideCount;
                GLuint offset_j = j * HeavisideCount;
                GLfloat distance =
                    squaredL2Norm(estimates.data() + offset_i, estimates.data() + offset_j, HeavisideCount);

                GLuint index = j + i * PixelCount - i * (i + 1) / 2;

                distanceMatrix[index] = distance; 
            }
        }
    }

    // Generate the buffer if it is was not initialized before
    if(m_dimension == 0) {
        glGenBuffers(1, &m_distanceMatrixSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_distanceMatrixSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLfloat) * DistanceMatrixSize, distanceMatrix.data(),
                     GL_STATIC_DRAW);

        GLuint blockID = glGetProgramResourceIndex(m_program, GL_SHADER_STORAGE_BLOCK, "DistanceData");
        glShaderStorageBlockBinding(m_program, blockID, 1);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_distanceMatrixSSBO);
    } else {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_distanceMatrixSSBO);

        GLfloat *buffer = (GLfloat *)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
        std::memcpy(buffer, distanceMatrix.data(), sizeof(GLfloat) * DistanceMatrixSize);

        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
}

std::vector<GLfloat> Optimizer::preintegrateDisplay(GLuint *scrambling) const {
    std::vector<GLfloat> result(PixelCount);

    double variance = 0.0;
    float Div = 1.f / (1ULL << 32);
    for(int i = 0; i < PixelCount; ++i) {
        double sum = 0.0;

        for(int j = 0; j < m_spp; ++j) {
            float x = ((sequence[j][m_dimension] ^ scrambling[4 * i]) + 0.5f) * Div;
            float y = ((sequence[j][m_dimension + 1] ^ scrambling[4 * i + 1]) + 0.5f) * Div;
            sum += std::exp(-x * x - y * y);
        }

        result[i] = float(sum / m_spp - 0.5577462854);
        variance += double(result[i] * result[i]);
    }
    variance /= PixelCount;
    float stddev = float(std::sqrt(variance));

    // Set the standard deviation to 1/4
    for(int i = 0; i < PixelCount; ++i)
        result[i] = result[i] / (4 * stddev) + 0.5f;

    return result;
}

float Optimizer::integrateHeaviside(GLuint scramble[2], const Heaviside &heaviside) const {
    const float SampleWeight = 1.f / m_spp;
    const float Div = 1.f / (1ULL << 32);

    double sum = 0.f;
    for(int k = 0; k < m_spp; ++k) {
        double sample[2] = {((sequence[k][m_dimension] ^ scramble[0]) + 0.5) * Div,
                            ((sequence[k][m_dimension + 1] ^ scramble[1]) + 0.5) * Div};

        double v[2] = {sample[0] - heaviside.px, sample[1] - heaviside.py};

        double eval = (v[0] * heaviside.nx + v[1] * heaviside.ny < 0.f ? 1.f : 0.f);

        sum += eval;
    }

    return float(sum * SampleWeight);
}
