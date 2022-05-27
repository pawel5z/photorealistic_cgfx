#version 330
#extension GL_ARB_explicit_uniform_location : require

layout(location = 1)uniform vec3 vertexColor;

in vec3 n;

out vec4 color;

void main(void) {
    color = vec4((normalize(n) / 2.f + 0.5f) * vertexColor, 1.f);
}
