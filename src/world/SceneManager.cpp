#include "SceneManager.h"

SceneManager::SceneManager()
    : currentScene(nullptr), currentSceneName("")
{
}

SceneManager::~SceneManager()
{
    UnloadCurrentScene();

    // Delete all registered scenes
    for (auto &pair : scenes)
    {
        delete pair.second;
    }
    scenes.clear();
}

void SceneManager::RegisterScene(const std::string &name, Scene *scene)
{
    if (scenes.find(name) != scenes.end())
    {
        TraceLog(LOG_WARNING, "SceneManager: Scene '%s' already registered", name.c_str());
        return;
    }

    scenes[name] = scene;
    TraceLog(LOG_INFO, "SceneManager: Registered scene '%s'", name.c_str());
}

void SceneManager::LoadScene(const std::string &name)
{
    if (scenes.find(name) == scenes.end())
    {
        TraceLog(LOG_ERROR, "SceneManager: Scene '%s' not found", name.c_str());
        return;
    }

    // Unload current scene
    UnloadCurrentScene();

    // Load new scene
    currentScene = scenes[name];
    currentSceneName = name;
    currentScene->Load();

    TraceLog(LOG_INFO, "SceneManager: Loaded scene '%s'", name.c_str());
}

void SceneManager::ReloadCurrentScene()
{
    if (currentScene && !currentSceneName.empty())
    {
        LoadScene(currentSceneName);
    }
}

bool SceneManager::HasScene(const std::string &name) const
{
    return scenes.find(name) != scenes.end();
}

void SceneManager::UnloadCurrentScene()
{
    if (currentScene)
    {
        currentScene->Unload();
        currentScene = nullptr;
        currentSceneName = "";
    }
}
