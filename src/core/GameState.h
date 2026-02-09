/**
 * @file GameState.h
 * @brief Game state management and UI menu system
 */

#pragma once
#include "raylib.h"
#include <functional>
#include <vector>
#include <string>

/**
 * @brief Game state enumeration
 *
 * Defines all possible states the game can be in.
 */
enum class GameState
{
    MAIN_MENU, ///< Main menu screen
    PLAYING,   ///< Active gameplay
    PAUSED,    ///< Game paused
    SETTINGS,  ///< Settings menu
    QUIT       ///< Exit game
};

/**
 * @brief Interactive button for menu interfaces
 */
struct MenuButton
{
    Rectangle bounds;              ///< Button screen bounds
    std::string text;              ///< Button label text
    std::function<void()> onClick; ///< Click callback function
    bool isHovered;                ///< Whether mouse is hovering over button
    bool isPressed;                ///< Whether button is being pressed

    /**
     * @brief Construct a new menu button
     * @param x X position
     * @param y Y position
     * @param width Button width
     * @param height Button height
     * @param buttonText Display text
     * @param callback Function to call on click
     */
    MenuButton(float x, float y, float width, float height, const std::string &buttonText, std::function<void()> callback)
        : bounds{x, y, width, height}, text(buttonText), onClick(callback), isHovered(false), isPressed(false) {}
};

/**
 * @brief Main menu screen implementation
 *
 * Provides the initial game menu with options to start playing,
 * access settings, or quit the game. Includes animations and
 * hover effects for enhanced user experience.
 */
class MainMenu
{
private:
    std::vector<MenuButton> buttons; ///< Menu buttons collection
    int screenWidth;                 ///< Current screen width
    int screenHeight;                ///< Current screen height

    static constexpr int BUTTON_WIDTH = 300;    ///< Button width in pixels
    static constexpr int BUTTON_HEIGHT = 60;    ///< Button height in pixels
    static constexpr int BUTTON_SPACING = 20;   ///< Spacing between buttons
    static constexpr int TITLE_FONT_SIZE = 60;  ///< Title text size
    static constexpr int BUTTON_FONT_SIZE = 30; ///< Button text size

    Color buttonColor = {60, 60, 80, 255};       ///< Default button color
    Color buttonHoverColor = {80, 80, 120, 255}; ///< Button color when hovered
    Color buttonPressColor = {40, 40, 60, 255};  ///< Button color when pressed
    Color buttonTextColor = WHITE;               ///< Button text color
    Color titleColor = WHITE;                    ///< Title text color
    Color backgroundColor = {20, 20, 30, 255};   ///< Background color

    float titleBounce = 0.0f; ///< Title bounce animation timer
    float fadeAlpha = 0.0f;   ///< Fade-in animation alpha
    bool fadeIn = true;       ///< Whether fade-in is active

public:
    /**
     * @brief Construct a new main menu
     */
    MainMenu() : screenWidth(1280), screenHeight(720) {}

    /**
     * @brief Initialize the main menu
     * @param width Screen width in pixels
     * @param height Screen height in pixels
     * @param onPlay Callback for Play button
     * @param onSettings Callback for Settings button
     * @param onQuit Callback for Quit button
     */
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

    /**
     * @brief Update menu state and handle input
     *
     * Updates animations and processes button interactions.
     */
    void Update()
    {
        if (fadeIn && fadeAlpha < 1.0f)
        {
            fadeAlpha += GetFrameTime() * 3.0f;
            if (fadeAlpha > 1.0f)
                fadeAlpha = 1.0f;
        }

        titleBounce += GetFrameTime() * 2.0f;

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

    /**
     * @brief Render the main menu
     *
     * Draws background, title with animations, buttons, and UI elements.
     */
    void Draw()
    {
        DrawRectangle(0, 0, screenWidth, screenHeight, backgroundColor);
        for (int i = 0; i < screenHeight / 2; i++)
        {
            float alpha = (float)i / (screenHeight / 2) * 0.3f * fadeAlpha;
            DrawRectangle(0, i, screenWidth, 1, Fade(PURPLE, alpha));
        }

        const char *title = "Zelda-like Game";
        int titleWidth = MeasureText(title, TITLE_FONT_SIZE);
        float titleY = 120 + sinf(titleBounce) * 10.0f;

        DrawText(title, (screenWidth - titleWidth) / 2 + 3, (int)titleY + 3, TITLE_FONT_SIZE, Fade(BLACK, 0.5f * fadeAlpha));
        DrawText(title, (screenWidth - titleWidth) / 2, (int)titleY, TITLE_FONT_SIZE, Fade(titleColor, fadeAlpha));

        const char *subtitle = "Press a button to continue";
        int subtitleWidth = MeasureText(subtitle, 20);
        DrawText(subtitle, (screenWidth - subtitleWidth) / 2, (int)titleY + TITLE_FONT_SIZE + 10, 20, Fade(GRAY, fadeAlpha));

        for (const auto &button : buttons)
        {
            Color currentColor = button.isPressed ? buttonPressColor : (button.isHovered ? buttonHoverColor : buttonColor);

            DrawRectangleRounded({button.bounds.x + 4, button.bounds.y + 4, button.bounds.width, button.bounds.height},
                                 0.2f, 8, Fade(BLACK, 0.3f * fadeAlpha));

            DrawRectangleRounded(button.bounds, 0.2f, 8, Fade(currentColor, fadeAlpha));

            DrawRectangleRoundedLines(button.bounds, 0.2f, 8, Fade(WHITE, 0.3f * fadeAlpha));
            int textWidth = MeasureText(button.text.c_str(), BUTTON_FONT_SIZE);
            float textX = button.bounds.x + (button.bounds.width - textWidth) / 2;
            float textY = button.bounds.y + (button.bounds.height - BUTTON_FONT_SIZE) / 2;

            DrawText(button.text.c_str(), (int)textX + 2, (int)textY + 2, BUTTON_FONT_SIZE, Fade(BLACK, 0.5f * fadeAlpha));
            DrawText(button.text.c_str(), (int)textX, (int)textY, BUTTON_FONT_SIZE, Fade(buttonTextColor, fadeAlpha));
            if (button.isHovered)
            {
                DrawRectangleRoundedLines(button.bounds, 0.2f, 8, Fade(WHITE, 0.6f * fadeAlpha));
            }
        }

        const char *footer = "Made with Raylib | 2025";
        int footerWidth = MeasureText(footer, 16);
        DrawText(footer, (screenWidth - footerWidth) / 2, screenHeight - 40, 16, Fade(DARKGRAY, fadeAlpha));
    }

    /**
     * @brief Reset menu animations
     *
     * Resets fade-in animation to initial state.
     */
    void Reset()
    {
        fadeAlpha = 0.0f;
        fadeIn = true;
    }
};

/**
 * @brief Pause menu screen implementation
 *
 * Displayed when the game is paused, offering options to resume,
 * access settings, return to main menu, or quit.
 */
class PauseMenu
{
private:
    std::vector<MenuButton> buttons; ///< Menu buttons collection
    int screenWidth;                 ///< Current screen width
    int screenHeight;                ///< Current screen height

    static constexpr int BUTTON_WIDTH = 250;  ///< Button width
    static constexpr int BUTTON_HEIGHT = 50;  ///< Button height
    static constexpr int BUTTON_SPACING = 15; ///< Spacing between buttons

public:
    /**
     * @brief Construct a new pause menu
     */
    PauseMenu() : screenWidth(1280), screenHeight(720) {}

    /**
     * @brief Initialize the pause menu
     * @param width Screen width
     * @param height Screen height
     * @param onResume Callback to resume game
     * @param onSettings Callback to open settings
     * @param onMainMenu Callback to return to main menu
     * @param onQuit Callback to quit game
     */
    void Init(int width, int height, std::function<void()> onResume, std::function<void()> onSettings, std::function<void()> onMainMenu, std::function<void()> onQuit)
    {
        screenWidth = width;
        screenHeight = height;
        buttons.clear();

        float centerX = (screenWidth - BUTTON_WIDTH) / 2.0f;
        float startY = screenHeight / 2.0f - BUTTON_HEIGHT;

        buttons.emplace_back(centerX, startY, BUTTON_WIDTH, BUTTON_HEIGHT, "Resume", onResume);
        buttons.emplace_back(centerX, startY + BUTTON_HEIGHT + BUTTON_SPACING, BUTTON_WIDTH, BUTTON_HEIGHT, "Settings", onSettings);
        buttons.emplace_back(centerX, startY + (BUTTON_HEIGHT + BUTTON_SPACING) * 2, BUTTON_WIDTH, BUTTON_HEIGHT, "Main Menu", onMainMenu);
        buttons.emplace_back(centerX, startY + (BUTTON_HEIGHT + BUTTON_SPACING) * 3, BUTTON_WIDTH, BUTTON_HEIGHT, "Quit", onQuit);
    }

    /**
     * @brief Update pause menu and handle input
     */
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

    /**
     * @brief Render the pause menu
     */
    void Draw()
    {
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
        const char *title = "PAUSED";
        int titleWidth = MeasureText(title, 50);
        DrawText(title, (screenWidth - titleWidth) / 2, screenHeight / 3, 50, WHITE);

        for (const auto &button : buttons)
        {
            Color currentColor = button.isPressed ? Color{40, 40, 60, 255} : (button.isHovered ? Color{80, 80, 120, 255} : Color{60, 60, 80, 255});

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

/**
 * @brief Settings menu screen implementation
 *
 * Provides interface for configuring game settings,
 * currently supporting display mode changes.
 */
class SettingsMenu
{
private:
    int screenWidth;  ///< Screen width
    int screenHeight; ///< Screen height

    int *fullscreenModePtr = nullptr;        ///< Pointer to fullscreen mode variable
    std::vector<std::string> displayOptions; ///< Available display mode options
    std::function<void()> onBack;            ///< Back button callback

    Rectangle backButton;       ///< Back button bounds
    Rectangle leftArrowButton;  ///< Left arrow button bounds
    Rectangle rightArrowButton; ///< Right arrow button bounds

    static constexpr int TITLE_FONT_SIZE = 48;
    static constexpr int LABEL_FONT_SIZE = 28;  ///< Label text size
    static constexpr int VALUE_FONT_SIZE = 28;  ///< Value text size
    static constexpr int BUTTON_FONT_SIZE = 24; ///< Button text size
    static constexpr int ARROW_FONT_SIZE = 32;  ///< Arrow text size

    static constexpr int TITLE_Y = 60;               ///< Title Y position
    static constexpr int DISPLAY_MODE_Y = 160;       ///< Display mode setting Y position
    static constexpr int DISPLAY_MODE_LABEL_X = 100; ///< Display mode label X position
    static constexpr int DISPLAY_MODE_VALUE_X = 360; ///< Display mode value X position

    static constexpr float BACK_BUTTON_MARGIN = 20.0f;        ///< Back button margin
    static constexpr float BACK_BUTTON_BOTTOM_OFFSET = 80.0f; ///< Back button offset from bottom
    static constexpr float BACK_BUTTON_WIDTH = 200.0f;        ///< Back button width
    static constexpr float BACK_BUTTON_HEIGHT = 50.0f;        ///< Back button height

    static constexpr float ARROW_BUTTON_SIZE = 43.0f;    ///< Arrow button size
    static constexpr float ARROW_LEFT_OFFSET = 56.0f;    ///< Left arrow X offset
    static constexpr float ARROW_RIGHT_OFFSET = 12.0f;   ///< Right arrow X offset
    static constexpr float ARROW_VERTICAL_OFFSET = 8.0f; ///< Arrow vertical offset

    static constexpr Color BACKGROUND_COLOR = {0, 0, 0, 153};         ///< Background overlay color
    static constexpr Color BUTTON_COLOR = {60, 60, 80, 255};          ///< Default button color
    static constexpr Color BUTTON_HOVER_COLOR = {135, 206, 235, 255}; ///< Hovered button color
    static constexpr Color ARROW_BUTTON_COLOR = {80, 80, 100, 255};   ///< Arrow button color

    /**
     * @brief Update arrow button positions based on current value width
     */
    void UpdateArrowButtonPositions();

    /**
     * @brief Draw an arrow button
     * @param button Button bounds
     * @param arrow Arrow text (< or >)
     * @param isHovered Whether button is hovered
     */
    void DrawArrowButton(const Rectangle &button, const char *arrow, bool isHovered) const;

    /**
     * @brief Check if button is hovered by mouse
     * @param button Button bounds to check
     * @return True if mouse is over button
     */
    bool IsButtonHovered(const Rectangle &button) const;

public:
    /**
     * @brief Construct a new settings menu
     */
    SettingsMenu() : screenWidth(1280), screenHeight(720) {}

    /**
     * @brief Initialize the settings menu
     * @param width Screen width
     * @param height Screen height
     * @param fullscreenPtr Pointer to fullscreen mode variable
     * @param backCallback Callback for back button
     */
    void Init(int width, int height, int *fullscreenPtr, std::function<void()> backCallback)
    {
        screenWidth = width;
        screenHeight = height;
        fullscreenModePtr = fullscreenPtr;
        onBack = backCallback;

        displayOptions = {"Windowed", "Fullscreen", "Borderless"};

        backButton = {
            BACK_BUTTON_MARGIN,
            (float)(screenHeight)-BACK_BUTTON_BOTTOM_OFFSET,
            BACK_BUTTON_WIDTH,
            BACK_BUTTON_HEIGHT};

        leftArrowButton = {0.0f, 0.0f, ARROW_BUTTON_SIZE, ARROW_BUTTON_SIZE};
        rightArrowButton = {0.0f, 0.0f, ARROW_BUTTON_SIZE, ARROW_BUTTON_SIZE};
    }

    /**
     * @brief Update settings menu and handle input
     */
    void Update()
    {
        if (!fullscreenModePtr)
            return;

        UpdateArrowButtonPositions();
        Vector2 mousePos = GetMousePosition();
        if (IsButtonHovered(backButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            if (onBack)
                onBack();
            return;
        }
        if (IsButtonHovered(leftArrowButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            (*fullscreenModePtr)--;
            if (*fullscreenModePtr < 0)
                *fullscreenModePtr = (int)displayOptions.size() - 1;
        }
        if (IsButtonHovered(rightArrowButton) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            (*fullscreenModePtr)++;
            if (*fullscreenModePtr >= (int)displayOptions.size())
                *fullscreenModePtr = 0;
        }
    }

    /**
     * @brief Render the settings menu
     */
    void Draw()
    {
        DrawRectangle(0, 0, screenWidth, screenHeight, BACKGROUND_COLOR);

        const char *title = "Settings";
        int titleWidth = MeasureText(title, TITLE_FONT_SIZE);
        DrawText(title, (screenWidth - titleWidth) / 2, TITLE_Y, TITLE_FONT_SIZE, WHITE);

        DrawText("Display Mode:", DISPLAY_MODE_LABEL_X, DISPLAY_MODE_Y, LABEL_FONT_SIZE, LIGHTGRAY);

        if (fullscreenModePtr && *fullscreenModePtr >= 0 && *fullscreenModePtr < (int)displayOptions.size())
        {
            const char *value = displayOptions[*fullscreenModePtr].c_str();
            DrawText(value, DISPLAY_MODE_VALUE_X, DISPLAY_MODE_Y, VALUE_FONT_SIZE, SKYBLUE);

            UpdateArrowButtonPositions();
            DrawArrowButton(leftArrowButton, "<", IsButtonHovered(leftArrowButton));
            DrawArrowButton(rightArrowButton, ">", IsButtonHovered(rightArrowButton));
        }

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

inline bool SettingsMenu::IsButtonHovered(const Rectangle &button) const
{
    return CheckCollisionPointRec(GetMousePosition(), button);
}

inline void SettingsMenu::DrawArrowButton(const Rectangle &button, const char *arrow, bool isHovered) const
{
    Color buttonColor = isHovered ? BUTTON_HOVER_COLOR : ARROW_BUTTON_COLOR;
    DrawRectangleRounded(button, 0.15f, 6, buttonColor);
    DrawRectangleRoundedLines(button, 0.15f, 6, Fade(WHITE, 0.6f));

    int arrowWidth = MeasureText(arrow, ARROW_FONT_SIZE);
    float arrowX = button.x + (button.width - arrowWidth) / 2;
    float arrowY = button.y + (button.height - ARROW_FONT_SIZE) / 2 + 2;
    DrawText(arrow, (int)arrowX, (int)arrowY, ARROW_FONT_SIZE, WHITE);
}

inline void SettingsMenu::UpdateArrowButtonPositions()
{
    if (!fullscreenModePtr || *fullscreenModePtr < 0 || *fullscreenModePtr >= (int)displayOptions.size())
    {
        leftArrowButton.x = DISPLAY_MODE_VALUE_X - ARROW_LEFT_OFFSET;
        leftArrowButton.y = DISPLAY_MODE_Y - ARROW_VERTICAL_OFFSET;
        rightArrowButton.x = DISPLAY_MODE_VALUE_X + 120.0f;
        rightArrowButton.y = DISPLAY_MODE_Y - ARROW_VERTICAL_OFFSET;
        return;
    }

    const std::string &valueText = displayOptions[*fullscreenModePtr];
    int valueWidth = MeasureText(valueText.c_str(), VALUE_FONT_SIZE);

    leftArrowButton.x = DISPLAY_MODE_VALUE_X - ARROW_LEFT_OFFSET;
    leftArrowButton.y = DISPLAY_MODE_Y - ARROW_VERTICAL_OFFSET;

    rightArrowButton.x = DISPLAY_MODE_VALUE_X + valueWidth + ARROW_RIGHT_OFFSET;
    rightArrowButton.y = DISPLAY_MODE_Y - ARROW_VERTICAL_OFFSET;
}