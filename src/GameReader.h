#pragma once
#include <vector>
#include <string>

struct PlayerInfo {
    void* ptr = nullptr;
    std::string name;
    int health = 0;
    int team = 0;
    bool isDead = false;
    float x = 0, y = 0, z = 0;
    bool valid = false;
};

// Получить всех игроков через FindObjectsOfType
std::vector<PlayerInfo> GetPlayers();

// Получить позицию камеры (для WorldToScreen)
bool GetCameraPosition(float& x, float& y, float& z);
