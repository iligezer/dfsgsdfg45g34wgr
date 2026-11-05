#pragma once
#include <vector>
#include <string>
#include "MonoAPI.h"

struct PlayerData {
    std::string name;
    Vector3 position;
    Vector3 head;
    int health;
    int maxHealth;
    int team;
    bool isDead;
};

// Основные функции
std::vector<PlayerData> GetPlayers();
Vector2 WorldToScreen(Vector3 worldPos);
void AimAt(Vector3 targetPos);
void* GetLocalPlayerTransform();

// Тестовая функция для поиска головы
void FindHeadBone();
