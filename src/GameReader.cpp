#include "GameReader.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// Лог-файл
std::ofstream g_log("D:\\vs\\kok\\cheat_log.txt", std::ios::app);

std::vector<PlayerData> GetPlayers() {
    std::vector<PlayerData> players;
    void* domain = mono_get_root_domain();
    if (!domain) return players;
    mono_thread_attach(domain);

    // 1. Загружаем сборку Assembly-CSharp
    void* assembly = mono_domain_assembly_open(domain, "Assembly-CSharp");
    if (!assembly) {
        g_log << "[-] Assembly-CSharp not found" << std::endl;
        return players;
    }
    void* image = mono_assembly_get_image(assembly);

    // 2. Ищем класс PlayerManager
    void* klassPlayerManager = mono_class_from_name(image, "", "PlayerManager");
    if (!klassPlayerManager) {
        g_log << "[-] PlayerManager class not found" << std::endl;
        return players;
    }

    // 3. Получаем PlayerManager.Instance
    void* methodGetInstance = mono_class_get_method_from_name(klassPlayerManager, "get_Instance", 0);
    if (!methodGetInstance) {
        g_log << "[-] PlayerManager.get_Instance not found" << std::endl;
        return players;
    }

    void* exception = nullptr;
    void* playerManager = mono_runtime_invoke(methodGetInstance, nullptr, nullptr, &exception);
    if (!playerManager) {
        g_log << "[-] PlayerManager.Instance is null" << std::endl;
        return players;
    }

    // 4. Получаем поле Players
    void* fieldPlayers = mono_class_get_field_from_name(klassPlayerManager, "Players");
    if (!fieldPlayers) {
        g_log << "[-] PlayerManager.Players field not found" << std::endl;
        return players;
    }
    
    int playersOffset = mono_field_get_offset(fieldPlayers);
    void* playersDictObj = *(void**)((uintptr_t)playerManager + playersOffset);
    if (!playersDictObj) {
        g_log << "[-] Players dictionary is null" << std::endl;
        return players;
    }

    // 5. Получаем внутренние поля Dictionary
    void* klassDict = mono_object_get_class(playersDictObj);
    
    // Ищем поле "_entries"
    void* fieldEntries = mono_class_get_field_from_name(klassDict, "_entries");
    if (!fieldEntries) fieldEntries = mono_class_get_field_from_name(klassDict, "entries");
    if (!fieldEntries) {
        g_log << "[-] Dictionary._entries field not found" << std::endl;
        return players;
    }
    int entriesOffset = mono_field_get_offset(fieldEntries);
    
    // Ищем поле "_count"
    void* fieldCount = mono_class_get_field_from_name(klassDict, "_count");
    if (!fieldCount) fieldCount = mono_class_get_field_from_name(klassDict, "count");
    if (!fieldCount) {
        g_log << "[-] Dictionary._count field not found" << std::endl;
        return players;
    }
    int countOffset = mono_field_get_offset(fieldCount);

    // Читаем длину и указатель на массив entries
    int count = *(int*)((uintptr_t)playersDictObj + countOffset);
    void* entriesArrayObj = *(void**)((uintptr_t)playersDictObj + entriesOffset);

    if (!entriesArrayObj || count == 0) {
        g_log << "[-] Dictionary is empty or entries array is null" << std::endl;
        return players;
    }

    // 6. Проходим по элементам Dictionary
    for (int i = 0; i < count; i++) {
        // Смещение данных: 0x10 (заголовок массива) + i * 16 (размер Entry)
        uintptr_t entryAddr = (uintptr_t)entriesArrayObj + 0x10 + (i * 16);
        
        // В структуре Entry поле value идет после hashCode (4) + next (4) + key (4) = 12 байт.
        void* playerObj = *(void**)(entryAddr + 12);
        
        if (playerObj == nullptr) continue;

        PlayerData pData;
        pData.name = "Player_" + std::to_string(i);

        void* klassPlayer = mono_object_get_class(playerObj);
        
        // Получаем Transform
        void* methodGetTransform = mono_class_get_method_from_name(klassPlayer, "get_transform", 0);
        if (methodGetTransform) {
            void* transformObj = mono_runtime_invoke(methodGetTransform, playerObj, nullptr, &exception);
            if (transformObj) {
                void* klassTransform = mono_object_get_class(transformObj);
                void* methodGetPosition = mono_class_get_method_from_name(klassTransform, "get_position", 0);
                if (methodGetPosition) {
                    MonoObject* posObj = (MonoObject*)mono_runtime_invoke(methodGetPosition, transformObj, nullptr, &exception);
                    if (posObj) {
                        Vector3* pos = (Vector3*)mono_object_unbox(posObj);
                        pData.position = *pos;
                        pData.head = { pos->x, pos->y + 1.6f, pos->z };
                    }
                }
            }
        }

        players.push_back(pData);
    }

    g_log << "[+] Found " << players.size() << " players via Dictionary entries" << std::endl;
    return players;
}

Vector2 WorldToScreen(Vector3 worldPos, int screenWidth, int screenHeight) {
    Vector2 screenPos = { 0, 0 };
    void* domain = mono_get_root_domain();
    if (!domain) return screenPos;
    mono_thread_attach(domain);

    void* assembly = mono_domain_assembly_open(domain, "UnityEngine");
    if (!assembly) return screenPos;
    void* image = mono_assembly_get_image(assembly);
    void* klassCamera = mono_class_from_name(image, "UnityEngine", "Camera");
    if (!klassCamera) return screenPos;

    void* methodGetMain = mono_class_get_method_from_name(klassCamera, "get_main", 0);
    if (!methodGetMain) return screenPos;

    void* exception = nullptr;
    void* mainCamera = mono_runtime_invoke(methodGetMain, nullptr, nullptr, &exception);
    if (!mainCamera) return screenPos;

    void* methodW2S = mono_class_get_method_from_name(klassCamera, "WorldToScreenPoint", 1);
    if (!methodW2S) return screenPos;

    void* args[1] = { &worldPos };
    MonoObject* resultObj = (MonoObject*)mono_runtime_invoke(methodW2S, mainCamera, args, &exception);
    
    if (resultObj) {
        Vector3* result = (Vector3*)mono_object_unbox(resultObj);
        screenPos.x = result->x;
        screenPos.y = screenHeight - result->y;
    }
    return screenPos;
}
