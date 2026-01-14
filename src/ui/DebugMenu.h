#pragma once
#include "UIMenu.h"

/**
 * Debug menu for quickly adjusting game settings during development.
 * Toggle with TAB key.
 */
class DebugMenu : public UIMenu
{
public:
    void Update() override;
    void Draw() override;
};
