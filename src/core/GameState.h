#pragma once
#include "raylib.h"
#include <functional>
#include <vector>
#include <string>

// Game state enumeration
enum class GameState
{
    MAIN_MENU,
    PLAYING,
    PAUSED,
    SETTINGS,
    QUIT
};

// Simple button structure for menu
struct MenuButton
{
    Rectangle bounds;
    std::string text;
    std::function<void()> onClick;
    bool isHovered;
    bool isPressed;

    MenuButton(float x, float y, float width, float height, const std::string &buttonText, std::function<void()> callback)
        : bounds{x, y, width, height}, text(buttonText), onClick(callback), isHovered(false), isPressed(false) {}
};

// Main menu class
class MainMenu
{
private:
    std::vector<MenuButton> buttons;
    int screenWidth;
    int screenHeight;

    // Visual settings
    static constexpr int BUTTON_WIDTH = 300;
    static constexpr int BUTTON_HEIGHT = 60;
    static constexpr int BUTTON_SPACING = 20;
    static constexpr int TITLE_FONT_SIZE = 60;
    static constexpr int BUTTON_FONT_SIZE = 30;

    // Colors
    Color buttonColor = {60, 60, 80, 255};
    Color buttonHoverColor = {80, 80, 120, 255};
    Color buttonPressColor = {40, 40, 60, 255};
    Color buttonTextColor = WHITE;
    Color titleColor = WHITE;
    Color backgroundColor = {20, 20, 30, 255};

    // Animation
    float titleBounce = 0.0f;
    float fadeAlpha = 0.0f;
    bool fadeIn = true;

public:
    MainMenu() : screenWidth(1280), screenHeight(720) {}

    void Init(int width, int height, std::function<void()> onPlay, std::function<void()> onSettings, std::function<void()> onQuit)
    {
        screenWidth = width;
        screenHeight = height;
        buttons.clear();

        // Calculate center position for buttons
        float centerX = (screenWidth - BUTTON_WIDTH) / 2.0f;
        float startY = screenHeight / 2.0f - BUTTON_HEIGHT;

        // Create buttons
        buttons.emplace_back(centerX, startY, BUTTON_WIDTH, BUTTON_HEIGHT, "Play", onPlay);
        buttons.emplace_back(centerX, startY + BUTTON_HEIGHT + BUTTON_SPACING, BUTTON_WIDTH, BUTTON_HEIGHT, "Settings", onSettings);
        buttons.emplace_back(centerX, startY + (BUTTON_HEIGHT + BUTTON_SPACING) * 2, BUTTON_WIDTH, BUTTON_HEIGHT, "Quit", onQuit);

        fadeAlpha = 0.0f;
        fadeIn = true;
    }

    void Update()
    {
        // Update fade animation
        if (fadeIn && fadeAlpha < 1.0f)
        {
            fadeAlpha += GetFrameTime() * 3.0f;
            if (fadeAlpha > 1.0f)
                fadeAlpha = 1.0f;
        }

        // Update title bounce animation
        titleBounce += GetFrameTime() * 2.0f;

        // Get mouse position
        Vector2 mousePos = GetMousePosition();

        // Update buttons
        for (auto &button : buttons)
        {
            button.isHovered = CheckCollisionPointRec(mousePos, button.bounds);
            button.isPressed = button.isHovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);

            // Check for click
            if (button.isHovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            {
                if (button.onClick)
                {
                    button.onClick();
                }
            }
        }
    }

    void Draw()
    {
        // Draw background
        DrawRectangle(0, 0, screenWidth, screenHeight, backgroundColor);

        // Draw decorative gradient
        for (int i = 0; i < screenHeight / 2; i++)
        {
            float alpha = (float)i / (screenHeight / 2) * 0.3f * fadeAlpha;
            DrawRectangle(0, i, screenWidth, 1, Fade(PURPLE, alpha));
        }

        // Draw title with bounce effect
        const char *title = "Zelda-like Game";
        int titleWidth = MeasureText(title, TITLE_FONT_SIZE);
        float titleY = 120 + sinf(titleBounce) * 10.0f;
        
        // Title shadow
        DrawText(title, (screenWidth - titleWidth) / 2 + 3, (int)titleY + 3, TITLE_FONT_SIZE, Fade(BLACK, 0.5f * fadeAlpha));
        // Title
        DrawText(title, (screenWidth - titleWidth) / 2, (int)titleY, TITLE_FONT_SIZE, Fade(titleColor, fadeAlpha));

        // Draw subtitle
        const char *subtitle = "Press a button to continue";
        int subtitleWidth = MeasureText(subtitle, 20);
        DrawText(subtitle, (screenWidth - subtitleWidth) / 2, (int)titleY + TITLE_FONT_SIZE + 10, 20, Fade(GRAY, fadeAlpha));

        // Draw buttons
        for (const auto &button : buttons)
        {
            Color currentColor = button.isPressed ? buttonPressColor : (button.isHovered ? buttonHoverColor : buttonColor);
            
            // Button shadow
            DrawRectangleRounded({button.bounds.x + 4, button.bounds.y + 4, button.bounds.width, button.bounds.height}, 
                                 0.2f, 8, Fade(BLACK, 0.3f * fadeAlpha));
            
            // Button background
            DrawRectangleRounded(button.bounds, 0.2f, 8, Fade(currentColor, fadeAlpha));
            
            // Button border
            DrawRectangleRoundedLines(button.bounds, 0.2f, 8, Fade(WHITE, 0.3f * fadeAlpha));

            // Button text
            int textWidth = MeasureText(button.text.c_str(), BUTTON_FONT_SIZE);
            float textX = button.bounds.x + (button.bounds.width - textWidth) / 2;
            float textY = button.bounds.y + (button.bounds.height - BUTTON_FONT_SIZE) / 2;
            
            // Text shadow
            DrawText(button.text.c_str(), (int)textX + 2, (int)textY + 2, BUTTON_FONT_SIZE, Fade(BLACK, 0.5f * fadeAlpha));
            // Text
            DrawText(button.text.c_str(), (int)textX, (int)textY, BUTTON_FONT_SIZE, Fade(buttonTextColor, fadeAlpha));

            // Hover effect - draw highlight
            if (button.isHovered)
            {
                DrawRectangleRoundedLines(button.bounds, 0.2f, 8, Fade(WHITE, 0.6f * fadeAlpha));
            }
        }

        // Draw footer
        const char *footer = "Made with Raylib | 2025";
        int footerWidth = MeasureText(footer, 16);
        DrawText(footer, (screenWidth - footerWidth) / 2, screenHeight - 40, 16, Fade(DARKGRAY, fadeAlpha));
    }

    void Reset()
    {
        fadeAlpha = 0.0f;
        fadeIn = true;
    }
};

// Pause menu class
class PauseMenu
{
private:
    std::vector<MenuButton> buttons;
    int screenWidth;
    int screenHeight;

    static constexpr int BUTTON_WIDTH = 250;
    static constexpr int BUTTON_HEIGHT = 50;
    static constexpr int BUTTON_SPACING = 15;

public:
    PauseMenu() : screenWidth(1280), screenHeight(720) {}

    void Init(int width, int height, std::function<void()> onResume, std::function<void()> onSettings, std::function<void()> onMainMenu, std::function<void()> onQuit)
    {
        screenWidth = width;
        screenHeight = height;
        buttons.clear();

        float centerX = (screenWidth - BUTTON_WIDTH) / 2.0f;
        float startY = screenHeight / 2.0f - BUTTON_HEIGHT;

        // Order: Resume | Settings | Main Menu | Quit
        buttons.emplace_back(centerX, startY, BUTTON_WIDTH, BUTTON_HEIGHT, "Resume", onResume);
        buttons.emplace_back(centerX, startY + BUTTON_HEIGHT + BUTTON_SPACING, BUTTON_WIDTH, BUTTON_HEIGHT, "Settings", onSettings);
        buttons.emplace_back(centerX, startY + (BUTTON_HEIGHT + BUTTON_SPACING) * 2, BUTTON_WIDTH, BUTTON_HEIGHT, "Main Menu", onMainMenu);
        buttons.emplace_back(centerX, startY + (BUTTON_HEIGHT + BUTTON_SPACING) * 3, BUTTON_WIDTH, BUTTON_HEIGHT, "Quit", onQuit);
    }

    void Update()
    {
        Vector2 mousePos = GetMousePosition();

        for (auto &button : buttons)
        {
            button.isHovered = CheckCollisionPointRec(mousePos, button.bounds);
            button.isPressed = button.isHovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);

            if (button.isHovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
            {
                if (button.onClick)
                {
                    button.onClick();
                }
            }
        }
    }

    void Draw()
    {
        // Semi-transparent overlay
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));

        // Pause title
        const char *title = "PAUSED";
        int titleWidth = MeasureText(title, 50);
        DrawText(title, (screenWidth - titleWidth) / 2, screenHeight / 3, 50, WHITE);

        // Draw buttons
        for (const auto &button : buttons)
        {
            Color currentColor = button.isPressed ? Color{40, 40, 60, 255} : 
                                (button.isHovered ? Color{80, 80, 120, 255} : Color{60, 60, 80, 255});
            
            DrawRectangleRounded(button.bounds, 0.2f, 8, currentColor);
            DrawRectangleRoundedLines(button.bounds, 0.2f, 8, Fade(WHITE, 0.3f));

            int textWidth = MeasureText(button.text.c_str(), 24);
            float textX = button.bounds.x + (button.bounds.width - textWidth) / 2;
            float textY = button.bounds.y + (button.bounds.height - 24) / 2;
            DrawText(button.text.c_str(), (int)textX, (int)textY, 24, WHITE);

            if (button.isHovered)
            {
                DrawRectangleRoundedLines(button.bounds, 0.2f, 8, Fade(WHITE, 0.6f));
            }
        }
    }
};

// Settings menu class
class SettingsMenu
{
private:
    // Screen dimensions
    int screenWidth;
    int screenHeight;
    
    // Settings data
    int *fullscreenModePtr = nullptr;
    std::vector<std::string> displayOptions;
    std::function<void()> onBack;

    // UI elements
    Rectangle backButton;
    Rectangle leftArrowButton;
    Rectangle rightArrowButton;
    
    // Layout constants
    static constexpr int TITLE_FONT_SIZE = 48;
    static constexpr int LABEL_FONT_SIZE = 28;
    static constexpr int VALUE_FONT_SIZE = 28;
    static constexpr int BUTTON_FONT_SIZE = 24;
    static constexpr int ARROW_FONT_SIZE = 32;
    
    static constexpr int TITLE_Y = 60;
    static constexpr int DISPLAY_MODE_Y = 160;
    static constexpr int DISPLAY_MODE_LABEL_X = 100;
    static constexpr int DISPLAY_MODE_VALUE_X = 360;
    
    static constexpr float BACK_BUTTON_MARGIN = 20.0f;
    static constexpr float BACK_BUTTON_BOTTOM_OFFSET = 80.0f;
    static constexpr float BACK_BUTTON_WIDTH = 200.0f;
    static constexpr float BACK_BUTTON_HEIGHT = 50.0f;
    
    static constexpr float ARROW_BUTTON_SIZE = 43.0f;
    static constexpr float ARROW_LEFT_OFFSET = 56.0f;
    static constexpr float ARROW_RIGHT_OFFSET = 12.0f;
    static constexpr float ARROW_VERTICAL_OFFSET = 8.0f;
    
    // Colors
    static constexpr Color BACKGROUND_COLOR = {0, 0, 0, 153}; // Fade(BLACK, 0.6f)
    static constexpr Color BUTTON_COLOR = {60, 60, 80, 255};
    static constexpr Color BUTTON_HOVER_COLOR = {135, 206, 235, 255}; // SKYBLUE
    static constexpr Color ARROW_BUTTON_COLOR = {80, 80, 100, 255};
    
    // Helper methods
    void UpdateArrowButtonPositions();
    void DrawArrowButton(const Rectangle& button, const char* arrow, bool isHovered) const;
    bool IsButtonHovered(const Rectangle& button) const;

public:
    SettingsMenu() : screenWidth(1280), screenHeight(720) {}

    void Init(int width, int height, int *fullscreenPtr, std::function<void()> backCallback)
    {
        screenWidth = width;
        screenHeight = height;
        fullscreenModePtr = fullscreenPtr;
        onBack = backCallback;

        displayOptions = {"Windowed", "Fullscreen", "Borderless"};

        // Position back button at bottom-left
        backButton = {
            BACK_BUTTON_MARGIN,
            (float)(screenHeight) - BACK_BUTTON_BOTTOM_OFFSET,
            BACK_BUTTON_WIDTH,
            BACK_BUTTON_HEIGHT
        };

        // Arrow buttons positioned dynamically in UpdateArrowButtonPositions()
        leftArrowButton = {0.0f, 0.0f, ARROW_BUTTON_SIZE, ARROW_BUTTON_SIZE};
        rightArrowButton = {0.0f, 0.0f, ARROW_BUTTON_SIZE, ARROW_BUTTON_SIZE};
    }

    void Update()
    {
        if (!fullscreenModePtr)
            return;
            
        UpdateArrowButtonPositions();
        Vector2 mousePos = GetMousePosition();

        // Handle back button
        if (IsButtonHovered(backButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            if (onBack)
                onBack();
            return;
        }

        // Handle left arrow (previous option)
        if (IsButtonHovered(leftArrowButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            (*fullscreenModePtr)--;
            if (*fullscreenModePtr < 0)
                *fullscreenModePtr = (int)displayOptions.size() - 1;
        }

        // Handle right arrow (next option)
        if (IsButtonHovered(rightArrowButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            (*fullscreenModePtr)++;
            if (*fullscreenModePtr >= (int)displayOptions.size())
                *fullscreenModePtr = 0;
        }
    }

    void Draw()
    {
        // Semi-transparent background overlay
        DrawRectangle(0, 0, screenWidth, screenHeight, BACKGROUND_COLOR);

        // Title
        const char *title = "Settings";
        int titleWidth = MeasureText(title, TITLE_FONT_SIZE);
        DrawText(title, (screenWidth - titleWidth) / 2, TITLE_Y, TITLE_FONT_SIZE, WHITE);

        // Display mode setting
        DrawText("Display Mode:", DISPLAY_MODE_LABEL_X, DISPLAY_MODE_Y, LABEL_FONT_SIZE, LIGHTGRAY);
        
        if (fullscreenModePtr && *fullscreenModePtr >= 0 && *fullscreenModePtr < (int)displayOptions.size())
        {
            const char *value = displayOptions[*fullscreenModePtr].c_str();
            DrawText(value, DISPLAY_MODE_VALUE_X, DISPLAY_MODE_Y, VALUE_FONT_SIZE, SKYBLUE);

            // Draw arrow buttons
            UpdateArrowButtonPositions();
            DrawArrowButton(leftArrowButton, "<", IsButtonHovered(leftArrowButton));
            DrawArrowButton(rightArrowButton, ">", IsButtonHovered(rightArrowButton));
        }

        // Back button
        bool backHovered = IsButtonHovered(backButton);
        Color backColor = backHovered ? BUTTON_HOVER_COLOR : BUTTON_COLOR;
        DrawRectangleRounded(backButton, 0.2f, 8, backColor);
        DrawRectangleRoundedLines(backButton, 0.2f, 8, WHITE);
        
        int backTextWidth = MeasureText("Back", BUTTON_FONT_SIZE);
        float backTextX = backButton.x + (backButton.width - backTextWidth) / 2;
        float backTextY = backButton.y + (backButton.height - BUTTON_FONT_SIZE) / 2;
        DrawText("Back", (int)backTextX, (int)backTextY, BUTTON_FONT_SIZE, WHITE);
    }
};

// SettingsMenu helper method implementations
inline bool SettingsMenu::IsButtonHovered(const Rectangle& button) const
{
    return CheckCollisionPointRec(GetMousePosition(), button);
}

inline void SettingsMenu::DrawArrowButton(const Rectangle& button, const char* arrow, bool isHovered) const
{
    Color buttonColor = isHovered ? BUTTON_HOVER_COLOR : ARROW_BUTTON_COLOR;
    DrawRectangleRounded(button, 0.15f, 6, buttonColor);
    DrawRectangleRoundedLines(button, 0.15f, 6, Fade(WHITE, 0.6f));
    
    int arrowWidth = MeasureText(arrow, ARROW_FONT_SIZE);
    float arrowX = button.x + (button.width - arrowWidth) / 2;
    float arrowY = button.y + (button.height - ARROW_FONT_SIZE) / 2 + 2; // +2 for slight vertical centering
    DrawText(arrow, (int)arrowX, (int)arrowY, ARROW_FONT_SIZE, WHITE);
}

inline void SettingsMenu::UpdateArrowButtonPositions()
{
    if (!fullscreenModePtr || *fullscreenModePtr < 0 || *fullscreenModePtr >= (int)displayOptions.size())
    {
        // Fallback: position arrows with default spacing
        leftArrowButton.x = DISPLAY_MODE_VALUE_X - ARROW_LEFT_OFFSET;
        leftArrowButton.y = DISPLAY_MODE_Y - ARROW_VERTICAL_OFFSET;
        rightArrowButton.x = DISPLAY_MODE_VALUE_X + 120.0f;
        rightArrowButton.y = DISPLAY_MODE_Y - ARROW_VERTICAL_OFFSET;
        return;
    }

    // Position arrows based on actual text width
    const std::string &valueText = displayOptions[*fullscreenModePtr];
    int valueWidth = MeasureText(valueText.c_str(), VALUE_FONT_SIZE);

    leftArrowButton.x = DISPLAY_MODE_VALUE_X - ARROW_LEFT_OFFSET;
    leftArrowButton.y = DISPLAY_MODE_Y - ARROW_VERTICAL_OFFSET;
    
    rightArrowButton.x = DISPLAY_MODE_VALUE_X + valueWidth + ARROW_RIGHT_OFFSET;
    rightArrowButton.y = DISPLAY_MODE_Y - ARROW_VERTICAL_OFFSET;}