#include "PostProcessingMenu.h"
#include <cstdio>

void PostProcessingMenu::Toggle()
{
    isVisible = !isVisible;
}

void PostProcessingMenu::AddBool(std::string name, bool *value)
{
    boolSettings.emplace_back(BoolSetting{std::move(name), value});
}

void PostProcessingMenu::AddFloat(std::string name, float *value, float min, float max, float step)
{
    floatSettings.emplace_back(FloatSetting{std::move(name), value, min, max, step});
}

void PostProcessingMenu::AddInt(std::string name, int *value, int min, int max, int step)
{
    intSettings.emplace_back(IntSetting{std::move(name), value, min, max, step});
}

void PostProcessingMenu::Update()
{
    // Toggle menu with F5
    if (IsKeyPressed(KEY_F5))
        Toggle();

    if (!isVisible)
        return;

    int totalItems = boolSettings.size() + floatSettings.size() + intSettings.size();
    if (totalItems == 0)
        return;

    // Navigation
    if (IsKeyPressed(KEY_UP))
    {
        selectedIndex--;
        if (selectedIndex < 0)
            selectedIndex = totalItems - 1;
    }
    if (IsKeyPressed(KEY_DOWN))
    {
        selectedIndex++;
        if (selectedIndex >= totalItems)
            selectedIndex = 0;
    }

    // Toggle bool or edit values
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        if (selectedIndex < (int)boolSettings.size())
        {
            // Toggle boolean
            *boolSettings[selectedIndex].value = !*boolSettings[selectedIndex].value;
        }
        else
        {
            // Toggle editing mode for numeric values
            editingValue = !editingValue;
        }
    }

    // Edit float value
    if (editingValue && selectedIndex >= (int)boolSettings.size() && selectedIndex < (int)(boolSettings.size() + floatSettings.size()))
    {
        int floatIndex = selectedIndex - boolSettings.size();
        FloatSetting &setting = floatSettings[floatIndex];

        if (IsKeyDown(KEY_LEFT))
            *setting.value -= setting.step;
        if (IsKeyDown(KEY_RIGHT))
            *setting.value += setting.step;

        // Fast adjustment with shift
        if (IsKeyDown(KEY_LEFT_SHIFT))
        {
            if (IsKeyDown(KEY_LEFT))
                *setting.value -= setting.step * 5.0f;
            if (IsKeyDown(KEY_RIGHT))
                *setting.value += setting.step * 5.0f;
        }

        // Clamp value
        if (*setting.value < setting.min)
            *setting.value = setting.min;
        if (*setting.value > setting.max)
            *setting.value = setting.max;
    }

    // Edit int value
    if (editingValue && selectedIndex >= (int)(boolSettings.size() + floatSettings.size()))
    {
        int intIndex = selectedIndex - boolSettings.size() - floatSettings.size();
        IntSetting &setting = intSettings[intIndex];

        if (IsKeyPressed(KEY_LEFT))
            *setting.value -= setting.step;
        if (IsKeyPressed(KEY_RIGHT))
            *setting.value += setting.step;

        // Fast adjustment with shift
        if (IsKeyDown(KEY_LEFT_SHIFT))
        {
            if (IsKeyPressed(KEY_LEFT))
                *setting.value -= setting.step * 5;
            if (IsKeyPressed(KEY_RIGHT))
                *setting.value += setting.step * 5;
        }

        // Clamp value
        if (*setting.value < setting.min)
            *setting.value = setting.min;
        if (*setting.value > setting.max)
            *setting.value = setting.max;
    }
}

void PostProcessingMenu::Draw()
{
    if (!isVisible)
        return;

    int x = 450;
    int y = 100;
    int width = 500;
    int lineHeight = 30;
    int totalItems = boolSettings.size() + floatSettings.size() + intSettings.size();
    int height = (totalItems + 2) * lineHeight + 20;

    // Draw background panel
    DrawRectangle(x - 10, y - 10, width, height, Fade(BLACK, 0.8f));
    DrawRectangleLines(x - 10, y - 10, width, height, SKYBLUE);

    // Title
    DrawText("POST-PROCESSING MENU (F5 to toggle)", x, y, 20, SKYBLUE);
    y += lineHeight;
    DrawText("UP/DOWN: Navigate | ENTER/SPACE: Toggle/Edit | LEFT/RIGHT: Adjust", x, y, 12, GRAY);
    y += lineHeight;

    int currentIndex = 0;

    // Draw boolean settings
    for (const auto &setting : boolSettings)
    {
        Color textColor = (currentIndex == selectedIndex) ? SKYBLUE : WHITE;
        Color valueColor = *setting.value ? GREEN : RED;
        std::string valueText = *setting.value ? "ON" : "OFF";

        DrawText(setting.name.c_str(), x, y, 20, textColor);
        DrawText(valueText.c_str(), x + 300, y, 20, valueColor);

        y += lineHeight;
        currentIndex++;
    }

    // Draw float settings
    for (const auto &setting : floatSettings)
    {
        Color textColor = (currentIndex == selectedIndex) ? SKYBLUE : WHITE;
        bool isEditing = (currentIndex == selectedIndex && editingValue);
        Color valueColor = isEditing ? YELLOW : textColor;

        char valueStr[64];
        snprintf(valueStr, sizeof(valueStr), "%.2f", *setting.value);

        DrawText(setting.name.c_str(), x, y, 20, textColor);
        DrawText(valueStr, x + 300, y, 20, valueColor);

        if (isEditing)
        {
            DrawText("<", x + 280, y, 20, YELLOW);
            DrawText(">", x + 380, y, 20, YELLOW);
        }

        y += lineHeight;
        currentIndex++;
    }

    // Draw int settings
    for (const auto &setting : intSettings)
    {
        Color textColor = (currentIndex == selectedIndex) ? SKYBLUE : WHITE;
        bool isEditing = (currentIndex == selectedIndex && editingValue);
        Color valueColor = isEditing ? YELLOW : textColor;

        char valueStr[64];
        snprintf(valueStr, sizeof(valueStr), "%d", *setting.value);

        DrawText(setting.name.c_str(), x, y, 20, textColor);
        DrawText(valueStr, x + 300, y, 20, valueColor);

        if (isEditing)
        {
            DrawText("<", x + 280, y, 20, YELLOW);
            DrawText(">", x + 380, y, 20, YELLOW);
        }

        y += lineHeight;
        currentIndex++;
    }

    // Draw hint at bottom
    if (totalItems > 0)
    {
        y += 10;
        DrawText("SHIFT + LEFT/RIGHT for faster adjustments", x, y, 12, DARKGRAY);
    }
}
