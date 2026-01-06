#include "SpeechBubble.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>

namespace Graphics {

// ============================================================================
// Internal Render Helpers
// ============================================================================

namespace {

    // Helper structure for billboard coordinate system
    struct BillboardGeometry
    {
        Vector3 center;         // World position of billboard center
        Vector3 right;          // Billboard's right vector (X-axis)
        Vector3 up;             // Billboard's up vector (Y-axis)
        Vector3 forward;        // Direction to camera (Z-axis)
        
        // Construct billboard geometry that always faces the camera
        static BillboardGeometry FromCamera(Vector3 worldPos, Camera3D camera)
        {
            BillboardGeometry geo;
            geo.center = worldPos;
            
            // Calculate normalized direction from billboard to camera
            geo.forward = Vector3Normalize(Vector3Subtract(camera.position, worldPos));
            
            // Calculate orthogonal right and up vectors
            Vector3 worldUp = {0.0f, 1.0f, 0.0f};
            geo.right = Vector3Normalize(Vector3CrossProduct(worldUp, geo.forward));
            geo.up = Vector3CrossProduct(geo.forward, geo.right);
            
            return geo;
        }
        
        // Transform local billboard coordinates to world space
        Vector3 GetPoint(float x, float y, float z = 0.0f) const
        {
            Vector3 pos = center;
            pos = Vector3Add(pos, Vector3Scale(right, x));
            pos = Vector3Add(pos, Vector3Scale(up, y));
            pos = Vector3Add(pos, Vector3Scale(forward, z));
            return pos;
        }
    };

    // Draw a double-sided triangle (visible from both sides)
    void TriangleDoubleSided(Vector3 p1, Vector3 p2, Vector3 p3)
    {
        // Front face (CCW)
        rlVertex3f(p1.x, p1.y, p1.z);
        rlVertex3f(p2.x, p2.y, p2.z);
        rlVertex3f(p3.x, p3.y, p3.z);
        
        // Back face (CW)
        rlVertex3f(p1.x, p1.y, p1.z);
        rlVertex3f(p3.x, p3.y, p3.z);
        rlVertex3f(p2.x, p2.y, p2.z);
    }
    
    // Draw a double-sided quad using two triangles per side
    void QuadDoubleSided(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4)
    {
        // Front face (two triangles, CCW)
        rlVertex3f(p1.x, p1.y, p1.z);
        rlVertex3f(p2.x, p2.y, p2.z);
        rlVertex3f(p3.x, p3.y, p3.z);
        
        rlVertex3f(p1.x, p1.y, p1.z);
        rlVertex3f(p3.x, p3.y, p3.z);
        rlVertex3f(p4.x, p4.y, p4.z);
        
        // Back face (two triangles, CW)
        rlVertex3f(p1.x, p1.y, p1.z);
        rlVertex3f(p3.x, p3.y, p3.z);
        rlVertex3f(p2.x, p2.y, p2.z);
        
        rlVertex3f(p1.x, p1.y, p1.z);
        rlVertex3f(p4.x, p4.y, p4.z);
        rlVertex3f(p3.x, p3.y, p3.z);
    }

    // Draw the rectangular regions of a rounded rectangle (excluding corners)
    void DrawCenterRectangles(const BillboardGeometry& geo, float halfW, float halfH, float r)
    {
        // Center vertical rectangle
        QuadDoubleSided(
            geo.GetPoint(-halfW, halfH - r),
            geo.GetPoint(halfW, halfH - r),
            geo.GetPoint(halfW, -halfH + r),
            geo.GetPoint(-halfW, -halfH + r)
        );
        
        // Top horizontal rectangle
        QuadDoubleSided(
            geo.GetPoint(-halfW + r, halfH),
            geo.GetPoint(halfW - r, halfH),
            geo.GetPoint(halfW - r, halfH - r),
            geo.GetPoint(-halfW + r, halfH - r)
        );
        
        // Bottom horizontal rectangle
        QuadDoubleSided(
            geo.GetPoint(-halfW + r, -halfH + r),
            geo.GetPoint(halfW - r, -halfH + r),
            geo.GetPoint(halfW - r, -halfH),
            geo.GetPoint(-halfW + r, -halfH)
        );
    }

    // Draw a rounded corner using a triangle fan
    void DrawRoundedCorner(const BillboardGeometry& geo, float cx, float cy, float r,
                           float startAngle, float endAngle, int segments)
    {
        Vector3 cornerCenter = geo.GetPoint(cx, cy);
        float angleStep = (endAngle - startAngle) / segments;
        
        // Lambda to calculate point on circle at given angle
        auto GetCirclePoint = [&](float angle) {
            float rad = angle * DEG2RAD;
            return Vector3Add(cornerCenter, Vector3Add(
                Vector3Scale(geo.right, cosf(rad) * r),
                Vector3Scale(geo.up, sinf(rad) * r)));
        };
        
        for (int i = 0; i < segments; i++)
        {
            Vector3 p1 = cornerCenter;
            Vector3 p2 = GetCirclePoint(startAngle + i * angleStep);
            Vector3 p3 = GetCirclePoint(startAngle + (i + 1) * angleStep);
            
            TriangleDoubleSided(p1, p2, p3);
        }
    }

    // Draw a dynamic tail that points from bubble to target position
    void DrawDynamicTail(const BillboardGeometry& geo, float bubbleHalfW, float bubbleHalfH, 
                         float tailBaseWidth, const Vector3* targetPos, Color color)
    {
        if (!targetPos) return;

        Vector3 toTarget = Vector3Subtract(*targetPos, geo.center);
        float distance3D = Vector3Length(toTarget);
        
        // Project target direction onto billboard plane
        float localX = Vector3DotProduct(toTarget, geo.right);
        float localY = Vector3DotProduct(toTarget, geo.up);
        
        float len2D = sqrtf(localX * localX + localY * localY);
        if (len2D < 0.1f) return; // Target too close to bubble
        
        // Find intersection point with bubble border using ray-box intersection
        float t = std::min(bubbleHalfW / fabsf(localX), bubbleHalfH / fabsf(localY));
        if (t >= 0.95f) return; // Target is inside bubble

        // Inset tail base slightly for smooth visual overlap
        const float TAIL_INSET = 0.08f;
        t *= (1.0f - TAIL_INSET);
        
        Vector3 tailBase = Vector3Add(geo.center, Vector3Add(
            Vector3Scale(geo.right, localX * t), 
            Vector3Scale(geo.up, localY * t)));

        // Calculate perpendicular direction for tail base width
        float perpX = -localY / len2D;
        float perpY = localX / len2D;
        
        // Scale tail width based on distance
        float distanceScale = std::clamp(distance3D / 2.0f, 0.0f, 1.2f);
        float scaledWidth = tailBaseWidth * distanceScale * 0.5f;
        
        Vector3 widthOffset = Vector3Add(
            Vector3Scale(geo.right, perpX * scaledWidth), 
            Vector3Scale(geo.up, perpY * scaledWidth));
        
        Vector3 p1 = Vector3Subtract(tailBase, widthOffset);
        Vector3 p2 = Vector3Add(tailBase, widthOffset);

        rlBegin(RL_TRIANGLES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        TriangleDoubleSided(p1, p2, *targetPos);
        rlEnd();
    }

    // Draw text with automatic word wrapping within a rectangle
    void DrawTextBoxed(Font font, const char* text, Rectangle rec, float fontSize, 
                       float spacing, Color color)
    {
        if (font.texture.id == 0) font = GetFontDefault();

        Vector2 pos = { rec.x, rec.y };
        float lineHeight = fontSize;
        float spaceWidth = MeasureTextEx(font, " ", fontSize, spacing).x;
        
        std::string word;
        
        for (int i = 0; text[i] != '\0'; i++)
        {
            char c = text[i];
            
            if (c == ' ' || c == '\n' || text[i + 1] == '\0')
            {
                // Add last character if not separator
                if (c != ' ' && c != '\n') word += c;
                
                if (!word.empty())
                {
                    Vector2 wordSize = MeasureTextEx(font, word.c_str(), fontSize, spacing);
                    
                    // Wrap if word doesn't fit
                    if (pos.x + wordSize.x > rec.x + rec.width && pos.x > rec.x)
                    {
                        pos.x = rec.x;
                        pos.y += lineHeight;
                    }
                    
                    // Draw word if within bounds
                    if (pos.y < rec.y + rec.height)
                    {
                        DrawTextEx(font, word.c_str(), pos, fontSize, spacing, color);
                    }
                    
                    pos.x += wordSize.x;
                    word.clear();
                }
                
                // Handle separators
                if (c == ' ') pos.x += spaceWidth;
                else if (c == '\n') { pos.x = rec.x; pos.y += lineHeight; }
            }
            else
            {
                word += c;
            }
        }
    }

    // Draw a rounded rectangle billboard
    void DrawRoundedRectangle(const BillboardGeometry& geo, float width, float height, 
                              float radius, int segments, Color color)
    {
        float halfW = width * 0.5f;
        float halfH = height * 0.5f;
        
        // Clamp radius to prevent oversized corners
        radius = std::min(radius, std::min(halfW, halfH) * 0.9f);
        
        rlBegin(RL_TRIANGLES);
        rlColor4ub(color.r, color.g, color.b, color.a);
        
        DrawCenterRectangles(geo, halfW, halfH, radius);
        
        // Draw corners clockwise from top-right
        DrawRoundedCorner(geo, halfW - radius, halfH - radius, radius, 0.0f, 90.0f, segments);
        DrawRoundedCorner(geo, -halfW + radius, halfH - radius, radius, 90.0f, 180.0f, segments);
        DrawRoundedCorner(geo, -halfW + radius, -halfH + radius, radius, 180.0f, 270.0f, segments);
        DrawRoundedCorner(geo, halfW - radius, -halfH + radius, radius, 270.0f, 360.0f, segments);
        
        rlEnd();
    }

    // Calculate dynamic bubble dimensions based on text content
    void CalculateBubbleDimensions(const std::string& text, float& outWidth, 
                                   float& outHeight, int& outLineCount)
    {
        const float CHAR_WIDTH = 0.25f;          // Average character width
        const float LINE_HEIGHT = 0.45f;         // Height per line
        const float MAX_BUBBLE_WIDTH = 6.0f;     // Maximum width before wrapping
        const float PADDING = 0.5f;              // Internal padding
        
        float currentLineWidth = 0;
        float maxLineWidth = 0;
        float currentWordWidth = 0;
        int lineCount = 1;
        
        for (char c : text)
        {
            if (c == '\n')
            {
                maxLineWidth = std::max(maxLineWidth, currentLineWidth + currentWordWidth);
                currentLineWidth = 0;
                currentWordWidth = 0;
                lineCount++;
            }
            else if (c == ' ')
            {
                float totalWidth = currentLineWidth + currentWordWidth;
                if (totalWidth > MAX_BUBBLE_WIDTH - PADDING * 2.0f)
                {
                    maxLineWidth = std::max(maxLineWidth, currentLineWidth);
                    currentLineWidth = currentWordWidth + CHAR_WIDTH;
                    lineCount++;
                }
                else
                {
                    currentLineWidth = totalWidth + CHAR_WIDTH;
                }
                currentWordWidth = 0;
            }
            else
            {
                currentWordWidth += CHAR_WIDTH;
            }
        }
        
        maxLineWidth = std::max(maxLineWidth, currentLineWidth + currentWordWidth);
        
        outWidth = std::max(maxLineWidth + PADDING * 2.0f, 1.0f);
        outHeight = std::max(lineCount, 1) * LINE_HEIGHT + PADDING;
        outLineCount = std::max(lineCount, 1);
    }

} // anonymous namespace

// ============================================================================
// SpeechBubble
// ============================================================================

SpeechBubble::SpeechBubble()
    : worldPosition({0, 0, 0})
    , targetPosition(nullptr)
    , positionalOffset({0, 0, 0})
    , text("")
    , timeRemaining(0)
    , isActive(false)
    , lineCount(1)
{
}

void SpeechBubble::Show(const std::string& message, Vector3 initialPosition, 
                        const Vector3* target, float duration)
{
    text = message;
    targetPosition = target;

    // Calculate dynamic bubble dimensions based on text content
    CalculateBubbleDimensions(message, config.width, config.height, lineCount);

    // Position bubble above target with appropriate vertical offset
    const float GAP_ABOVE_TARGET = 3.0f;
    config.offsetY = GAP_ABOVE_TARGET + (config.height * 0.5f);
    
    // Add random jitter for organic, natural appearance
    float randomX = ((float)GetRandomValue(0, 200) / 100.0f - 1.0f) * 3.0f;  // [-3, 3]
    float randomY = ((float)GetRandomValue(0, 100) / 100.0f - 0.5f) * 1.0f;  // [-0.5, 0.5]
    float randomZ = ((float)GetRandomValue(0, 200) / 100.0f - 1.0f) * 3.0f;  // [-3, 3]

    positionalOffset = {randomX, config.offsetY + randomY, randomZ};

    Vector3 basePos = target ? *target : initialPosition;
    worldPosition = Vector3Add(basePos, positionalOffset);
    
    timeRemaining = (duration > 0) ? duration : config.defaultDuration;
    isActive = true;
}

void SpeechBubble::Update(float deltaTime)
{
    if (!isActive) return;
    
    if (targetPosition)
    {
        worldPosition = Vector3Add(*targetPosition, positionalOffset);
    }
    
    timeRemaining -= deltaTime;
    if (timeRemaining <= 0)
    {
        isActive = false;
        targetPosition = nullptr;
    }
}

void SpeechBubble::DrawBackground(Camera3D camera)
{
    if (!isActive) return;
    
    // Calculate fade alpha for animation
    float alpha = CalculateFadeAlpha();
    Color bgColor = ApplyAlpha(config.backgroundColor, alpha);
    Color borderColor = ApplyAlpha(config.borderColor, alpha);
    
    BillboardGeometry geo = BillboardGeometry::FromCamera(worldPosition, camera);
    
    // Setup rendering state
    rlPushMatrix();
    rlDisableBackfaceCulling();
    rlDisableTexture();
    rlDisableDepthMask();
    
    // Draw outer border
    DrawRoundedRectangle(geo, config.width, config.height, config.cornerRadius, 
                        config.cornerSegments, borderColor);
    DrawDynamicTail(geo, config.width * 0.5f, config.height * 0.5f, 
                    config.tailWidth, targetPosition, borderColor);
    
    // Draw inner fill (slightly offset to prevent z-fighting)
    BillboardGeometry innerGeo = geo;
    innerGeo.center = geo.GetPoint(0, 0, config.depthOffset);
    
    float innerWidth = config.width - (config.borderThickness * 2.0f);
    float innerHeight = config.height - (config.borderThickness * 2.0f);
    float innerRadius = std::max(config.cornerRadius - config.borderThickness, 0.05f);
    float innerTailWidth = std::max(config.tailWidth - config.borderThickness * 2.0f, 0.05f);
    
    DrawRoundedRectangle(innerGeo, innerWidth, innerHeight, innerRadius, 
                        config.cornerSegments, bgColor);
    DrawDynamicTail(innerGeo, config.width * 0.5f, config.height * 0.5f, 
                    innerTailWidth, targetPosition, bgColor);
    
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
    rlPopMatrix();
}

void SpeechBubble::DrawText(Camera3D camera)
{
    if (!isActive) return;
    
    // Frustum culling: check if bubble is in front of camera
    Vector3 toCamera = Vector3Subtract(worldPosition, camera.position);
    Vector3 cameraForward = Vector3Subtract(camera.target, camera.position);
    if (Vector3DotProduct(toCamera, cameraForward) <= 0.0f) return;
    
    BillboardGeometry geo = BillboardGeometry::FromCamera(worldPosition, camera);
    
    // Calculate text area dimensions (inner bubble area minus padding)
    float padding = config.borderThickness * 2.0f;
    float halfInnerW = (config.width * 0.5f) - padding;
    float halfInnerH = (config.height * 0.5f) - padding;
    
    // Project text area corners to screen space
    Vector2 topScreen = GetWorldToScreen(geo.GetPoint(0, halfInnerH), camera);
    Vector2 bottomScreen = GetWorldToScreen(geo.GetPoint(0, -halfInnerH), camera);
    Vector2 leftScreen = GetWorldToScreen(geo.GetPoint(-halfInnerW, 0), camera);
    Vector2 rightScreen = GetWorldToScreen(geo.GetPoint(halfInnerW, 0), camera);
    
    // Calculate pixel dimensions of text area
    float pixelWidth = fabsf(rightScreen.x - leftScreen.x);
    float pixelHeight = fabsf(bottomScreen.y - topScreen.y);
    
    Vector2 screenPos = GetWorldToScreen(worldPosition, camera);
    Rectangle textBox = {
        screenPos.x - pixelWidth * 0.5f,
        screenPos.y - pixelHeight * 0.5f,
        pixelWidth,
        pixelHeight
    };
    
    // Screen space culling: skip if completely off-screen
    if (textBox.x + textBox.width < 0 || textBox.x > GetScreenWidth() ||
        textBox.y + textBox.height < 0 || textBox.y > GetScreenHeight())
    {
        return;
    }

    // Calculate adaptive font size based on bubble size and line count
    float fontSize = (pixelHeight / (float)lineCount) * 0.70f;
    fontSize = std::clamp(fontSize, 8.0f, 40.0f);  // Clamp to readable range
    
    Color textColor = ApplyAlpha(config.textColor, CalculateFadeAlpha());
    float spacing = fontSize / 10.0f;
    
    DrawTextBoxed(GetFontDefault(), text.c_str(), textBox, fontSize, spacing, textColor);
}

float SpeechBubble::CalculateFadeAlpha() const
{
    if (timeRemaining < config.fadeDuration)
    {
        return timeRemaining / config.fadeDuration;
    }
    return 1.0f;
}

Color SpeechBubble::ApplyAlpha(Color color, float alpha) const
{
    color.a = (unsigned char)((float)color.a * alpha);
    return color;
}

// ============================================================================
// SpeechBubbleManager
// ============================================================================

SpeechBubbleManager::SpeechBubbleManager(int maxBubbles)
    : maxBubbles(maxBubbles)
{
    bubbles.reserve(maxBubbles);
}

void SpeechBubbleManager::ShowBubble(const std::string& message, Vector3 initialPosition, 
                                     const Vector3* target, float duration)
{
    // Try to reuse an inactive bubble (object pooling)
    for (auto& bubble : bubbles)
    {
        if (!bubble.isActive)
        {
            bubble.Show(message, initialPosition, target, duration);
            return;
        }
    }

    // Create new bubble if we haven't reached the limit
    if (bubbles.size() < (size_t)maxBubbles)
    {
        bubbles.emplace_back();
        bubbles.back().Show(message, initialPosition, target, duration);
    }
}

void SpeechBubbleManager::UpdateAll(float deltaTime)
{
    for (auto& bubble : bubbles)
    {
        bubble.Update(deltaTime);
    }
}

void SpeechBubbleManager::DrawBackgrounds(Camera3D camera)
{
    rlSetBlendMode(RL_BLEND_ALPHA);
    
    for (auto& bubble : bubbles)
    {
        bubble.DrawBackground(camera);
    }
}

void SpeechBubbleManager::DrawText(Camera3D camera)
{
    for (auto& bubble : bubbles)
    {
        bubble.DrawText(camera);
    }
}

void SpeechBubbleManager::Clear()
{
    bubbles.clear();
}

int SpeechBubbleManager::GetActiveCount() const
{
    int count = 0;
    for (const auto& bubble : bubbles)
    {
        if (bubble.isActive) count++;
    }
    return count;
}

void SpeechBubbleManager::SetConfig(const SpeechBubbleConfig& newConfig)
{
    for (auto& bubble : bubbles)
    {
        bubble.config = newConfig;
    }
}

} // namespace Graphics
