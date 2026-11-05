#pragma once
#include <vector>
#include <string>
#include "MonoAPI.h" // <-- Убедимся, что подключено

struct PlayerData {
    std::string name;
    Vector3 position;
    Vector3 head;
};

std::vector<PlayerData> GetPlayers();
Vector2 WorldToScreen(Vector3 worldPos, int screenWidth, int screenHeight);
