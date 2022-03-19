#version 330
#extension GL_ARB_explicit_uniform_location : require
#extension GL_ARB_shading_language_420pack : require

layout(location = 0)uniform mat4 vp;

layout(location = 0)in vec3 vPosWSpace;

void main(void) {
    gl_Position = vp * vec4(vPosWSpace, 1);
}
