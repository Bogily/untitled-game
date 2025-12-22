#include "DebugMenu.h"
#include <cstdio>

void DebugMenu::Toggle()
{
    isVisible = !isVisible;
}

void DebugMenu::AddBool(std::string name, bool *value)
{
    boolSettings.emplace_back(BoolSetting{std::move(name), value});
}

void DebugMenu::AddFloat(std::string name, float *value, float min, float max, float step)
{
    floatSettings.emplace_back(FloatSetting{std::move(name), value, min, max, step});
}

void DebugMenu::AddString(std::string name, int *selectedIndex, std::vector<std::string> options)
{
    stringSettings.emplace_back(StringSetting{std::move(name), selectedIndex, std::move(options)});
}

void DebugMenu::Update()
{
    // Toggle menu with F1
    if (IsKeyPressed(KEY_TAB))
        Toggle();

    if (!isVisible)
        return;

    int totalItems = boolSettings.size() + floatSettings.size() + stringSettings.size();
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

    // Toggle bool or edit float/string
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        if (selectedIndex < (int)boolSettings.size())
        {
            // Toggle boolean
            *boolSettings[selectedIndex].value = !*boolSettings[selectedIndex].value;
        }
        else if (selectedIndex < (int)(boolSettings.size() + floatSettings.size()))
        {
            // Toggle editing mode for float
            editingValue = !editingValue;
        }
        else
        {
            // Toggle editing mode for string
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

    // Edit string value (cycle through options)
    if (editingValue && selectedIndex >= (int)(boolSettings.size() + floatSettings.size()))
    {
        int stringIndex = selectedIndex - boolSettings.size() - floatSettings.size();
        StringSetting &setting = stringSettings[stringIndex];

        if (IsKeyPressed(KEY_LEFT))
        {
            (*setting.selectedIndex)--;
            if (*setting.selectedIndex < 0)
                *setting.selectedIndex = setting.options.size() - 1;
        }
        if (IsKeyPressed(KEY_RIGHT))
        {
            (*setting.selectedIndex)++;
            if (*setting.selectedIndex >= (int)setting.options.size())
                *setting.selectedIndex = 0;
        }
    }
}

void DebugMenu::Draw()
{
    if (!isVisible)
        return;

    int x = 20;
    int y = 100;
    int width = 400;
    int lineHeight = 30;
    int totalItems = boolSettings.size() + floatSettings.size() + stringSettings.size();
    int height = (totalItems + 2) * lineHeight + 20;

    // Draw background panel
    DrawRectangle(x - 10, y - 10, width, height, Fade(BLACK, 0.8f));
    DrawRectangleLines(x - 10, y - 10, width, height, YELLOW);

    // Title
    DrawText("DEBUG MENU (F1 to toggle)", x, y, 20, YELLOW);
    y += lineHeight;
    DrawText("UP/DOWN: Navigate | ENTER/SPACE: Toggle/Edit | LEFT/RIGHT: Adjust", x, y, 12, GRAY);
    y += lineHeight;

    int currentIndex = 0;

    // Draw boolean settings
    for (const auto &setting : boolSettings)
    {
        Color textColor = (currentIndex == selectedIndex) ? YELLOW : WHITE;
        Color valueColor = *setting.value ? GREEN : RED;
        std::string valueText = *setting.value ? "ON" : "OFF";

        DrawText(setting.name.c_str(), x, y, 20, textColor);
        DrawText(valueText.c_str(), x + 280, y, 20, valueColor);

        if (currentIndex == selectedIndex)
            DrawText(">", x - 20, y, 20, YELLOW);

        y += lineHeight;
        currentIndex++;
    }

    // Draw float settings
    for (const auto &setting : floatSettings)
    {
        Color textColor = (currentIndex == selectedIndex) ? YELLOW : WHITE;
        Color valueColor = (currentIndex == selectedIndex && editingValue) ? GREEN : SKYBLUE;

        char valueText[32];
        snprintf(valueText, sizeof(valueText), "%.2f", *setting.value);

        DrawText(setting.name.c_str(), x, y, 20, textColor);
        DrawText(valueText, x + 280, y, 20, valueColor);

        if (currentIndex == selectedIndex)
        {
            DrawText(">", x - 20, y, 20, YELLOW);
            if (editingValue)
                DrawText("<- EDITING ->", x + 320, y, 15, GREEN);
        }

        y += lineHeight;
        currentIndex++;
    }

    // Draw string settings
    for (const auto &setting : stringSettings)
    {
        Color textColor = (currentIndex == selectedIndex) ? YELLOW : WHITE;
        Color valueColor = (currentIndex == selectedIndex && editingValue) ? GREEN : SKYBLUE;

        const char *valueText = setting.options[*setting.selectedIndex].c_str();

        DrawText(setting.name.c_str(), x, y, 20, textColor);
        DrawText(valueText, x + 280, y, 20, valueColor);

        if (currentIndex == selectedIndex)
        {
            DrawText(">", x - 20, y, 20, YELLOW);
            if (editingValue)
                DrawText("<- EDITING ->", x + 320, y, 15, GREEN);
        }

        y += lineHeight;
        currentIndex++;
    }

    // Instructions at bottom
    DrawText("Press F1 to close menu", x, y + 10, 15, GRAY);
}
