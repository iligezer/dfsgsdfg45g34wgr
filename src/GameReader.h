#pragma once
#include <vector>
#include <string>
#include "MonoAPI.h"

struct PlayerData {
    // ...
};

void FindAllManagers();
void CommandThread();
void ProcessCommand(const std::string& cmd);
// ...
