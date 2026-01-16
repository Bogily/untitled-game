#pragma once
#include <RmlUi/Core.h>

class RaylibRmlUi
{
public:
    static Rml::Context *Context;

    static bool Initialize(int windowWidth, int windowHeight);
    static void LoadFont(const char *path);
    static void LoadFontFace(const char *path, const char *family, bool fallback = false);
    static void DeInitialize();
    static void EnableDebugger();
    static void DisableDebugger();
    static void ToggleDebugger();
    static void LoadRml(const char *path, const char *id, bool show = false);
    static void ShowPage(const char *id);
    static void Update();
    static void Draw();
    static void SetViewport(int width, int height);
    static Rml::ElementDocument *GetPage(const char *id);
};
