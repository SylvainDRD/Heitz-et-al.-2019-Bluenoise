#include <chrono>
#include <thread>

#include <utils.hpp>
#include <display.hpp>
#include <optimizer.hpp>

#include <GLFW/glfw3.h>


using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::steady_clock;

const uint maskSize = 128;

bool handleArgs(uint &pixelDimension, int argc, char **argv);

int main(int argc, char **argv) {
    uint pixelDimension;

    if(!handleArgs(pixelDimension, argc, argv)) {
        std::cerr << "[ERROR] Invalid arguments, possible usages:\n"
                     "1) ./BluenoiseSequenceMaskOptimzer\n"
                     "2) ./BluenoiseSequenceMaskOptimzer Dimension\n"
                     "Note that Dimension maximum value is 12"
                  << std::endl;

        return INVALID_ARGUMENTS;
    }

    // GLFW initialization
    if(!glfwInit()) {
        std::cerr << "[ERROR] There was an issue during the initialization of GLFW" << std::endl;

        return GLFW_INIT_ERROR;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow *window = glfwCreateWindow(maskSize, maskSize, "Bluenoise Mask Optimizer", nullptr, nullptr);

    if(!window) {
        std::cerr << "[ERROR] There was an issue during the initialization of the GLFW window" << std::endl;

        glfwTerminate();
        return GLFW_WINDOW_ERROR;
    }

    glfwMakeContextCurrent(window);

    // OpenGL initialization
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[ERROR] There was an issue loading the OpenGL function: make sure your GPU is compatible with "
                     "OpenGL 4.3."
                  << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();
        return GL_LOAD_ERROR;
    }

    Optimizer optimizer(maskSize, pixelDimension);
    Display display(maskSize, pixelDimension, optimizer.indexTexture());

    auto start = steady_clock::now();
    while(!glfwWindowShouldClose(window)) {
        optimizer.run();

        // 16,000 µs <=> maximum 60 fps
        if(duration_cast<microseconds>(steady_clock::now() - start).count() > 16000) {
            display.draw();

            glfwSwapBuffers(window);
            glfwPollEvents();

            start = std::chrono::steady_clock::now();
        }
        // Avoids window freezes
        std::this_thread::sleep_for(microseconds(1));
    }

    // Save the last mask
    optimizer.exportMaskAsPPM(PROJECT_ROOT "mask.ppm");
    optimizer.exportMaskAsHeader(PROJECT_ROOT "mask.h");

    // Cleanup
    display.freeGLRessources();
    optimizer.freeGLRessources();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}


bool handleArgs(uint &pixelDimension, int argc, char **argv) {
    switch(argc) {
    case 1:
        pixelDimension = 3;

        return true;

    case 2:
        pixelDimension = std::atoi(argv[1]);

        return pixelDimension <= 12;

    default:
        return false;
    }
}
