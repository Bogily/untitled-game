#pragma once
#include "raylib.h"
#include "raymath.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void Init(int width, int height);
    void Shutdown();

    // Get screen dimensions
    int GetScreenWidth() const { return screenWidth; }
    int GetScreenHeight() const { return screenHeight; }

private:
    int screenWidth;
    int screenHeight;
};
