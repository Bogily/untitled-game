#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include <functional>

/**
 * Base class for UI menus with common settings and functionality.
 * Provides an easy-to-use system for creating debug and settings menus.
 */
class UIMenu
{
public:
    virtual ~UIMenu() = default;

    // Visibility
    bool isVisible = false;

    // Setting types
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

    struct StringSetting
    {
        std::string name;
        int *selectedIndex;
        std::vector<std::string> options;
    };

    struct ButtonSetting
    {
        std::string name;
        std::function<void()> callback;
    };

    // Settings storage
    std::vector<BoolSetting> boolSettings;
    std::vector<FloatSetting> floatSettings;
    std::vector<IntSetting> intSettings;
    std::vector<StringSetting> stringSettings;
    std::vector<ButtonSetting> buttonSettings;

    // Navigation state
    int selectedIndex = 0;
    bool editingValue = false;

    // Methods to add settings
    void AddBool(std::string name, bool *value);
    void AddFloat(std::string name, float *value, float min, float max, float step = 0.1f);
    void AddInt(std::string name, int *value, int min, int max, int step = 1);
    void AddString(std::string name, int *selectedIndex, std::vector<std::string> options);
    void AddButton(std::string name, std::function<void()> callback);

    // Core functionality
    void Toggle();
    virtual void Update() = 0; // Must be implemented by derived classes
    virtual void Draw() = 0;   // Must be implemented by derived classes

protected:
    // Helper methods for derived classes
    int GetTotalItems() const;
    void HandleNavigation();
    void HandleSelection();
    void HandleFloatEdit(int floatIndex);
    void HandleIntEdit(int intIndex);
    void HandleStringEdit(int stringIndex);

    // Drawing helpers
    void DrawBoolSettings(int &x, int &y, int lineHeight, Color highlightColor) const;
    void DrawFloatSettings(int &x, int &y, int lineHeight, Color highlightColor) const;
    void DrawIntSettings(int &x, int &y, int lineHeight, Color highlightColor) const;
    void DrawStringSettings(int &x, int &y, int lineHeight, Color highlightColor) const;
    void DrawButtonSettings(int &x, int &y, int lineHeight, Color highlightColor) const;

    // Temporary variable for drawing (mutable to allow in const methods)
    mutable int currentDrawIndex = 0;

private:
};
