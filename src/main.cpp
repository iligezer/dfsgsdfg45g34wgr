#include <windows.h>
#include <fstream>
#include "MonoAPI.h"
#include "Hook.h"
#include "GameReader.h"

static std::ofstream g_log;

static DWORD WINAPI MainThread(LPVOID) {
    g_log.open("C:\\Users\\Unknown\\Desktop\\Новая папка (8)\\КОК.txt", std::ios::out | std::ios::trunc);
    if (!g_log.is_open()) return 0;

    g_log << "[+] DLL loaded (32-bit)\n";

    if (!LoadMono()) {
        g_log << "[-] Mono not found\n";
        return 0;
    }
    g_log << "[+] Mono loaded\n";

    // Инициализация хука для меню
    if (!InitHook()) {
        g_log << "[-] Hook failed\n";
        return 0;
    }
    g_log << "[+] Hook installed (Press INSERT to toggle menu)\n";

    // Основной цикл: дампим игроков в лог каждые 5 секунд
    while (true) {
        auto players = GetPlayers();
        g_log << "\n=== Players (" << players.size() << ") ===\n";
        for (auto& p : players) {
            if (!p.valid) continue;
            g_log << p.name << " | HP: " << p.health << " | Team: " << p.team
                  << " | Pos: (" << p.x << ", " << p.y << ", " << p.z << ")\n";
        }
        g_log.flush();
        Sleep(5000);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
