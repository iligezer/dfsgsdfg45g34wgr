#include "GameReader.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

std::ofstream g_log("D:\\vs\\kok\\cheat_log.txt", std::ios::app);

// Глобальные указатели на классы
static void* klassLocalPlayerManager = nullptr;
static void* klassCombatPlayer = nullptr;
static void* klassTransform = nullptr;

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
        klassTransform = mono_class_from_name(unityImage, "UnityEngine", "Transform");
    }
}

// Функция для получения имени Transform
std::string GetTransformName(void* transformObj, void* exception) {
    if (!transformObj || !klassTransform) return "";

    // Получаем класс Transform
    void* klass = mono_object_get_class(transformObj);
    if (!klass) return "";

    // Вызываем get_name() (наследуется от UnityEngine.Object)
    void* getName = mono_class_get_method_from_name(klass, "get_name", 0);
    if (!getName) return "";

    MonoObject* nameObj = (MonoObject*)mono_runtime_invoke(getName, transformObj, nullptr, &exception);
    if (!nameObj) return "";

    // Конвертируем MonoString в char*
    char* nameStr = mono_string_to_utf8(nameObj);
    if (!nameStr) return "";

    std::string result(nameStr);
    return result;
}

void FindHeadBone() {
    InitClasses();
    if (!klassLocalPlayerManager || !klassCombatPlayer || !klassTransform) {
        g_log << "[-] Classes not found" << std::endl;
        return;
    }

    void* domain = mono_get_root_domain();
    mono_thread_attach(domain);
    void* exception = nullptr;

    // 1. Получаем LocalPlayerManager.Instance
    void* getInstance = mono_class_get_method_from_name(klassLocalPlayerManager, "get_Instance", 0);
    if (!getInstance) {
        g_log << "[-] get_Instance not found" << std::endl;
        return;
    }
    void* manager = mono_runtime_invoke(getInstance, nullptr, nullptr, &exception);
    if (!manager) {
        g_log << "[-] Manager is null (not in game)" << std::endl;
        return;
    }

    // 2. Получаем LocalPlayer
    void* fieldLocalPlayer = mono_class_get_field_from_name(klassLocalPlayerManager, "LocalPlayer");
    if (!fieldLocalPlayer) {
        g_log << "[-] LocalPlayer field not found" << std::endl;
        return;
    }
    void* localPlayer = *(void**)((uintptr_t)manager + mono_field_get_offset(fieldLocalPlayer));
    if (!localPlayer) {
        g_log << "[-] LocalPlayer is null" << std::endl;
        return;
    }
    g_log << "[+] LocalPlayer found!" << std::endl;

    // 3. Получаем Biped (Transform скелета)
    void* getBiped = mono_class_get_method_from_name(klassCombatPlayer, "get_Biped", 0);
    if (!getBiped) {
        g_log << "[-] get_Biped not found" << std::endl;
        return;
    }
    void* biped = mono_runtime_invoke(getBiped, localPlayer, nullptr, &exception);
    if (!biped) {
        g_log << "[-] Biped is null" << std::endl;
        return;
    }
    g_log << "[+] Biped found!" << std::endl;

    // 4. Перебираем всех детей Biped и ищем голову
    void* getChildCount = mono_class_get_method_from_name(klassTransform, "get_childCount", 0);
    if (!getChildCount) {
        g_log << "[-] get_childCount not found" << std::endl;
        return;
    }
    MonoObject* countObj = (MonoObject*)mono_runtime_invoke(getChildCount, biped, nullptr, &exception);
    int childCount = 0;
    if (countObj) {
        childCount = *(int*)mono_object_unbox(countObj);
    }
    g_log << "[+] Biped has " << childCount << " children" << std::endl;

    void* getChild = mono_class_get_method_from_name(klassTransform, "GetChild", 1);
    if (!getChild) {
        g_log << "[-] GetChild not found" << std::endl;
        return;
    }

    void* getPosition = mono_class_get_method_from_name(klassTransform, "get_position", 0);
    if (!getPosition) {
        g_log << "[-] get_position not found" << std::endl;
        return;
    }

    for (int i = 0; i < childCount; i++) {
        void* args[1] = { &i };
        void* childObj = mono_runtime_invoke(getChild, biped, args, &exception);
        if (!childObj) continue;

        std::string name = GetTransformName(childObj, exception);
        g_log << "  Child[" << i << "]: " << name << std::endl;

        // Ищем голову (может быть "Bip01 Head", "Bip001 Head", "Head")
        if (name.find("Head") != std::string::npos) {
            g_log << "[+] HEAD FOUND! Name: " << name << std::endl;

            // Получаем позицию головы
            MonoObject* posObj = (MonoObject*)mono_runtime_invoke(getPosition, childObj, nullptr, &exception);
            if (posObj) {
                Vector3* pos = (Vector3*)mono_object_unbox(posObj);
                g_log << "[+] Head position: " << pos->x << ", " << pos->y << ", " << pos->z << std::endl;
            }
        }
    }

    g_log << "[+] Done searching children" << std::endl;
}

std::vector<PlayerData> GetPlayers() {
    // Пока возвращаем пустой список, чтобы не ломать Overlay
    return {};
}

Vector2 WorldToScreen(Vector3 worldPos) {
    return { 0, 0 };
}

void AimAt(Vector3 targetPos) {
}
