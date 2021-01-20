#include <chrono>
#include <thread>
#include <iomanip>

#include <utils.hpp>
#include <display.hpp>
#include <optimizer.hpp>

#include <GLFW/glfw3.h>


using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

bool handleArgs(int argc, char **argv, int &spp, int &threshold);

int main(int argc, char **argv) {
    // GLFW initialization
    if(!glfwInit()) {
        ERROR << "There was an issue during the initialization of GLFW" << std::endl;

        return GLFW_INIT_ERROR;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow *window = glfwCreateWindow(MaskSize, MaskSize, "Optimizer", nullptr, nullptr);

    if(!window) {
        ERROR << "There was an issue during the initialization of the GLFW window" << std::endl;

        glfwTerminate();
        return GLFW_WINDOW_ERROR;
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowCloseCallback(window, [](GLFWwindow *window) { glfwSetWindowShouldClose(window, GLFW_FALSE); });

    // OpenGL initialization
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        ERROR << "There was an issue loading the OpenGL function: make sure your GPU is compatible with "
                 "OpenGL 4.3."
              << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();
        return GL_LOAD_ERROR;
    }

    // Check the GPU capabilities
    GLint64 ssboMaxSize;
    glGetInteger64v(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &ssboMaxSize);
    if(ssboMaxSize < int(sizeof(GLfloat)) * DistanceMatrixSize) {
        ERROR << "Your OpenGL implementation only support SSBO of maximum size " << ssboMaxSize << ": aborting."
              << std::endl;

        return GL_SSBO_SIZE_ERROR;
    }

    int spp, threshold;
    if(!handleArgs(argc, argv, spp, threshold)) {
        ERROR << "Invalid arguments, possible usages :\n"
                 "1) ./Optimizer\n"
                 "2) ./Optimizer SampleCount Threshold\n"
                 "Note: 1 <= SampleCount <= 4096 and 1 <= Threshold"
              << std::endl;

        return INVALID_ARGUMENTS;
    }

    Optimizer optimizer(spp);
    Display display(optimizer.displayTexture());

    int dispatchCount = 0;
    int prevAcceptedSwaps = 0;
    auto start = steady_clock::now();

    while(!glfwWindowShouldClose(window)) {
        optimizer.run();
        glFinish();

        if(duration_cast<milliseconds>(steady_clock::now() - start).count() > 100) {
            display.draw();
            LOG << "Accepted permutations: " << std::setw(6) << optimizer.acceptedSwapCount() << '\r' << std::flush;

            glfwSwapBuffers(window);
            glfwPollEvents();

            start = std::chrono::steady_clock::now();
        }

        // Check if the number of swaps for the current pair of dimension is below a threshold
        if(++dispatchCount == 100) {
            int acceptedSwaps = optimizer.acceptedSwapCount();

            if(acceptedSwaps - prevAcceptedSwaps < threshold) {
                LOG << "\n\n";
                if(!optimizer.nextDimensions())
                    glfwSetWindowShouldClose(window, true);
            }

            prevAcceptedSwaps = acceptedSwaps;
            dispatchCount = 0;
        }
    }

    LOG << "Exporting the mask and cleaning up before exiting." << std::endl;

    // Save the last mask
    optimizer.exportMaskAsHeader(PROJECT_ROOT "mask.h");
    optimizer.exportMaskAsTile(PROJECT_ROOT "tile.samples");

    // Cleanup
    display.freeGLRessources();
    optimizer.freeGLRessources();

    glfwDestroyWindow(window);
    glfwTerminate();

    return SUCCESS;
}

bool handleArgs(int argc, char **argv, int &spp, int &threshold) {
    if(argc == 1) {
        spp = 16;
        threshold = 15;
    } else if(argc == 3) {
        spp = std::atoi(argv[1]);
        if(spp <= 0 || spp > 4096)
            return false;

        if(spp & (spp - 1))
            WARN << "The sample per pixel argument should be a power of two for optimal convergence." << std::endl;

        threshold = std::atoi(argv[2]);
        if(threshold <= 0) {
            WARN << "The provided threshold should be greater than 0. Using default threshold value (threshold = 15)."
                 << std::endl;
            threshold = 15;
        }
    } else
        return false;

    return true;
}
