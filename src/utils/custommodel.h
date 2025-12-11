#pragma once

class Player;

class CustomModel{
public:
    void loadPlayerModel(Player& player, const char* modelPath, const char* texturePath = nullptr);
    void drawPlayerModel(const Player& player);
};