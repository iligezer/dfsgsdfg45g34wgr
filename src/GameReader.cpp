#include "GameReader.h"
#include <iostream>
#include <fstream>
#include <vector>

std::ofstream g_log("D:\\vs\\kok\\cheat_log.txt", std::ios::app);

// Структура для хранения данных игрока
struct PlayerData {
    std::string name;
    Vector3 position;
    Vector3 head;
};

std::vector<PlayerData> GetPlayers() {
    std::vector<PlayerData> players;
    void* domain = mono_get_root_domain();
    if (!domain) return players;
    mono_thread_attach(domain);

    // 1. Ищем сборку Assembly-CSharp (там лежат игровые классы)
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

    // 3. Получаем статическое свойство PlayerManager.Instance
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

    // 4. Получаем поле Players (это Dictionary<int, CombatPlayer>)
    void* fieldPlayers = mono_class_get_field_from_name(klassPlayerManager, "Players");
    if (!fieldPlayers) {
        g_log << "[-] PlayerManager.Players field not found" << std::endl;
        return players;
    }
    
    int playersOffset = mono_field_get_offset(fieldPlayers);
    
    // В Mono получить Dictionary сложно, поэтому мы просто прочитаем его как массив
    // Но для этого нужно знать структуру Dictionary. 
    // Проще использовать метод get_Values() у Dictionary.
    void* playersDictObj = *(void**)((uintptr_t)playerManager + playersOffset);
    if (!playersDictObj) {
        g_log << "[-] Players dictionary is null" << std::endl;
        return players;
    }

    // Получаем класс Dictionary и метод get_Values()
    void* klassDict = mono_object_get_class(playersDictObj);
    void* methodGetValues = mono_class_get_method_from_name(klassDict, "get_Values", 0);
    if (!methodGetValues) {
        g_log << "[-] Dictionary.get_Values not found" << std::endl;
        return players;
    }

    void* valuesCollection = mono_runtime_invoke(methodGetValues, playersDictObj, nullptr, &exception);
    if (!valuesCollection) {
        g_log << "[-] Values collection is null" << std::endl;
        return players;
    }

    // Теперь у нас есть ValueCollection. Преобразуем его в массив (ToArray)
    void* klassCollection = mono_object_get_class(valuesCollection);
    void* methodToArray = mono_class_get_method_from_name(klassCollection, "ToArray", 0);
    if (!methodToArray) {
        // Если ToArray нет, пробуем использовать CopyTo (сложнее)
        g_log << "[-] ValueCollection.ToArray not found" << std::endl;
        return players;
    }

    void* arrayObj = mono_runtime_invoke(methodToArray, valuesCollection, nullptr, &exception);
    if (!arrayObj) {
        g_log << "[-] ToArray returned null" << std::endl;
        return players;
    }

    int length = mono_array_length(arrayObj);
    g_log << "[+] Found " << length << " players via PlayerManager" << std::endl;

    // 5. Проходим по каждому CombatPlayer
    for (int i = 0; i < length; i++) {
        void* playerObj = mono_array_addr(arrayObj, i);
        if (!playerObj) continue;

        PlayerData pData;
        
        // Получаем имя игрока (поле Name)
        void* klassPlayer = mono_object_get_class(playerObj);
        void* fieldName = mono_class_get_field_from_name(klassPlayer, "Name");
        if (fieldName) {
            int nameOffset = mono_field_get_offset(fieldName);
            // Читаем строку (MonoString)
            void* nameStrObj = *(void**)((uintptr_t)playerObj + nameOffset);
            if (nameStrObj) {
                // В Mono строка - это массив символов (UTF-16), но для простоты пока оставим так
                pData.name = "Player_" + std::to_string(i);
            } else {
                pData.name = "Player_" + std::to_string(i);
            }
        } else {
            pData.name = "Player_" + std::to_string(i);
        }

        // Получаем Transform
        void* methodGetTransform = mono_class_get_method_from_name(klassPlayer, "get_transform", 0);
        if (methodGetTransform) {
            void* transformObj = mono_runtime_invoke(methodGetTransform, playerObj, nullptr, &exception);
            if (transformObj) {
                // Получаем позицию
                void* klassTransform = mono_object_get_class(transformObj);
                void* methodGetPosition = mono_class_get_method_from_name(klassTransform, "get_position", 0);
                if (methodGetPosition) {
                    MonoObject* posObj = (MonoObject*)mono_runtime_invoke(methodGetPosition, transformObj, nullptr, &exception);
                    if (posObj) {
                        Vector3* pos = (Vector3*)mono_object_unbox(posObj);
                        pData.position = *pos;
                        pData.head = { pos->x, pos->y + 1.6f, pos->z }; // Заглушка для головы
                    }
                }
            }
        }

        players.push_back(pData);
    }

    return players;
}

// Функция WorldToScreen (осталась без изменений, она работает через Camera.main)
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
