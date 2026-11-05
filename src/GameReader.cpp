#include "GameReader.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

std::ofstream g_log("D:\\vs\\kok\\cheat_log.txt", std::ios::app);

std::vector<PlayerData> GetPlayers() {
    std::vector<PlayerData> players;
    void* domain = mono_get_root_domain();
    if (!domain) return players;
    mono_thread_attach(domain);

    // Загружаем Assembly-CSharp
    void* assembly = mono_domain_assembly_open(domain, "Assembly-CSharp");
    if (!assembly) {
        g_log << "[-] Assembly-CSharp not found" << std::endl;
        return players;
    }
    void* image = mono_assembly_get_image(assembly);

    // Ищем LocalPlayerManager
    void* klassManager = mono_class_from_name(image, "", "LocalPlayerManager");
    if (!klassManager) {
        g_log << "[-] LocalPlayerManager class not found" << std::endl;
        return players;
    }

    // Получаем Instance через get_Instance()
    void* methodGetInstance = mono_class_get_method_from_name(klassManager, "get_Instance", 0);
    if (!methodGetInstance) {
        g_log << "[-] get_Instance not found" << std::endl;
        return players;
    }

    void* exception = nullptr;
    void* manager = mono_runtime_invoke(methodGetInstance, nullptr, nullptr, &exception);
    if (!manager) {
        g_log << "[-] Instance is null" << std::endl;
        return players;
    }

    // Получаем поле Players
    void* fieldPlayers = mono_class_get_field_from_name(klassManager, "Players");
    if (!fieldPlayers) {
        g_log << "[-] Players field not found" << std::endl;
        return players;
    }
    int playersOffset = mono_field_get_offset(fieldPlayers);
    void* playersDictObj = *(void**)((uintptr_t)manager + playersOffset);
    if (!playersDictObj) {
        g_log << "[-] Players dictionary is null" << std::endl;
        return players;
    }

    // Получаем Values у Dictionary
    void* klassDict = mono_object_get_class(playersDictObj);
    void* methodGetValues = mono_class_get_method_from_name(klassDict, "get_Values", 0);
    if (!methodGetValues) {
        g_log << "[-] get_Values not found" << std::endl;
        return players;
    }
    void* valuesCollection = mono_runtime_invoke(methodGetValues, playersDictObj, nullptr, &exception);
    if (!valuesCollection) {
        g_log << "[-] Values collection is null" << std::endl;
        return players;
    }

    // Получаем Enumerator
    void* klassCollection = mono_object_get_class(valuesCollection);
    void* methodGetEnumerator = mono_class_get_method_from_name(klassCollection, "GetEnumerator", 0);
    if (!methodGetEnumerator) {
        g_log << "[-] GetEnumerator not found" << std::endl;
        return players;
    }
    void* enumeratorObj = mono_runtime_invoke(methodGetEnumerator, valuesCollection, nullptr, &exception);
    if (!enumeratorObj) {
        g_log << "[-] Enumerator is null" << std::endl;
        return players;
    }

    void* klassEnumerator = mono_object_get_class(enumeratorObj);
    void* methodMoveNext = mono_class_get_method_from_name(klassEnumerator, "MoveNext", 0);
    void* methodGetCurrent = mono_class_get_method_from_name(klassEnumerator, "get_Current", 0);
    
    if (!methodMoveNext || !methodGetCurrent) {
        g_log << "[-] Enumerator methods not found" << std::endl;
        return players;
    }

    int playerIndex = 0;
    while (true) {
        MonoObject* moveNextResult = (MonoObject*)mono_runtime_invoke(methodMoveNext, enumeratorObj, nullptr, &exception);
        if (!moveNextResult) break;
        
        bool hasNext = *(bool*)mono_object_unbox(moveNextResult);
        if (!hasNext) break;

        void* playerObj = mono_runtime_invoke(methodGetCurrent, enumeratorObj, nullptr, &exception);
        if (!playerObj) continue;

        PlayerData pData;
        pData.name = "Player_" + std::to_string(playerIndex);
        playerIndex++;

        void* klassPlayer = mono_object_get_class(playerObj);
        
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

    g_log << "[+] Found " << players.size() << " players" << std::endl;
    return players;
}

// WorldToScreen (без изменений)
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
