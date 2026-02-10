#include "ShaderUtils.h"
#include "raylib.h"
#include <glad/glad.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

unsigned int CompileComputeProgram(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
    {
        TraceLog(LOG_WARNING, "Cannot open compute shader: %s", path);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *src = (char *)malloc(size + 1);
    if (!src)
    {
        fclose(f);
        return 0;
    }

    fread(src, 1, size, f);
    src[size] = '\0';
    fclose(f);

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, (const char **)&src, NULL);
    glCompileShader(shader);

    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLchar log[4096];
        GLsizei len = 0;
        glGetShaderInfoLog(shader, sizeof(log), &len, log);
        TraceLog(LOG_WARNING, "Compute shader compile error: %s", log);
        glDeleteShader(shader);
        free(src);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);

    GLint linked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLchar log[4096];
        GLsizei len = 0;
        glGetProgramInfoLog(program, sizeof(log), &len, log);
        TraceLog(LOG_WARNING, "Compute program link error: %s", log);
        glDeleteProgram(program);
        free(src);
        return 0;
    }

    free(src);
    TraceLog(LOG_INFO, "Compute shader compiled successfully: %s", path);
    return program;
}
