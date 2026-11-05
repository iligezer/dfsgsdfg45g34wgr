#include "GameReader.h"
#include "MonoAPI.h"
#include <iostream>

std::vector<PlayerInfo> GetPlayers() {
    std::vector<PlayerInfo> result;

    void* rootDomain = mono_get_root_domain();
    if (!rootDomain) return result;
    mono_thread_attach(rootDomain);

    void* combatPlayerClass = FindClass("", "CombatPlayer");
    void* transformClass = FindClass("UnityEngine", "Transform");
    void* objectClass = FindClass("UnityEngine", "Object");
    if (!combatPlayerClass || !transformClass || !objectClass) return result;

    // Получаем System.Type для CombatPlayer
    void* type = mono_class_get_type(combatPlayerClass);
    void* typeObj = mono_type_get_object(rootDomain, type);
    if (!typeObj) return result;

    // FindObjectsOfType
    void* findMethod = mono_class_get_method_from_name(objectClass, "FindObjectsOfType", 1);
    if (!findMethod) return result;

    void* params[1] = { typeObj };
    void* exc = nullptr;
    void* arr = mono_runtime_invoke(findMethod, nullptr, params, &exc);
    if (!arr) return result;

    int len = mono_array_length(arr);
    if (len <= 0) return result;

    // Методы
    void* getHealth = mono_class_get_method_from_name(combatPlayerClass, "get_Health", 0);
    void* getName   = mono_class_get_method_from_name(combatPlayerClass, "get_Name", 0);
    void* getTeam   = mono_class_get_method_from_name(combatPlayerClass, "get_Team", 0);
    void* getIsDead = mono_class_get_method_from_name(combatPlayerClass, "get_IsDead", 0);
    void* getBiped  = mono_class_get_method_from_name(combatPlayerClass, "get_Biped", 0);
    void* getPos    = mono_class_get_method_from_name(transformClass, "get_position", 0);

    for (int i = 0; i < len; i++) {
        void* player = *(void**)((char*)arr + 0x20 + i * 8); // 32-bit: 0x10? Проверь!
        if (!player) continue;

        PlayerInfo info;
        info.ptr = player;
        info.valid = true;

        void* h = mono_runtime_invoke(getHealth, player, nullptr, &exc);
        info.health = h ? *(int*)h : 0;

        void* n = mono_runtime_invoke(getName, player, nullptr, &exc);
        if (n) {
            char* s = mono_string_to_utf8(n);
            if (s) info.name = s;
        }

        void* t = mono_runtime_invoke(getTeam, player, nullptr, &exc);
        info.team = t ? *(short*)t : 0;

        void* d = mono_runtime_invoke(getIsDead, player, nullptr, &exc);
        info.isDead = d ? *(bool*)d : false;

        void* b = mono_runtime_invoke(getBiped, player, nullptr, &exc);
        if (b) {
            void* p = mono_runtime_invoke(getPos, b, nullptr, &exc);
            if (p) {
                float* f = (float*)p;
                info.x = f[0]; info.y = f[1]; info.z = f[2];
            }
        }
        result.push_back(info);
    }
    return result;
}

bool GetCameraPosition(float& x, float& y, float& z) {
    void* cameraClass = FindClass("UnityEngine", "Camera");
    void* transformClass = FindClass("UnityEngine", "Transform");
    void* componentClass = FindClass("UnityEngine", "Component");
    if (!cameraClass || !transformClass || !componentClass) return false;

    void* getMain = mono_class_get_method_from_name(cameraClass, "get_main", 0);
    if (!getMain) return false;
    void* cam = mono_runtime_invoke(getMain, nullptr, nullptr, nullptr);
    if (!cam) return false;

    void* getTransform = mono_class_get_method_from_name(componentClass, "get_transform", 0);
    void* trans = mono_runtime_invoke(getTransform, cam, nullptr, nullptr);
    if (!trans) return false;

    void* getPos = mono_class_get_method_from_name(transformClass, "get_position", 0);
    void* pos = mono_runtime_invoke(getPos, trans, nullptr, nullptr);
    if (!pos) return false;

    float* f = (float*)pos;
    x = f[0]; y = f[1]; z = f[2];
    return true;
}
