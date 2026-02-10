/**
 * @file Actor.h
 * @brief Base actor and shared components
 */

#pragma once

#include "raylib.h"
#include <string>

struct TransformComponent
{
    Vector3 position = {0.0f, 0.0f, 0.0f};
    Vector3 rotation = {0.0f, 0.0f, 0.0f};
    Vector3 scale = {1.0f, 1.0f, 1.0f};
};

struct RenderComponent
{
    Model model = {0};
    bool modelLoaded = false;
    Vector3 modelScale = {1.0f, 1.0f, 1.0f};
    Vector3 modelRotationOffset = {0.0f, 0.0f, 0.0f};
    Color tint = WHITE;
};

struct MetadataComponent
{
    std::string name;
    bool isStatic = false;
};

class Actor
{
public:
    virtual ~Actor() = default;

    TransformComponent &GetTransform() { return transform; }
    const TransformComponent &GetTransform() const { return transform; }

    RenderComponent &GetRender() { return render; }
    const RenderComponent &GetRender() const { return render; }

    MetadataComponent &GetMetadata() { return metadata; }
    const MetadataComponent &GetMetadata() const { return metadata; }

protected:
    TransformComponent transform;
    RenderComponent render;
    MetadataComponent metadata;
};
