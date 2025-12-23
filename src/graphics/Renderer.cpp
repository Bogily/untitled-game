#include "Renderer.h"

Renderer::Renderer()
    : screenWidth(0), screenHeight(0)
{
}

Renderer::~Renderer()
{
    Shutdown();
}

void Renderer::Init(int width, int height)
{
    screenWidth = width;
    screenHeight = height;
    TraceLog(LOG_INFO, "Simple Renderer initialized (%dx%d)", width, height);
}

void Renderer::Shutdown()
{
    TraceLog(LOG_INFO, "Renderer shutdown");
}
