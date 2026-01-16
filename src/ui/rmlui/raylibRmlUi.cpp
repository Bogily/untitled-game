#include "raylibRmlUi.h"
#include "raylibFileInterface.h"
#include "raylibSystemInterface.h"
#include "raylibRenderInterface.h"
#include "raylib.h"
#include "RmlUi/Debugger.h"
#include <unordered_map>

static RaylibFileInterface fileInterface;
static RaylibRenderInterface renderInterface;
static RaylibSystemInterface systemInterface;
static std::unordered_map<std::string, Rml::ElementDocument *> documents;

Rml::Context *RaylibRmlUi::Context = nullptr;

bool RaylibRmlUi::Initialize(int windowWidth, int windowHeight)
{
    Rml::SetFileInterface(&fileInterface);
    Rml::SetRenderInterface(&renderInterface);
    Rml::SetSystemInterface(&systemInterface);

    Rml::Initialise();
    Context = Rml::CreateContext("ui", {windowWidth, windowHeight});
    if (!Context)
    {
        Rml::Shutdown();
        return false;
    }

    Rml::Debugger::Initialise(Context);
    return true;
}

void RaylibRmlUi::LoadFont(const char *path)
{
    Rml::LoadFontFace(path);
}

void RaylibRmlUi::LoadFontFace(const char *path, const char *family, bool fallback)
{
    if (!Rml::LoadFontFace(path, fallback))
    {
        TraceLog(LOG_WARNING, "Failed to load font: %s", path);
    }
    else
    {
        TraceLog(LOG_INFO, "Loaded font: %s as family '%s'", path, family);
    }
}

void RaylibRmlUi::DeInitialize()
{
    if (!Context)
        return;

    for (auto &document : documents)
        document.second->Close();
    documents.clear();

    Context = nullptr;
    Rml::Shutdown();
}

void RaylibRmlUi::EnableDebugger()
{
    Rml::Debugger::SetVisible(true);
}

void RaylibRmlUi::DisableDebugger()
{
    Rml::Debugger::SetVisible(false);
}

void RaylibRmlUi::ToggleDebugger()
{
    Rml::Debugger::SetVisible(!Rml::Debugger::IsVisible());
}

void RaylibRmlUi::Update()
{
    if (!Context)
        return;

    Vector2 delta = GetMouseDelta();
    if (delta.x != 0 || delta.y != 0)
    {
        Vector2 pos = GetMousePosition();
        Context->ProcessMouseMove(static_cast<int>(pos.x), static_cast<int>(pos.y), 0);
    }

    systemInterface.HandleMouseEvents(Context);
    Context->ProcessMouseWheel(-GetMouseWheelMove(), 0);
    systemInterface.HandleKeyboardEvents(Context);
    Context->Update();
}

void RaylibRmlUi::Draw()
{
    if (!Context)
        return;
    renderInterface.BeginFrame();
    Context->Render();
    renderInterface.EndFrame();
}

void RaylibRmlUi::SetViewport(int width, int height)
{
    if (Context)
        Context->SetDimensions({width, height});
}

void RaylibRmlUi::LoadRml(const char *path, const char *id, bool show)
{
    documents[id] = Context->LoadDocument(path);
    if (show)
        ShowPage(id);
}

void RaylibRmlUi::ShowPage(const char *id)
{
    for (auto &page : documents)
    {
        page.second->Hide();
        if (page.first == id)
            page.second->Show();
    }
}

Rml::ElementDocument *RaylibRmlUi::GetPage(const char *id)
{
    auto it = documents.find(id);
    if (it != documents.end())
        return it->second;
    return nullptr;
}
