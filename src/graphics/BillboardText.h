#pragma once
#include "raylib.h"
#include "raymath.h"
#include <string>

class BillboardText
{
public:
    // Draw 3D text at a world position that always faces the camera
    static void DrawText3D(const char *text, Vector3 worldPosition, Camera3D camera,
                           int fontSize, Color color, bool centered = true)
    {
        // Convert world position to screen position
        Vector2 screenPos = GetWorldToScreen(worldPosition, camera);

        // Check if the position is in front of the camera
        Vector3 toCameraDir = Vector3Subtract(worldPosition, camera.position);
        Vector3 cameraForward = Vector3Subtract(camera.target, camera.position);
        float dotProduct = Vector3DotProduct(Vector3Normalize(toCameraDir), Vector3Normalize(cameraForward));

        // Only render if position is in front of camera (dot product > 0)
        if (dotProduct > 0.0f && screenPos.x >= 0 && screenPos.x <= GetScreenWidth() &&
            screenPos.y >= 0 && screenPos.y <= GetScreenHeight())
        {
            if (centered)
            {
                int textWidth = MeasureText(text, fontSize);
                screenPos.x -= textWidth / 2.0f;
                screenPos.y -= fontSize / 2.0f;
            }

            DrawText(text, (int)screenPos.x, (int)screenPos.y, fontSize, color);
        }
    }

    // Draw 3D text with background
    static void DrawText3DWithBackground(const char *text, Vector3 worldPosition, Camera3D camera,
                                         int fontSize, Color textColor, Color bgColor)
    {
        Vector2 screenPos = GetWorldToScreen(worldPosition, camera);

        // Check if the position is in front of the camera
        Vector3 toCameraDir = Vector3Subtract(worldPosition, camera.position);
        Vector3 cameraForward = Vector3Subtract(camera.target, camera.position);
        float dotProduct = Vector3DotProduct(Vector3Normalize(toCameraDir), Vector3Normalize(cameraForward));

        if (dotProduct > 0.0f && screenPos.x >= 0 && screenPos.x <= GetScreenWidth() &&
            screenPos.y >= 0 && screenPos.y <= GetScreenHeight())
        {
            int textWidth = MeasureText(text, fontSize);
            int padding = 5;

            screenPos.x -= textWidth / 2.0f;
            screenPos.y -= fontSize / 2.0f;

            // Draw background rectangle
            DrawRectangle((int)screenPos.x - padding, (int)screenPos.y - padding,
                          textWidth + padding * 2, fontSize + padding * 2, bgColor);

            DrawText(text, (int)screenPos.x, (int)screenPos.y, fontSize, textColor);
        }
    }

    // Draw distance-scaled text (gets smaller with distance)
    static void DrawText3DScaled(const char *text, Vector3 worldPosition, Camera3D camera,
                                 float baseSize, float maxDistance, Color color)
    {
        Vector2 screenPos = GetWorldToScreen(worldPosition, camera);

        // Calculate distance from camera
        float distance = Vector3Distance(camera.position, worldPosition);

        // Check if position is in front of camera
        Vector3 toCameraDir = Vector3Subtract(worldPosition, camera.position);
        Vector3 cameraForward = Vector3Subtract(camera.target, camera.position);
        float dotProduct = Vector3DotProduct(Vector3Normalize(toCameraDir), Vector3Normalize(cameraForward));

        if (dotProduct > 0.0f && distance < maxDistance &&
            screenPos.x >= 0 && screenPos.x <= GetScreenWidth() &&
            screenPos.y >= 0 && screenPos.y <= GetScreenHeight())
        {
            // Scale based on distance (inverse relationship)
            float scale = 1.0f - (distance / maxDistance);
            int fontSize = (int)(baseSize * scale);

            if (fontSize > 8) // Minimum readable size
            {
                int textWidth = MeasureText(text, fontSize);
                screenPos.x -= textWidth / 2.0f;
                screenPos.y -= fontSize / 2.0f;

                DrawText(text, (int)screenPos.x, (int)screenPos.y, fontSize, color);
            }
        }
    }

    // Draw text with a line connecting to world position
    static void DrawText3DWithLine(const char *text, Vector3 worldPosition, Camera3D camera,
                                   int fontSize, Color textColor, Color lineColor, float lineOffset = 50.0f)
    {
        Vector2 screenPos = GetWorldToScreen(worldPosition, camera);

        Vector3 toCameraDir = Vector3Subtract(worldPosition, camera.position);
        Vector3 cameraForward = Vector3Subtract(camera.target, camera.position);
        float dotProduct = Vector3DotProduct(Vector3Normalize(toCameraDir), Vector3Normalize(cameraForward));

        if (dotProduct > 0.0f && screenPos.x >= 0 && screenPos.x <= GetScreenWidth() &&
            screenPos.y >= 0 && screenPos.y <= GetScreenHeight())
        {
            int textWidth = MeasureText(text, fontSize);

            // Position text above the line endpoint
            Vector2 textPos = {screenPos.x - textWidth / 2.0f, screenPos.y - lineOffset - fontSize};
            Vector2 lineStart = screenPos;
            Vector2 lineEnd = {screenPos.x, screenPos.y - lineOffset};

            // Draw connecting line
            DrawLineEx(lineStart, lineEnd, 2.0f, lineColor);

            // Draw text with background
            int padding = 3;
            DrawRectangle((int)textPos.x - padding, (int)textPos.y - padding,
                          textWidth + padding * 2, fontSize + padding * 2,
                          Fade(BLACK, 0.7f));

            DrawText(text, (int)textPos.x, (int)textPos.y, fontSize, textColor);
        }
    }
};
