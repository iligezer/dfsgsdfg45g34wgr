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

std::vector<PlayerInfo> GetPlayers();
