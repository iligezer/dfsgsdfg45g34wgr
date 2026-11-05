#include "GameReader.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

std::ofstream g_log("D:\\vs\\kok\\cheat_log.txt", std::ios::app);

// ... (твои функции InitClasses, GetTransformName, FindHeadBoneRecursive и т.д.) ...

void ProcessCommand(const std::string& cmd) {
    g_log << "[*] Command: " << cmd << std::endl;
    if (cmd == "find_head") {
        FindAllManagers();
    } else if (cmd == "clear_log") {
        g_log.close();
        g_log.open("D:\\vs\\kok\\cheat_log.txt", std::ios::trunc);
        g_log << "Log cleared" << std::endl;
    } else {
        g_log << "[-] Unknown command" << std::endl;
    }
}

void CommandThread() {
    std::string lastCmd;
    while (true) {
        std::ifstream file("D:\\vs\\kok\\command.txt");
        if (file.is_open()) {
            std::string cmd;
            std::getline(file, cmd);
            file.close();
            if (!cmd.empty() && cmd != lastCmd) {
                lastCmd = cmd;
                ProcessCommand(cmd);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
