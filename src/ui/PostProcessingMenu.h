#pragma once
#include "UIMenu.h"

/**
 * Post-processing menu for adjusting shader effects.
 * Toggle with F5 key.
 */
class PostProcessingMenu : public UIMenu
{
public:
    void Update() override;
    void Draw() override;
};
