#pragma once
#include "Scene.h"
#include <unordered_map>
#include <string>
#include <memory>

// SceneManager now only handles scene switching and data access
class SceneManager
{
public:
    SceneManager();
    ~SceneManager();

    // Scene registration
    void RegisterScene(const std::string &name, Scene *scene);

    // Scene switching
    void LoadScene(const std::string &name);
    void ReloadCurrentScene();

    // Queries
    Scene *GetCurrentScene() const { return currentScene; }
    const std::string &GetCurrentSceneName() const { return currentSceneName; }
    bool HasScene(const std::string &name) const;

private:
    std::unordered_map<std::string, Scene *> scenes;
    Scene *currentScene;
    std::string currentSceneName;

    void UnloadCurrentScene();
};
