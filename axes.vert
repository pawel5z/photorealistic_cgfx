#version 330
#extension GL_ARB_explicit_uniform_location : require
#extension GL_ARB_shading_language_420pack : require

layout(location = 0)uniform mat4 vp;

const vec3 ends[6] = {
    {0.f, 0.f, 0.f},
    {1000.f, 0.f, 0.f},
    {0.f, 0.f, 0.f},
    {0.f, 1000.f, 0.f},
    {0.f, 0.f, 0.f},
    {0.f, 0.f, 1000.f},
};

const vec3 colors[3] = {
    {1.f, 0.f, 0.f},
    {0.f, 1.f, 0.f},
    {0.f, 0.f, 1.f},
};

out vec3 fragColor;

void main(void) {
    gl_Position = vp * vec4(ends[gl_VertexID], 1.f);
    fragColor = colors[int(floor(gl_VertexID / 2.f))];
}
