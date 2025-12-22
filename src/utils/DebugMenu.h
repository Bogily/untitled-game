#pragma once
#include "raylib.h"
#include <vector>
#include <string>

// Debug menu for quickly adjusting settings during development
class DebugMenu
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

    struct StringSetting
    {
        std::string name;
        int *selectedIndex;
        std::vector<std::string> options;
    };

    std::vector<BoolSetting> boolSettings;
    std::vector<FloatSetting> floatSettings;
    std::vector<StringSetting> stringSettings;
    int selectedIndex = 0;
    bool editingValue = false;

    // Methods
    void Toggle();
    void AddBool(std::string name, bool *value);
    void AddFloat(std::string name, float *value, float min, float max, float step = 0.1f);
    void AddString(std::string name, int *selectedIndex, std::vector<std::string> options);
    void Update();
    void Draw();
};
