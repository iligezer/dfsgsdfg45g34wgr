#include <windows.h>
#include <thread>
#include <string>
#include "MonoAPI.h"
#include "Overlay.h"

void MainThread() {
    Sleep(5000); 

    // Создаем папку для логов, если её нет
    std::string folderPath = "D:\\vs\\kok";
    std::wstring widePath = std::wstring(folderPath.begin(), folderPath.end());
    CreateDirectoryW(widePath.c_str(), NULL);

    if (!InitMono()) {
        // Если Mono не инициализировалась, пишем об этом в лог
        std::ofstream errLog("D:\\vs\\kok\\cheat_log.txt", std::ios::app);
        errLog << "[-] InitMono() failed! mono.dll not found or broken." << std::endl;
        errLog.close();
        return;
    }

    std::thread overlayThread(StartOverlay);
    overlayThread.join();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
