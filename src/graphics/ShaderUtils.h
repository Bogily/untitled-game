#pragma once

#include <glad/glad.h>

// Compile a compute shader from file and return OpenGL program ID (0 on failure)
unsigned int CompileComputeProgram(const char *path);
