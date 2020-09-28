#version 430 core

in vec3 position;
out vec2 textureCoordinates;


void main() {
    gl_Position = vec4(position, 1.f);

    textureCoordinates = (position.xy + 1.f) * 0.5f;
}
