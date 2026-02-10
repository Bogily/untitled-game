#include "NPC.h"
#include "raymath.h"

NPC::NPC()
    : m_DialogueLines(), m_CurrentDialogueLine(0), m_Color(BLUE), m_Radius(0.5f), m_Height(2.0f), m_CanInteract(false), m_InteractionRange(3.0f)
{
    metadata.name = "NPC";
}

NPC::NPC(Vector3 position, const std::string &name, const std::vector<std::string> &dialogue, Color color)
    : m_DialogueLines(dialogue), m_CurrentDialogueLine(0), m_Color(color), m_Radius(0.5f), m_Height(2.0f), m_CanInteract(false), m_InteractionRange(3.0f)
{
    transform.position = position;
    metadata.name = name;
}

void NPC::Update(Vector3 playerPosition)
{
    // Ignore y axis for distance check to allow interaction even if heights differ slightly
    Vector3 pos = transform.position;
    float distance = Vector3Distance(
        {pos.x, playerPosition.y, pos.z},
        playerPosition);
    m_CanInteract = (distance <= m_InteractionRange);
}

void NPC::Draw()
{
    // Draw body
    DrawCylinder(transform.position, m_Radius, m_Radius, m_Height, 16, m_Color);

    // Draw head (positioned on top of the body)
    Vector3 headPos = Vector3Add(transform.position, {0, m_Height + m_Radius * 0.8f, 0});
    DrawSphere(headPos, m_Radius * 0.8f, Fade(m_Color, 0.8f));

    // Draw interaction indicator if player is nearby
    if (m_CanInteract)
    {
        Vector3 indicatorPos = Vector3Add(transform.position, {0, m_Height + 1.5f, 0});
        float pulse = (sinf(GetTime() * 5.0f) + 1.0f) * 0.5f; // Simple pulse animation
        Color indicatorColor = ColorLerp(YELLOW, GOLD, pulse);
        DrawSphere(indicatorPos, 0.15f, indicatorColor);
    }
}

bool NPC::IsInteractable() const
{
    return m_CanInteract;
}

std::string NPC::GetNextDialogue()
{
    if (m_DialogueLines.empty())
        return "...";

    std::string line = m_DialogueLines[m_CurrentDialogueLine];
    m_CurrentDialogueLine = (m_CurrentDialogueLine + 1) % m_DialogueLines.size();
    return line;
}

std::string NPC::GetCurrentDialogue() const
{
    if (m_DialogueLines.empty())
        return "...";
    return m_DialogueLines[m_CurrentDialogueLine];
}

void NPC::ResetDialogue()
{
    m_CurrentDialogueLine = 0;
}

Vector3 NPC::GetPosition() const
{
    return transform.position;
}

const Vector3 &NPC::GetPositionRef() const
{
    return transform.position;
}

Vector3 NPC::GetHeadPosition() const
{
    return Vector3Add(transform.position, {0, m_Height + m_Radius * 0.8f, 0});
}

float NPC::GetRadius() const
{
    return m_Radius;
}

float NPC::GetHeight() const
{
    return m_Height;
}

const std::string &NPC::GetName() const
{
    return metadata.name;
}

void NPC::SetInteractionRange(float range)
{
    m_InteractionRange = range;
}

void NPC::SetPosition(Vector3 pos)
{
    transform.position = pos;
}
