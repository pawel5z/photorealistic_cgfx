#version 330
#extension GL_ARB_explicit_uniform_location : require

in vec3 fragColor;

out vec4 color;

void main(void) {
    color = vec4(fragColor, 1.f);
}
