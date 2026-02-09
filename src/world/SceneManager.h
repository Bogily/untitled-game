/**
 * @file SceneManager.h
 * @brief Scene registration and switching system
 */

#pragma once
#include "Scene.h"
#include <unordered_map>
#include <string>
#include <memory>

/**
 * @brief Scene manager handling scene switching and data access
 *
 * Manages registered scenes and coordinates scene transitions.
 */
class SceneManager
{
public:
    /**
     * @brief Construct scene manager
     */
    SceneManager();

    /**
     * @brief Destroy scene manager and cleanup scenes
     */
    ~SceneManager();

    /**
     * @brief Register scene with name
     * @param name Scene identifier
     * @param scene Scene pointer (ownership transferred)
     */
    void RegisterScene(const std::string &name, Scene *scene);

    /**
     * @brief Load and activate scene by name
     * @param name Scene identifier
     */
    void LoadScene(const std::string &name);

    /**
     * @brief Reload current scene
     */
    void ReloadCurrentScene();

    /**
     * @brief Get currently active scene
     * @return Current scene pointer
     */
    Scene *GetCurrentScene() const { return currentScene; }

    /**
     * @brief Get current scene name
     * @return Scene name
     */
    const std::string &GetCurrentSceneName() const { return currentSceneName; }

    /**
     * @brief Check if scene is registered
     * @param name Scene identifier
     * @return True if scene exists
     */
    bool HasScene(const std::string &name) const;

private:
    std::unordered_map<std::string, Scene *> scenes; ///< Registered scenes
    Scene *currentScene;                             ///< Active scene
    std::string currentSceneName;                    ///< Active scene name

    /**
     * @brief Unload currently active scene
     */
    void UnloadCurrentScene();
};
