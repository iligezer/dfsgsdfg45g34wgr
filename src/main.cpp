#include <windows.h>
#include <thread>
#include "MonoAPI.h"
#include "GameReader.h"
#include "Overlay.h"

void MainThread() {
    Sleep(5000); 
    if (!InitMono()) return;

    // Запускаем оверлей ImGui в отдельном потоке
    std::thread overlayThread(StartOverlay);
    
    while (true) {
        // Основной цикл (можно оставить пустым, так как GetPlayers вызывается внутри Overlay)
        Sleep(100);
    }
    overlayThread.join();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}
