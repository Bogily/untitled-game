#include "ActorRenderer.h"
#include "../actors/Player.h"
#include "../world/Scene.h"
#include "raymath.h"

void ActorRenderer::Draw(Player &player, Scene *scene, const Camera3D &renderCamera)
{
    // Fade player slightly when camera gets very close
    Vector3 playerViewPos = Vector3Add(player.GetTransform().position, {0.0f, player.eyeHeight, 0.0f});
    float cameraDistance = Vector3Distance(renderCamera.position, playerViewPos);
    const float fadeStartDistance = 4.4f;
    const float fadeMinDistance = 0.8f;
    float alpha = 1.0f;
    if (cameraDistance < fadeStartDistance)
    {
        float t = (cameraDistance - fadeMinDistance) / (fadeStartDistance - fadeMinDistance);
        t = Clamp(t, 0.0f, 1.0f);
        alpha = Lerp(0.25f, 1.0f, t);
    }

    Color tint = WHITE;
    tint.a = (unsigned char)(alpha * 255.0f);
    player.GetRender().tint = tint;

    player.Draw();

    if (scene)
    {
        for (auto &npc : scene->GetNPCs())
        {
            npc.Draw();
        }
    }
}
