#pragma once
#include "raylib.h"
#include <string>
#include <vector>

class NPC
{
public:
    NPC();
    NPC(Vector3 position, const std::string &name, const std::vector<std::string> &dialogue, Color color = BLUE);

    // Core functionality
    void Update(Vector3 playerPosition);
    void Draw();

    // Interaction
    bool IsInteractable() const;
    std::string GetNextDialogue();
    std::string GetCurrentDialogue() const;
    void ResetDialogue();

    // Getters
    Vector3 GetPosition() const;
    const Vector3 &GetPositionRef() const;
    Vector3 GetHeadPosition() const;
    float GetRadius() const;
    float GetHeight() const;
    const std::string &GetName() const;

    // Setters
    void SetInteractionRange(float range);
    void SetPosition(Vector3 pos);

private:
    Vector3 m_Position;
    std::string m_Name;
    std::vector<std::string> m_DialogueLines;
    size_t m_CurrentDialogueLine;
    Color m_Color;

    // Dimensions
    float m_Radius;
    float m_Height;

    // Interaction state
    bool m_CanInteract;
    float m_InteractionRange;
};
