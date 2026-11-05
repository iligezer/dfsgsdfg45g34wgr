#include "GameReader.h"
#include <iostream>

std::vector<PlayerData> GetPlayers() {
    std::vector<PlayerData> players;
    void* domain = mono_get_root_domain();
    if (!domain) return players;
    mono_thread_attach(domain);

    void* assembly = mono_domain_assembly_open(domain, "CCNetcodeRuntime");
    if (!assembly) assembly = mono_domain_assembly_open(domain, "Assembly-CSharp");
    if (!assembly) return players;
    
    void* image = mono_assembly_get_image(assembly);

    void* klass = mono_class_from_name(image, "CCNetcodeRuntime", "CombatPlayer");
    if (!klass) return players;

    void* klassUnityObject = mono_class_from_name(image, "UnityEngine", "Object");
    if (!klassUnityObject) return players;
    void* methodFind = mono_class_get_method_from_name(klassUnityObject, "FindObjectsOfType", 1);
    if (!methodFind) return players;

    void* args[1] = { klass };
    void* exception = nullptr;
    void* arrayObj = mono_runtime_invoke(methodFind, nullptr, args, &exception);
    if (!arrayObj) return players;

    int length = mono_array_length(arrayObj);
    for (int i = 0; i < length; i++) {
        void* playerObj = mono_array_addr(arrayObj, i);
        if (!playerObj) continue;

        PlayerData pData;
        pData.name = "Player_" + std::to_string(i);

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
