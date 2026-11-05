#include "GameReader.h"
#include <iostream>
#include <cmath>

// Глобальные указатели на классы, чтобы не искать их каждый кадр
static void* klassLocalPlayerManager = nullptr;
static void* klassCombatPlayer = nullptr;
static void* klassCamera = nullptr;

void InitClasses() {
    if (klassLocalPlayerManager) return;
    void* domain = mono_get_root_domain();
    if (!domain) return;
    mono_thread_attach(domain);

    void* assembly = mono_domain_assembly_open(domain, "Assembly-CSharp");
    if (!assembly) return;
    void* image = mono_assembly_get_image(assembly);

    klassLocalPlayerManager = mono_class_from_name(image, "", "LocalPlayerManager");
    klassCombatPlayer = mono_class_from_name(image, "", "CombatPlayer");
    
    void* unityAssembly = mono_domain_assembly_open(domain, "UnityEngine");
    if (unityAssembly) {
        void* unityImage = mono_assembly_get_image(unityAssembly);
        klassCamera = mono_class_from_name(unityImage, "UnityEngine", "Camera");
    }
}

std::vector<PlayerData> GetPlayers() {
    std::vector<PlayerData> players;
    InitClasses();
    if (!klassLocalPlayerManager || !klassCombatPlayer) return players;

    void* domain = mono_get_root_domain();
    mono_thread_attach(domain);
    void* exception = nullptr;

    // 1. Получаем Instance LocalPlayerManager
    void* getInstance = mono_class_get_method_from_name(klassLocalPlayerManager, "get_Instance", 0);
    if (!getInstance) return players;
    void* manager = mono_runtime_invoke(getInstance, nullptr, nullptr, &exception);
    if (!manager) return players;

    // 2. Получаем LocalPlayer и его команду
    void* fieldLocalPlayer = mono_class_get_field_from_name(klassLocalPlayerManager, "LocalPlayer");
    if (!fieldLocalPlayer) return players;
    void* localPlayer = *(void**)((uintptr_t)manager + mono_field_get_offset(fieldLocalPlayer));
    
    int localTeam = -1;
    if (localPlayer) {
        void* getTeam = mono_class_get_method_from_name(klassCombatPlayer, "get_Team", 0);
        if (getTeam) {
            MonoObject* teamObj = (MonoObject*)mono_runtime_invoke(getTeam, localPlayer, nullptr, &exception);
            if (teamObj) localTeam = *(int*)mono_object_unbox(teamObj);
        }
    }

    // 3. Получаем Players (Dictionary)
    void* fieldPlayers = mono_class_get_field_from_name(klassLocalPlayerManager, "Players");
    if (!fieldPlayers) return players;
    void* playersDict = *(void**)((uintptr_t)manager + mono_field_get_offset(fieldPlayers));
    if (!playersDict) return players;

    // 4. Перебираем Dictionary через Enumerator
    void* klassDict = mono_object_get_class(playersDict);
    void* getValues = mono_class_get_method_from_name(klassDict, "get_Values", 0);
    if (!getValues) return players;
    void* values = mono_runtime_invoke(getValues, playersDict, nullptr, &exception);
    if (!values) return players;

    void* klassCollection = mono_object_get_class(values);
    void* getEnumerator = mono_class_get_method_from_name(klassCollection, "GetEnumerator", 0);
    if (!getEnumerator) return players;
    void* enumerator = mono_runtime_invoke(getEnumerator, values, nullptr, &exception);
    if (!enumerator) return players;

    void* klassEnumerator = mono_object_get_class(enumerator);
    void* moveNext = mono_class_get_method_from_name(klassEnumerator, "MoveNext", 0);
    void* getCurrent = mono_class_get_method_from_name(klassEnumerator, "get_Current", 0);
    if (!moveNext || !getCurrent) return players;

    int index = 0;
    while (true) {
        MonoObject* moveNextResult = (MonoObject*)mono_runtime_invoke(moveNext, enumerator, nullptr, &exception);
        if (!moveNextResult) break;
        bool hasNext = *(bool*)mono_object_unbox(moveNextResult);
        if (!hasNext) break;

        void* playerObj = mono_runtime_invoke(getCurrent, enumerator, nullptr, &exception);
        if (!playerObj) continue;

        PlayerData pData;
        pData.name = "Player_" + std::to_string(index++);
        
        // Получаем Transform и позицию
        void* getTransform = mono_class_get_method_from_name(klassCombatPlayer, "get_transform", 0);
        if (getTransform) {
            void* transformObj = mono_runtime_invoke(getTransform, playerObj, nullptr, &exception);
            if (transformObj) {
                void* klassTransform = mono_object_get_class(transformObj);
                void* getPosition = mono_class_get_method_from_name(klassTransform, "get_position", 0);
                if (getPosition) {
                    MonoObject* posObj = (MonoObject*)mono_runtime_invoke(getPosition, transformObj, nullptr, &exception);
                    if (posObj) {
                        Vector3* pos = (Vector3*)mono_object_unbox(posObj);
                        pData.position = *pos;
                        pData.head = { pos->x, pos->y + 1.6f, pos->z }; // Заглушка для головы
                    }
                }
            }
        }

        // Получаем данные игрока (HP, Team, IsDead)
        void* getHealth = mono_class_get_method_from_name(klassCombatPlayer, "get_Health", 0);
        if (getHealth) {
            MonoObject* hpObj = (MonoObject*)mono_runtime_invoke(getHealth, playerObj, nullptr, &exception);
            if (hpObj) pData.health = *(int*)mono_object_unbox(hpObj);
        }
        void* getMaxHealth = mono_class_get_method_from_name(klassCombatPlayer, "get_MaxHealth", 0);
        if (getMaxHealth) {
            MonoObject* maxHpObj = (MonoObject*)mono_runtime_invoke(getMaxHealth, playerObj, nullptr, &exception);
            if (maxHpObj) pData.maxHealth = *(int*)mono_object_unbox(maxHpObj);
        }
        void* getTeam = mono_class_get_method_from_name(klassCombatPlayer, "get_Team", 0);
        if (getTeam) {
            MonoObject* teamObj = (MonoObject*)mono_runtime_invoke(getTeam, playerObj, nullptr, &exception);
            if (teamObj) pData.team = *(int*)mono_object_unbox(teamObj);
        }
        void* getIsDead = mono_class_get_method_from_name(klassCombatPlayer, "get_IsDead", 0);
        if (getIsDead) {
            MonoObject* deadObj = (MonoObject*)mono_runtime_invoke(getIsDead, playerObj, nullptr, &exception);
            if (deadObj) pData.isDead = *(bool*)mono_object_unbox(deadObj);
        }
        void* getName = mono_class_get_method_from_name(klassCombatPlayer, "get_Name", 0);
        if (getName) {
            MonoObject* nameObj = (MonoObject*)mono_runtime_invoke(getName, playerObj, nullptr, &exception);
            if (nameObj) {
                // Упрощенно, просто ставим имя по индексу
                // Для реального чтения строки нужен mono_string_to_utf8
            }
        }

        // Если команда локального игрока не определена, рисуем всех
        pData.team = localTeam; // Упрощение: пока не рисуем врагов/союзников

        players.push_back(pData);
    }
    return players;
}

Vector2 WorldToScreen(Vector3 worldPos) {
    Vector2 screenPos = { 0, 0 };
    if (!klassCamera) return screenPos;

    void* domain = mono_get_root_domain();
    mono_thread_attach(domain);
    void* exception = nullptr;

    void* getMain = mono_class_get_method_from_name(klassCamera, "get_main", 0);
    if (!getMain) return screenPos;
    void* mainCamera = mono_runtime_invoke(getMain, nullptr, nullptr, &exception);
    if (!mainCamera) return screenPos;

    void* w2s = mono_class_get_method_from_name(klassCamera, "WorldToScreenPoint", 1);
    if (!w2s) return screenPos;

    void* args[1] = { &worldPos };
    MonoObject* resultObj = (MonoObject*)mono_runtime_invoke(w2s, mainCamera, args, &exception);
    if (resultObj) {
        Vector3* result = (Vector3*)mono_object_unbox(resultObj);
        screenPos.x = result->x;
        screenPos.y = ImGui::GetIO().DisplaySize.y - result->y;
    }
    return screenPos;
}

void AimAt(Vector3 targetPos) {
    // Логика аимбота: нужно получить позицию камеры и вычислить угол.
    // Но для простоты мы просто перемещаем курсор мыши (это работает в оконном режиме).
    // В полноэкранном режиме это не сработает. Нужно писать в память (transform.rotation).
    // Пока оставим заглушку.
}
