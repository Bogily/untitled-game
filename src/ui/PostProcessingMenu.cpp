#include "PostProcessingMenu.h"

void PostProcessingMenu::Update()
{
    // Toggle menu with F5
    if (IsKeyPressed(KEY_F5))
        Toggle();

    if (!isVisible)
        return;

    if (GetTotalItems() == 0)
        return;

    // Handle navigation
    HandleNavigation();

    // Handle selection
    HandleSelection();

    // Handle editing
    int boolCount = boolSettings.size();
    int floatCount = floatSettings.size();
    int intCount = intSettings.size();

    if (editingValue)
    {
        if (selectedIndex >= boolCount && selectedIndex < boolCount + floatCount)
        {
            // Edit float value
            int floatIndex = selectedIndex - boolCount;
            HandleFloatEdit(floatIndex);
        }
        else if (selectedIndex >= boolCount + floatCount && selectedIndex < boolCount + floatCount + intCount)
        {
            // Edit int value
            int intIndex = selectedIndex - boolCount - floatCount;
            HandleIntEdit(intIndex);
        }
        else if (selectedIndex >= boolCount + floatCount + intCount)
        {
            // Edit string value
            int stringIndex = selectedIndex - boolCount - floatCount - intCount;
            HandleStringEdit(stringIndex);
        }
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
    int totalItems = GetTotalItems();
    int height = (totalItems + 2) * lineHeight + 20;

    // Draw background panel
    DrawRectangle(x - 10, y - 10, width, height, Fade(BLACK, 0.8f));
    DrawRectangleLines(x - 10, y - 10, width, height, SKYBLUE);

    // Title
    DrawText("POST-PROCESSING MENU (F5 to toggle)", x, y, 20, SKYBLUE);
    y += lineHeight;
    DrawText("UP/DOWN: Navigate | ENTER/SPACE: Toggle/Edit | LEFT/RIGHT: Adjust", x, y, 12, GRAY);
    y += lineHeight;

    // Reset draw index and draw all settings
    currentDrawIndex = 0;
    DrawBoolSettings(x, y, lineHeight, SKYBLUE);
    DrawFloatSettings(x, y, lineHeight, SKYBLUE);
    DrawIntSettings(x, y, lineHeight, SKYBLUE);
    DrawStringSettings(x, y, lineHeight, SKYBLUE);

    // Draw hint at bottom
    if (totalItems > 0)
    {
        DrawText("SHIFT + LEFT/RIGHT for faster adjustments", x, y + 10, 12, DARKGRAY);
    }
}
