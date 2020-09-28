# Bluenoise Mask Optimizer

## What is this ? 

Hello there ! 

This application's purpose is to generate tileable bluenoise masks with up to 12 dimensions on the GPU to speed up the optimization process. The function minimized to produce those masks comes from the research paper *Blue-noise Dithered Sampling*, Iliyan Georgiev & Marcos Fajardo (2016, https://www.arnoldrenderer.com/research/dither_abstract.pdf). Note that the maximum dimension of the masks can trivially be increased but the quality achieved by this optimization in 12D is already limited.

## Requirements

The application has two simple requirements:
 - OpenGL 4.3 
 - CMake 3.2 

It uses GLFW to generate an OpenGL context. It is included in the repository as a submodule, so make sure you get it using *git submodule update --init* after cloning.

## Usage

On launch, the optimization process starts automatically and you get a feedback on the current state of your mask in the GLFW window. Note that, obviously, only the first 3 channels are displayed.

When the window is closed, the optimization stops and the last version of the mask is saved as a .ppm file at the root of the project (mask.ppm).
