#include <windows.h>
#include <fstream>
#include <thread>
#include <chrono>
#include "MonoAPI.h"
#include "GameReader.h"

static std::ofstream g_log;

static void MainThread() {
    g_log.open("C:\\Users\\Unknown\\Desktop\\Новая папка (8)\\КОК.txt", std::ios::out | std::ios::trunc);
    if (!g_log.is_open()) return;

    g_log << "[+] DLL loaded (32-bit)\n";

    if (!LoadMono()) {
        g_log << "[-] Mono not found\n";
        return;
    }
    g_log << "[+] Mono loaded\n";

    while (true) {
        auto players = GetPlayers();
        g_log << "\n=== Players (" << players.size() << ") ===\n";
        for (auto& p : players) {
            if (!p.valid) continue;
            g_log << p.name << " | HP: " << p.health << " | Team: " << p.team
                  << " | Pos: (" << p.x << ", " << p.y << ", " << p.z << ")\n";
        }
        g_log.flush();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        std::thread(MainThread).detach();
    }
    return TRUE;
}
