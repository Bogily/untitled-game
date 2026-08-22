#include "GlLoader.h"
#include <glad/glad.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

typedef PROC(WINAPI *PFNWGLGETPROCADDRESSPROC)(LPCSTR);

static void *GetAnyGLFuncAddress(const char *name)
{
    static HMODULE opengl32Module = LoadLibraryA("opengl32.dll");
    if (!opengl32Module)
        return nullptr;

    static PFNWGLGETPROCADDRESSPROC pfnWglGetProcAddress =
        (PFNWGLGETPROCADDRESSPROC)GetProcAddress(opengl32Module, "wglGetProcAddress");

    void *p = nullptr;
    if (pfnWglGetProcAddress)
        p = (void *)pfnWglGetProcAddress(name);

    if (p == nullptr || (p == (void *)0x1) || (p == (void *)0x2) || (p == (void *)0x3) || (p == (void *)-1))
    {
        p = (void *)GetProcAddress(opengl32Module, name);
    }
    return p;
}
#endif

namespace GlLoader
{
    bool Init()
    {
#ifdef _WIN32
        return gladLoadGLLoader((GLADloadproc)GetAnyGLFuncAddress) != 0;
#else
        return gladLoadGL() != 0;
#endif
    }
}
