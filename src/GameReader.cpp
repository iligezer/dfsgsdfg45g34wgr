#include "GameReader.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

std::ofstream g_log("D:\\vs\\kok\\cheat_log.txt", std::ios::app);

// Глобальные указатели
static void* klassCombatPlayer = nullptr;
static void* klassTransform = nullptr;
static void* klassCamera = nullptr;

void InitClasses() {
    if (klassCombatPlayer) return;
    void* domain = mono_get_root_domain();
    if (!domain) return;
    mono_thread_attach(domain);

    void* assembly = mono_domain_assembly_open(domain, "Assembly-CSharp");
    if (!assembly) return;
    void* image = mono_assembly_get_image(assembly);

    klassCombatPlayer = mono_class_from_name(image, "", "CombatPlayer");

    void* unityAssembly = mono_domain_assembly_open(domain, "UnityEngine");
    if (unityAssembly) {
        void* unityImage = mono_assembly_get_image(unityAssembly);
        klassTransform = mono_class_from_name(unityImage, "UnityEngine", "Transform");
        klassCamera = mono_class_from_name(unityImage, "UnityEngine", "Camera");
    }
}

// Функция для получения имени Transform
std::string GetTransformName(void* transformObj, void* exception) {
    if (!transformObj || !klassTransform) return "";
    void* klass = mono_object_get_class(transformObj);
    if (!klass) return "";
    void* getName = mono_class_get_method_from_name(klass, "get_name", 0);
    if (!getName) return "";
    MonoObject* nameObj = (MonoObject*)mono_runtime_invoke(getName, transformObj, nullptr, &exception);
    if (!nameObj) return "";
    char* nameStr = mono_string_to_utf8(nameObj);
    if (!nameStr) return "";
    return std::string(nameStr);
}

// Рекурсивный поиск головы
void FindHeadBoneRecursive(void* transformObj, int depth, void* exception) {
    if (!transformObj || depth > 10) return; // Ограничим глубину

    std::string name = GetTransformName(transformObj, exception);
    if (!name.empty()) {
        g_log << std::string(depth * 2, ' ') << "Bone: " << name << std::endl;
    }

    // Если нашли голову — выводим позицию
    if (name.find("Head") != std::string::npos) {
        g_log << "[+] HEAD FOUND! Name: " << name << std::endl;
        void* getPosition = mono_class_get_method_from_name(klassTransform, "get_position", 0);
        if (getPosition) {
            MonoObject* posObj = (MonoObject*)mono_runtime_invoke(getPosition, transformObj, nullptr, &exception);
            if (posObj) {
                Vector3* pos = (Vector3*)mono_object_unbox(posObj);
                g_log << "[+] Head position: " << pos->x << ", " << pos->y << ", " << pos->z << std::endl;
            }
        }
    }

    // Рекурсивно обходим детей
    void* getChildCount = mono_class_get_method_from_name(klassTransform, "get_childCount", 0);
    void* getChild = mono_class_get_method_from_name(klassTransform, "GetChild", 1);
    if (!getChildCount || !getChild) return;

    MonoObject* countObj = (MonoObject*)mono_runtime_invoke(getChildCount, transformObj, nullptr, &exception);
    int childCount = 0;
    if (countObj) childCount = *(int*)mono_object_unbox(countObj);

    for (int i = 0; i < childCount; i++) {
        void* args[1] = { &i };
        void* childObj = mono_runtime_invoke(getChild, transformObj, args, &exception);
        if (childObj) {
            FindHeadBoneRecursive(childObj, depth + 1, exception);
        }
    }
}

// Поиск головы через Biped
void FindHeadBone(void* localPlayer) {
    void* exception = nullptr;
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
    g_log << "[+] Biped found! Starting recursive search..." << std::endl;
    FindHeadBoneRecursive(biped, 0, exception);
    g_log << "[+] Recursive search done." << std::endl;
}

void FindAllManagers() {
    InitClasses();
    if (!klassCombatPlayer) {
        g_log << "[-] CombatPlayer class not found" << std::endl;
        return;
    }

    void* domain = mono_get_root_domain();
    mono_thread_attach(domain);
    void* exception = nullptr;

    void* assembly = mono_domain_assembly_open(domain, "Assembly-CSharp");
    if (!assembly) return;
    void* image = mono_assembly_get_image(assembly);

    const char* candidateClasses[] = {
        "LocalPlayerManager", "PlayerManager", "PlayerLocal", "LocalPlayer", "GameManager", "Main"
    };

    for (const char* className : candidateClasses) {
        void* klass = mono_class_from_name(image, "", className);
        if (!klass) continue;

        void* getInstance = mono_class_get_method_from_name(klass, "get_Instance", 0);
        if (!getInstance) continue;

        void* manager = mono_runtime_invoke(getInstance, nullptr, nullptr, &exception);
        if (!manager) {
            g_log << "[-] " << className << ".Instance is null" << std::endl;
            continue;
        }
        g_log << "[+] " << className << ".Instance found!" << std::endl;

        // Пробуем найти LocalPlayer
        void* fieldLocalPlayer = mono_class_get_field_from_name(klass, "LocalPlayer");
        if (fieldLocalPlayer) {
            void* localPlayer = *(void**)((uintptr_t)manager + mono_field_get_offset(fieldLocalPlayer));
            if (localPlayer) {
                g_log << "[+] " << className << ".LocalPlayer found!" << std::endl;
                FindHeadBone(localPlayer);
            } else {
                g_log << "[-] " << className << ".LocalPlayer is null" << std::endl;
            }
        }

        // Пробуем найти Players (Dictionary)
        void* fieldPlayers = mono_class_get_field_from_name(klass, "Players");
        if (fieldPlayers) {
            void* playersDict = *(void**)((uintptr_t)manager + mono_field_get_offset(fieldPlayers));
            if (playersDict) {
                g_log << "[+] " << className << ".Players found!" << std::endl;
                // Можно перебрать, но пока просто отметим
            } else {
                g_log << "[-] " << className << ".Players is null" << std::endl;
            }
        }
    }
}

// --- Заглушки для Overlay ---
std::vector<PlayerData> GetPlayers() {
    return {};
}

Vector2 WorldToScreen(Vector3 worldPos) {
    return { 0, 0 };
}

void AimAt(Vector3 targetPos) {
}

void FindHeadBone() {
    FindAllManagers();
}
