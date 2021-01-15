# A GPU Optimizer for Blue-Noise Screen-Space Sampler

## What is this ? 

Hello there ! This application implements the optimization process described in [A Low-Discrepancy Sampler that Distributes Monte Carlo Errors as a Blue Noise in Screen Space](https://belcour.github.io/blog/research/publication/2019/06/17/sampling-bluenoise.html), Heitz et al. (2019) on the GPU (minus the ranking part that allows for progressive ). 
It produces 128 by 128 masks of 12 scramble values that can be used to scramble the owen sobol sequence provided in ```include/sobol_4096spp_256d.h```. A sampling function is provided with the optimized mask. 

In case you want to optimize a mask for more or less than those 12 values, feel free to edit the constant ```D``` set in ```include/optimizer.hpp``` (just make sure it remains a multiple of two).


## Requirements

The application has three requirements:
 - OpenGL 4.3 for compute shaders
 - OpenMP to speed up all the pre-computations
 - CMake 3.2 for makefiles generation

It also uses [GLFW](https://github.com/glfw/glfw) to generate an OpenGL context and [GLAD](https://github.com/Dav1dde/glad) to load the OpenGL functions. 
GLFW is included in the repository as a submodule, so make sure you get it using ```git submodule update --init``` after cloning the repository if you did not clone using the ```--recursive``` option in the first place.


## Build instructions

### On Windows (with a Visual Studio):

On any version on VS that supports CMake, just open the project directory with the IDE it's really that simple (tested on VS 2019).

### On Linux:

After cloning the repository and its dependencies (see the *Requirements* section), simply do the following in terminal while at the root of the project:
```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make 
```

And you should be good to go!


## Usage

On launch, the optimization process starts automatically and you get a preview on the state of the optimization in the GLFW window (each sequence of the mask is used to integrate the same gaussian, the normalized integration results are displayed). 
As the permutations quickly get hard to visualize, the total number of permutations that were applied is constantly updated in the terminal.

The application expects either none or a two parameters. The first parameter is the sample count you want to optimize the mask for. As the sequence used for the optimization is an owen scrambled sobol sequence, it is strongly recommended to set that parameter with a value that is a power of two such that ```0 < n <= 4096```. The default value for the sample count is 16. 
The second parameter is a threshold that must be strictly greather than zero, it is used to stop the optimization and its default value is 15. 

Therefore you can use:
```
./Optimizer 16 15
```
to launch the optimization for 16 samples per sequence and the default halt condition.

The optimization is done by pairs of dimensions. The condition that must be fulfilled to halt the optimization for a given pair of dimension is for the number of accepted permutations in a batch of 100 dispatches to be lower than the threshold (each compute shader dispatch attemps 4096 permutations). Note that the process can take several minutes (or even hours!) to complete depending on your GPU.
The application will close when the 12 dimensions are optimized and the scrambling mask (and a sampling function) is exported at the root of the project in a header file (mask.h).


## Sample result
Here is the kind of result that can be obtained with the method described in that paper at 16 samples per pixel on the *Boxed* scene (rendered with [Mitsuba](http://www.mitsuba-renderer.org)):

| Traditional whitenoise sampler                                                           | [Bluenoise sampler](https://belcour.github.io/blog/research/publication/2019/06/17/sampling-bluenoise.html) |
| ---------------------------------------------------------------------------------------- |:-----------------------------------------------------------------------------------------------------------:|
| <img src="https://i.imgur.com/GkNUQcz.png" alt="Boxed 16spp whitenoise"> | <img src="https://i.imgur.com/mOj1XTK.png" alt="Boxed 16spp bluenoise">                      |

