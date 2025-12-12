#pragma once
#include <string>
#include <vector>

class Player;

struct ModelData {
    std::string modelPath;
    std::string texturePath;
    std::string name;
    Vector3 scale;
    Vector3 rotationOffset;  // X, Y, Z rotation in degrees
};

class CustomModel{
private:
    std::vector<ModelData> availableModels;
    
public:
    void addModel(const std::string& name, const std::string& modelPath, const std::string& texturePath = "", Vector3 scale = {1.0f, 1.0f, 1.0f}, Vector3 rotationOffset = {0.0f, 0.0f, 0.0f});
    void loadPlayerModel(Player& player, int modelIndex);
    void loadPlayerModel(Player& player, const char* modelPath, const char* texturePath = nullptr);
    void drawPlayerModel(const Player& player);
    int getModelCount() const { return availableModels.size(); }
    std::string getModelName(int index) const;
};