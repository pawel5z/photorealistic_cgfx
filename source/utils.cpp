#include "utils.hpp"

#include <fstream>
#include <vector>

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

std::ostream &operator<<(std::ostream &os, const glm::vec3 &v) {
    os << v[0] << ' ' << v[1] << ' ' << v[2] << ' ' << glm::length(v);
    return os;
}
