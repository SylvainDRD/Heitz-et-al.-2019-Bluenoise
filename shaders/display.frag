#version 430 core

#define MASK_SIZE 1337


in vec2 textureCoordinates;
out vec4 color;

uniform usampler2D indices;


void main() {
    color = vec4(float(texture(indices, textureCoordinates).x) / (MASK_SIZE * MASK_SIZE - 1));
}
