#pragma once
#include "raylib.h"
#include <vector>
#include <string>

// Post-processing debug menu for adjusting shader effects (F5)
class PostProcessingMenu
{
public:
    bool isVisible = false;

    // Settings storage
    struct BoolSetting
    {
        std::string name;
        bool *value;
    };

    struct FloatSetting
    {
        std::string name;
        float *value;
        float min;
        float max;
        float step;
    };

    struct IntSetting
    {
        std::string name;
        int *value;
        int min;
        int max;
        int step;
    };

    std::vector<BoolSetting> boolSettings;
    std::vector<FloatSetting> floatSettings;
    std::vector<IntSetting> intSettings;
    int selectedIndex = 0;
    bool editingValue = false;

    // Methods
    void Toggle();
    void AddBool(std::string name, bool *value);
    void AddFloat(std::string name, float *value, float min, float max, float step = 0.1f);
    void AddInt(std::string name, int *value, int min, int max, int step = 1);
    void Update();
    void Draw();
};
