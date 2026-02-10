/**
 * @file NPC.h
 * @brief Non-player character entity
 */

#pragma once
#include "Actor.h"
#include "raylib.h"
#include <string>
#include <vector>

/**
 * @brief Non-player character with dialogue system
 *
 * NPCs can be positioned in the world, have multiple dialogue lines,
 * and detect when the player is in interaction range.
 */
class NPC : public Actor
{
public:
    /**
     * @brief Construct a default NPC
     */
    NPC();

    /**
     * @brief Construct an NPC with parameters
     * @param position World position
     * @param name NPC name
     * @param dialogue Dialogue lines
     * @param color NPC visual color (default BLUE)
     */
    NPC(Vector3 position, const std::string &name, const std::vector<std::string> &dialogue, Color color = BLUE);

    /**
     * @brief Update NPC state and check interaction range
     * @param playerPosition Player's current position
     */
    void Update(Vector3 playerPosition);

    /**
     * @brief Render NPC visual representation
     */
    void Draw();

    /**
     * @brief Check if player can interact with this NPC
     * @return True if within interaction range
     */
    bool IsInteractable() const;

    /**
     * @brief Get next dialogue line and advance
     * @return Next dialogue string
     */
    std::string GetNextDialogue();

    /**
     * @brief Get current dialogue line without advancing
     * @return Current dialogue string
     */
    std::string GetCurrentDialogue() const;

    /**
     * @brief Reset dialogue to first line
     */
    void ResetDialogue();

    /**
     * @brief Get NPC world position
     * @return Position vector
     */
    Vector3 GetPosition() const;

    /**
     * @brief Get reference to NPC position
     * @return Position reference
     */
    const Vector3 &GetPositionRef() const;

    /**
     * @brief Get NPC head position (for speech bubbles)
     * @return Head position vector
     */
    Vector3 GetHeadPosition() const;

    /**
     * @brief Get NPC cylinder radius
     * @return Radius value
     */
    float GetRadius() const;

    /**
     * @brief Get NPC cylinder height
     * @return Height value
     */
    float GetHeight() const;

    /**
     * @brief Get NPC name
     * @return Name string reference
     */
    const std::string &GetName() const;

    /**
     * @brief Set interaction range distance
     * @param range New interaction range
     */
    void SetInteractionRange(float range);

    /**
     * @brief Set NPC world position
     * @param pos New position
     */
    void SetPosition(Vector3 pos);

private:
    std::vector<std::string> m_DialogueLines; ///< All dialogue lines
    size_t m_CurrentDialogueLine;             ///< Current dialogue index
    Color m_Color;                            ///< Visual color

    float m_Radius; ///< Cylinder radius
    float m_Height; ///< Cylinder height

    bool m_CanInteract;       ///< Interaction availability
    float m_InteractionRange; ///< Max interaction distance
};
