#include <IL/il.h>
#include <IL/ilu.h>
#include <epoxy/gl.h>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

#include "utils.hpp"

void ilLogErrorStack() {
    ILenum errCode;
    while (errCode = ilGetError())
        std::cerr << iluErrorString(errCode) << '\n';
}

/**
 * >>>
 * Taken from
 * AGL3:  GL/GLFW init AGLWindow and AGLDrawable class definitions
 * Ver.3  14.I.2020 (c) A. Łukaszewski
 */
int compileShaders(GLuint &pId, GLuint v, GLuint f, GLuint g = 0);
static GLint compileLink(GLuint v, const char *which, int prog = 0);
static void getShaderSource(GLuint sId, const char *file);

int compileProgram(GLuint &pId, const char *vs, const char *fs, const char *gs) {
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint g = 0;
    if (gs)
        g = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(v, 1, &vs, nullptr); // Also read from file: next fun
    glShaderSource(f, 1, &fs, nullptr); // ...
    if (gs)
        glShaderSource(g, 1, &gs, nullptr); // ...

    int res = compileShaders(pId, v, f, g);
    glUseProgram(pId);
    return res;
}

int compileProgramFromFile(GLuint &pId, const char *vs, const char *fs, const char *gs) {
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint g = 0;
    if (gs)
        g = glCreateShader(GL_GEOMETRY_SHADER);
    getShaderSource(v, vs);
    getShaderSource(f, fs);
    if (gs)
        getShaderSource(g, gs);

    int res = compileShaders(pId, v, f, g);
    glUseProgram(pId);
    return res;
}

int compileShaders(GLuint &pId, GLuint v, GLuint f, GLuint g) {
    GLint Result = GL_FALSE;
    if (g)
        Result = compileLink(g, "GS");
    if ((Result = compileLink(v, "VS")))
        if (compileLink(f, "FS")) {
            pId = glCreateProgram();
            glAttachShader(pId, v);
            glAttachShader(pId, f);
            if (g)
                glAttachShader(pId, g);
            compileLink(pId, "Linking", 3);
        }
    glDeleteShader(v);
    glDeleteShader(f);
    if (g)
        glDeleteShader(g);
    return Result;
}

GLint compileLink(GLuint v, const char *which, int prog) {
    GLint Result = GL_FALSE;
    int InfoLogLength;
    if (prog) {
        glLinkProgram(v);
        glGetProgramiv(v, GL_LINK_STATUS, &Result);
        glGetProgramiv(v, GL_INFO_LOG_LENGTH, &InfoLogLength);
    } else {
        glCompileShader(v);
        glGetShaderiv(v, GL_COMPILE_STATUS, &Result);
        glGetShaderiv(v, GL_INFO_LOG_LENGTH, &InfoLogLength);
    }
    if (InfoLogLength > 0 && !Result) {
        std::vector<char> Message(InfoLogLength + 1);
        if (prog)
            glGetProgramInfoLog(v, InfoLogLength, nullptr, &Message[0]);
        else
            glGetShaderInfoLog(v, InfoLogLength, nullptr, &Message[0]);
        printf("%s: %s\n", which, &Message[0]);
    }
    return Result;
}

void getShaderSource(GLuint sId, const char *file) {
    std::string sCode;
    std::ifstream sStream(file, std::ios::in);
    if (sStream.is_open()) {
        std::string Line = "";
        while (getline(sStream, Line))
            sCode += "\n" + Line;
        sStream.close();
    } else {
        printf("Error opening file:  %s !\n", file);
        getchar();
        return;
    }
    char const *SourcePointer = sCode.c_str();
    glShaderSource(sId, 1, &SourcePointer, nullptr);
}
/**
 * <<<
 * Taken from
 * AGL3:  GL/GLFW init AGLWindow and AGLDrawable class definitions
 * Ver.3  14.I.2020 (c) A. Łukaszewski
 */

// >>> adapted from https://ii.uni.wroc.pl/~anl/dyd/RGK/files/triangle_moller.c
#define EPSILON .000001f

bool intersectRayTriangle(const glm::vec3 &orig, const glm::vec3 &dir, const glm::vec3 &vert0,
                          const glm::vec3 &vert1, const glm::vec3 &vert2, glm::vec2 &baryPosition,
                          float &t) {
    glm::vec3 edge1, edge2, tvec, pvec, qvec;
    float det, inv_det;

    /* find vectors for two edges sharing vert0 */
    edge1 = vert1 - vert0;
    edge2 = vert2 - vert0;

    /* begin calculating determinant - also used to calculate U parameter */
    pvec = glm::cross(dir, edge2);

    /* if determinant is near zero, ray lies in plane of triangle */
    det = glm::dot(edge1, pvec);

#ifdef TEST_CULL /* define TEST_CULL if culling is desired */
    if (det < EPSILON)
        return false;

    /* calculate distance from vert0 to ray origin */
    tvec = orig - vert0;

    /* calculate U parameter and test bounds */
    baryPosition[0] = glm::dot(tvec, pvec);
    if (baryPosition[0] < 0.f || baryPosition[0] > det)
        return false;

    /* prepare to test V parameter */
    qvec = glm::cross(tvec, edge1);

    /* calculate V parameter and test bounds */
    baryPosition[1] = glm::dot(dir, qvec);
    if (baryPosition[1] < 0.f || baryPosition[0] + baryPosition[1] > det)
        return false;

    /* calculate t, scale parameters, ray intersects triangle */
    t = glm::dot(edge2, qvec);
    inv_det = 1.f / det;
    t *= inv_det;
    u *= inv_det;
    v *= inv_det;
#else /* the non-culling branch */
    if (det > -EPSILON && det < EPSILON)
        return false;
    inv_det = 1.f / det;

    /* calculate distance from vert0 to ray origin */
    tvec = orig - vert0;

    /* calculate U parameter and test bounds */
    baryPosition[0] = glm::dot(tvec, pvec) * inv_det;
    if (baryPosition[0] < 0.f || baryPosition[0] > 1.f)
        return false;

    /* prepare to test V parameter */
    qvec = glm::cross(tvec, edge1);

    /* calculate V parameter and test bounds */
    baryPosition[1] = glm::dot(dir, qvec) * inv_det;
    if (baryPosition[1] < 0.f || baryPosition[0] + baryPosition[1] > 1.f)
        return false;

    /* calculate t, ray intersects triangle */
    t = glm::dot(edge2, qvec) * inv_det;
#endif
    return true;
}
// >>> adapted from https://ii.uni.wroc.pl/~anl/dyd/RGK/files/triangle_moller.c
