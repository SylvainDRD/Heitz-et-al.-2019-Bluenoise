#version 430 core


out vec4 color;

uniform sampler2D display;


void main() {
    ivec2 coords = ivec2(gl_FragCoord.xy);
    
    color = vec4(texelFetch(display, coords, 0).x);
}
