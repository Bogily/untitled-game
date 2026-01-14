#include "UIMenu.h"
#include <cstdio>

void UIMenu::Toggle()
{
    isVisible = !isVisible;
}

void UIMenu::AddBool(std::string name, bool *value)
{
    boolSettings.emplace_back(BoolSetting{std::move(name), value});
}

void UIMenu::AddFloat(std::string name, float *value, float min, float max, float step)
{
    floatSettings.emplace_back(FloatSetting{std::move(name), value, min, max, step});
}

void UIMenu::AddInt(std::string name, int *value, int min, int max, int step)
{
    intSettings.emplace_back(IntSetting{std::move(name), value, min, max, step});
}

void UIMenu::AddString(std::string name, int *selectedIndex, std::vector<std::string> options)
{
    stringSettings.emplace_back(StringSetting{std::move(name), selectedIndex, std::move(options)});
}

int UIMenu::GetTotalItems() const
{
    return boolSettings.size() + floatSettings.size() + intSettings.size() + stringSettings.size();
}

void UIMenu::HandleNavigation()
{
    int totalItems = GetTotalItems();
    if (totalItems == 0)
        return;

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
}

void UIMenu::HandleSelection()
{
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        int boolCount = boolSettings.size();
        int floatCount = floatSettings.size();
        int intCount = intSettings.size();

        if (selectedIndex < boolCount)
        {
            // Toggle boolean
            *boolSettings[selectedIndex].value = !*boolSettings[selectedIndex].value;
        }
        else if (selectedIndex < boolCount + floatCount)
        {
            // Toggle editing mode for float
            editingValue = !editingValue;
        }
        else if (selectedIndex < boolCount + floatCount + intCount)
        {
            // Toggle editing mode for int
            editingValue = !editingValue;
        }
        else
        {
            // Toggle editing mode for string
            editingValue = !editingValue;
        }
    }
}

void UIMenu::HandleFloatEdit(int floatIndex)
{
    if (floatIndex < 0 || floatIndex >= (int)floatSettings.size())
        return;

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

void UIMenu::HandleIntEdit(int intIndex)
{
    if (intIndex < 0 || intIndex >= (int)intSettings.size())
        return;

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

void UIMenu::HandleStringEdit(int stringIndex)
{
    if (stringIndex < 0 || stringIndex >= (int)stringSettings.size())
        return;

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

void UIMenu::DrawBoolSettings(int &x, int &y, int lineHeight, Color highlightColor) const
{
    for (const auto &setting : boolSettings)
    {
        Color textColor = (currentDrawIndex == selectedIndex) ? highlightColor : WHITE;
        Color valueColor = *setting.value ? GREEN : RED;
        std::string valueText = *setting.value ? "ON" : "OFF";

        DrawText(setting.name.c_str(), x, y, 20, textColor);
        DrawText(valueText.c_str(), x + 280, y, 20, valueColor);

        if (currentDrawIndex == selectedIndex)
            DrawText(">", x - 20, y, 20, highlightColor);

        y += lineHeight;
        const_cast<UIMenu *>(this)->currentDrawIndex++;
    }
}

void UIMenu::DrawFloatSettings(int &x, int &y, int lineHeight, Color highlightColor) const
{
    for (const auto &setting : floatSettings)
    {
        Color textColor = (currentDrawIndex == selectedIndex) ? highlightColor : WHITE;
        bool isEditing = (currentDrawIndex == selectedIndex && editingValue);
        Color valueColor = isEditing ? YELLOW : SKYBLUE;

        char valueText[32];
        snprintf(valueText, sizeof(valueText), "%.2f", *setting.value);

        DrawText(setting.name.c_str(), x, y, 20, textColor);
        DrawText(valueText, x + 280, y, 20, valueColor);

        if (currentDrawIndex == selectedIndex)
        {
            DrawText(">", x - 20, y, 20, highlightColor);
            if (isEditing)
            {
                DrawText("<", x + 260, y, 20, YELLOW);
                DrawText(">", x + 340, y, 20, YELLOW);
            }
        }

        y += lineHeight;
        const_cast<UIMenu *>(this)->currentDrawIndex++;
    }
}

void UIMenu::DrawIntSettings(int &x, int &y, int lineHeight, Color highlightColor) const
{
    for (const auto &setting : intSettings)
    {
        Color textColor = (currentDrawIndex == selectedIndex) ? highlightColor : WHITE;
        bool isEditing = (currentDrawIndex == selectedIndex && editingValue);
        Color valueColor = isEditing ? YELLOW : SKYBLUE;

        char valueText[32];
        snprintf(valueText, sizeof(valueText), "%d", *setting.value);

        DrawText(setting.name.c_str(), x, y, 20, textColor);
        DrawText(valueText, x + 280, y, 20, valueColor);

        if (currentDrawIndex == selectedIndex)
        {
            DrawText(">", x - 20, y, 20, highlightColor);
            if (isEditing)
            {
                DrawText("<", x + 260, y, 20, YELLOW);
                DrawText(">", x + 340, y, 20, YELLOW);
            }
        }

        y += lineHeight;
        const_cast<UIMenu *>(this)->currentDrawIndex++;
    }
}

void UIMenu::DrawStringSettings(int &x, int &y, int lineHeight, Color highlightColor) const
{
    for (const auto &setting : stringSettings)
    {
        Color textColor = (currentDrawIndex == selectedIndex) ? highlightColor : WHITE;
        bool isEditing = (currentDrawIndex == selectedIndex && editingValue);
        Color valueColor = isEditing ? YELLOW : SKYBLUE;

        const char *valueText = setting.options[*setting.selectedIndex].c_str();

        DrawText(setting.name.c_str(), x, y, 20, textColor);
        DrawText(valueText, x + 280, y, 20, valueColor);

        if (currentDrawIndex == selectedIndex)
        {
            DrawText(">", x - 20, y, 20, highlightColor);
            if (isEditing)
            {
                DrawText("<", x + 260, y, 20, YELLOW);
                DrawText(">", x + 340, y, 20, YELLOW);
            }
        }

        y += lineHeight;
        const_cast<UIMenu *>(this)->currentDrawIndex++;
    }
}
