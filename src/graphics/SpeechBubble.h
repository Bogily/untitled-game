#pragma once
#include "raylib.h"
#include <string>
#include <vector>

namespace Graphics {

// ============================================================================
// SpeechBubbleConfig - Configuration for speech bubble appearance and behavior
// ============================================================================
struct SpeechBubbleConfig
{
    // Dimensions (in world units)
    float width = 3.0f;
    float height = 1.2f;
    float offsetY = 3.5f;            // Height above NPC position
    
    // Colors
    Color backgroundColor = {255, 255, 255, 240};
    Color textColor = BLACK;
    Color borderColor = BLACK;
    
    // Border properties
    float borderThickness = 0.05f;   // Border width in world units
    float cornerRadius = 0.15f;      // Rounded corner radius
    int cornerSegments = 6;          // Corner smoothness (higher = smoother)
    
    // Tail (speech bubble pointer)
    float tailWidth = 0.3f;
    float tailHeight = 0.5f;
    float tailOffset = 0.1f;         // Overlap amount with bubble
    
    // Text rendering
    int fontSize = 20;               // Base font size in pixels
    
    // Animation timing
    float fadeDuration = 0.5f;       // Fade out duration in seconds
    float defaultDuration = 3.0f;    // Default visibility duration in seconds
    
    // Rendering settings
    float depthOffset = 0.01f;       // Z-offset to prevent z-fighting
};

// ============================================================================
// SpeechBubble - Individual speech bubble instance with text and animation
// ============================================================================
class SpeechBubble
{
public:
    // Position and tracking
    Vector3 worldPosition;              // Current world position
    const Vector3* targetPosition;      // Pointer to track moving objects
    Vector3 positionalOffset;           // Offset from target (includes jitter)
    
    // Content
    std::string text;                   // Displayed message
    int lineCount = 1;                  // Number of text lines (for scaling)
    
    // Animation state
    float timeRemaining;                // Time until bubble disappears
    bool isActive;                      // Whether bubble is currently shown
    
    // Appearance configuration
    SpeechBubbleConfig config;
    
    SpeechBubble();
    
    // Display a new message in the bubble
    void Show(const std::string& message, Vector3 initialPosition, 
              const Vector3* target = nullptr, float duration = 0.0f);
    
    // Update animation and position
    void Update(float deltaTime);
    
    // Render the bubble geometry and background
    void DrawBackground(Camera3D camera);
    
    // Render the text content
    void DrawText(Camera3D camera);
    
private:
    // Calculate current fade alpha based on time remaining
    float CalculateFadeAlpha() const;
    
    // Apply alpha transparency to a color
    Color ApplyAlpha(Color color, float alpha) const;
};

// ============================================================================
// SpeechBubbleManager - Manages multiple speech bubbles with pooling
// ============================================================================
class SpeechBubbleManager
{
private:
    std::vector<SpeechBubble> bubbles;
    int maxBubbles;

public:
    explicit SpeechBubbleManager(int maxBubbles = 10);

    // Create or reuse a bubble to display a message
    void ShowBubble(const std::string& message, Vector3 initialPosition, 
                    const Vector3* target = nullptr, float duration = 0.0f);
    
    // Update all active bubbles
    void UpdateAll(float deltaTime);
    
    // Render all bubble backgrounds (call before text for proper layering)
    void DrawBackgrounds(Camera3D camera);
    
    // Render all bubble text
    void DrawText(Camera3D camera);
    
    // Remove all bubbles
    void Clear();
    
    // Get count of currently active bubbles
    int GetActiveCount() const;
    
    // Apply configuration to all bubbles
    void SetConfig(const SpeechBubbleConfig& newConfig);
};

} // namespace Graphics
