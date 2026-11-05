#include <windows.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdio>

// ============================================================
// ТИПЫ ФУНКЦИЙ MONO (загружаются через GetProcAddress)
// ============================================================
typedef void* (*mono_get_root_domain_t)();
typedef void  (*mono_thread_attach_t)(void* domain);
typedef void* (*mono_class_from_name_t)(void* image, const char* ns, const char* name);
typedef void* (*mono_class_get_method_from_name_t)(void* klass, const char* name, int param_count);
typedef void* (*mono_runtime_invoke_t)(void* method, void* obj, void** params, void** exc);
typedef void* (*mono_object_get_class_t)(void* obj);
typedef void* (*mono_class_get_type_t)(void* klass);
typedef void* (*mono_type_get_object_t)(void* domain, void* type);
typedef int   (*mono_array_length_t)(void* arr);
typedef char* (*mono_array_addr_with_size_t)(void* arr, int size, int index);
typedef char* (*mono_string_to_utf8_t)(void* str);
typedef void* (*mono_assembly_get_image_t)(void* assembly);
typedef void* (*mono_image_loaded_t)(const char* name);
typedef void* (*mono_class_get_parent_t)(void* klass);
typedef void* (*mono_class_get_name_t)(void* klass);

// ============================================================
// УКАЗАТЕЛИ НА ФУНКЦИИ
// ============================================================
mono_get_root_domain_t          mono_get_root_domain          = nullptr;
mono_thread_attach_t            mono_thread_attach            = nullptr;
mono_class_from_name_t          mono_class_from_name          = nullptr;
mono_class_get_method_from_name_t mono_class_get_method_from_name = nullptr;
mono_runtime_invoke_t           mono_runtime_invoke           = nullptr;
mono_object_get_class_t         mono_object_get_class         = nullptr;
mono_class_get_type_t           mono_class_get_type           = nullptr;
mono_type_get_object_t          mono_type_get_object          = nullptr;
mono_array_length_t             mono_array_length             = nullptr;
mono_array_addr_with_size_t     mono_array_addr_with_size     = nullptr;
mono_string_to_utf8_t           mono_string_to_utf8           = nullptr;
mono_image_loaded_t             mono_image_loaded             = nullptr;
mono_class_get_name_t           mono_class_get_name           = nullptr;

// ============================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================
void* rootDomain = nullptr;
void* image = nullptr;
std::ofstream logFile;

// ============================================================
// ФУНКЦИЯ ЗАГРУЗКИ MONO
// ============================================================
bool LoadMono() {
    HMODULE hMono = GetModuleHandleA("mono.dll");
    if (!hMono) hMono = GetModuleHandleA("mono-2.0-bdwgc.dll");
    if (!hMono) hMono = GetModuleHandleA("mono-2.0-sgen.dll");
    if (!hMono) hMono = GetModuleHandleA("mono-2.0.dll");
    if (!hMono) {
        // Пробуем найти через перебор модулей
        HMODULE modules[1024];
        DWORD needed;
        if (EnumProcessModules(GetCurrentProcess(), modules, sizeof(modules), &needed)) {
            for (DWORD i = 0; i < needed / sizeof(HMODULE); i++) {
                char name[MAX_PATH];
                if (GetModuleFileNameA(modules[i], name, MAX_PATH)) {
                    if (strstr(name, "mono") != nullptr) {
                        hMono = modules[i];
                        break;
                    }
                }
            }
        }
    }
    if (!hMono) return false;

    mono_get_root_domain          = (mono_get_root_domain_t)GetProcAddress(hMono, "mono_get_root_domain");
    mono_thread_attach            = (mono_thread_attach_t)GetProcAddress(hMono, "mono_thread_attach");
    mono_class_from_name          = (mono_class_from_name_t)GetProcAddress(hMono, "mono_class_from_name");
    mono_class_get_method_from_name = (mono_class_get_method_from_name_t)GetProcAddress(hMono, "mono_class_get_method_from_name");
    mono_runtime_invoke           = (mono_runtime_invoke_t)GetProcAddress(hMono, "mono_runtime_invoke");
    mono_object_get_class         = (mono_object_get_class_t)GetProcAddress(hMono, "mono_object_get_class");
    mono_class_get_type           = (mono_class_get_type_t)GetProcAddress(hMono, "mono_class_get_type");
    mono_type_get_object          = (mono_type_get_object_t)GetProcAddress(hMono, "mono_type_get_object");
    mono_array_length             = (mono_array_length_t)GetProcAddress(hMono, "mono_array_length");
    mono_array_addr_with_size     = (mono_array_addr_with_size_t)GetProcAddress(hMono, "mono_array_addr_with_size");
    mono_string_to_utf8           = (mono_string_to_utf8_t)GetProcAddress(hMono, "mono_string_to_utf8");
    mono_image_loaded             = (mono_image_loaded_t)GetProcAddress(hMono, "mono_image_loaded");
    mono_class_get_name           = (mono_class_get_name_t)GetProcAddress(hMono, "mono_class_get_name");

    return true;
}

// ============================================================
// ПОИСК КЛАССА ВО ВСЕХ СБОРКАХ
// ============================================================
void* FindClass(const char* namespace_, const char* className) {
    // Пробуем Assembly-CSharp
    if (!image) {
        image = mono_image_loaded("Assembly-CSharp");
        if (!image) image = mono_image_loaded("Assembly-CSharp.dll");
    }
    if (image) {
        void* klass = mono_class_from_name(image, namespace_, className);
        if (klass) return klass;
    }

    // Если не нашли — перебираем все сборки
    // (в реальном чите нужно получить список сборок через mono_assembly_foreach,
    // но для простоты ограничимся Assembly-CSharp и mscorlib)
    void* mscorlib = mono_image_loaded("mscorlib");
    if (mscorlib) {
        void* klass = mono_class_from_name(mscorlib, namespace_, className);
        if (klass) return klass;
    }
    return nullptr;
}

// ============================================================
// ВЫЗОВ МЕТОДА ИЗ ЭКЗЕМПЛЯРА
// ============================================================
void* InvokeMethod(void* klass, void* obj, const char* methodName, int paramCount, void** params) {
    void* method = mono_class_get_method_from_name(klass, methodName, paramCount);
    if (!method) return nullptr;
    void* exc = nullptr;
    return mono_runtime_invoke(method, obj, params, &exc);
}

// ============================================================
// ПОЛУЧЕНИЕ СТРОКИ ИЗ MONO STRING
// ============================================================
std::string GetMonoString(void* str) {
    if (!str) return "";
    char* utf8 = mono_string_to_utf8(str);
    if (!utf8) return "";
    std::string result(utf8);
    // mono_free(utf8); // если есть, но не обязательно
    return result;
}

// ============================================================
// ОСНОВНАЯ ЛОГИКА
// ============================================================
void DumpAllPlayers() {
    logFile << "========================================\n";
    logFile << "[*] DumpAllPlayers started\n";

    // 1. Получаем root domain
    rootDomain = mono_get_root_domain();
    if (!rootDomain) {
        logFile << "[-] rootDomain is null\n";
        return;
    }
    mono_thread_attach(rootDomain);
    logFile << "[+] rootDomain = " << rootDomain << "\n";

    // 2. Находим классы
    void* playerManagerClass = FindClass("", "PlayerManager");
    void* combatPlayerClass  = FindClass("", "CombatPlayer");
    void* transformClass     = FindClass("UnityEngine", "Transform");
    void* objectClass        = FindClass("UnityEngine", "Object");

    if (!playerManagerClass) { logFile << "[-] PlayerManager not found\n"; return; }
    if (!combatPlayerClass)  { logFile << "[-] CombatPlayer not found\n"; return; }
    if (!transformClass)     { logFile << "[-] Transform not found\n"; return; }
    if (!objectClass)        { logFile << "[-] Object not found\n"; return; }

    logFile << "[+] PlayerManager = " << playerManagerClass << "\n";
    logFile << "[+] CombatPlayer = " << combatPlayerClass << "\n";
    logFile << "[+] Transform = " << transformClass << "\n";
    logFile << "[+] Object = " << objectClass << "\n";

    // 3. Получаем System.Type для CombatPlayer
    void* type = mono_class_get_type(combatPlayerClass);
    void* typeObj = mono_type_get_object(rootDomain, type);
    if (!typeObj) {
        logFile << "[-] typeObj is null (mono_type_get_object failed)\n";
        return;
    }
    logFile << "[+] typeObj = " << typeObj << "\n";

    // 4. Вызываем FindObjectsOfType(Type)
    void* findMethod = mono_class_get_method_from_name(objectClass, "FindObjectsOfType", 1);
    if (!findMethod) {
        logFile << "[-] FindObjectsOfType not found\n";
        return;
    }

    void* params[1] = { typeObj };
    void* exc = nullptr;
    void* arr = mono_runtime_invoke(findMethod, nullptr, params, &exc);
    if (!arr) {
        logFile << "[-] FindObjectsOfType returned null\n";
        return;
    }

    int len = mono_array_length(arr);
    logFile << "[+] FindObjectsOfType returned array of " << len << " CombatPlayer\n";

    // 5. Методы для вызова
    void* methodGetHealth = mono_class_get_method_from_name(combatPlayerClass, "get_Health", 0);
    void* methodGetName   = mono_class_get_method_from_name(combatPlayerClass, "get_Name", 0);
    void* methodGetTeam   = mono_class_get_method_from_name(combatPlayerClass, "get_Team", 0);
    void* methodGetIsDead = mono_class_get_method_from_name(combatPlayerClass, "get_IsDead", 0);
    void* methodGetBiped  = mono_class_get_method_from_name(combatPlayerClass, "get_Biped", 0);
    void* methodGetPos    = mono_class_get_method_from_name(transformClass, "get_position", 0);

    logFile << "[+] get_Health = " << methodGetHealth << "\n";
    logFile << "[+] get_Name   = " << methodGetName << "\n";
    logFile << "[+] get_Team   = " << methodGetTeam << "\n";
    logFile << "[+] get_IsDead = " << methodGetIsDead << "\n";
    logFile << "[+] get_Biped  = " << methodGetBiped << "\n";
    logFile << "[+] get_position = " << methodGetPos << "\n";

    // 6. Проходим по массиву
    for (int i = 0; i < len; i++) {
        void* player = *(void**)((char*)arr + 0x20 + i * 8); // элементы массива
        if (!player) {
            logFile << "\n[" << i << "] CombatPlayer = null\n";
            continue;
        }

        logFile << "\n[" << i << "] CombatPlayer = " << player << "\n";

        // get_Health
        void* healthObj = mono_runtime_invoke(methodGetHealth, player, nullptr, &exc);
        int health = healthObj ? *(int*)healthObj : -1;
        logFile << "    Health: " << health << "\n";

        // get_Name
        void* nameObj = mono_runtime_invoke(methodGetName, player, nullptr, &exc);
        std::string name = GetMonoString(nameObj);
        logFile << "    Name: " << name << "\n";

        // get_Team
        void* teamObj = mono_runtime_invoke(methodGetTeam, player, nullptr, &exc);
        short team = teamObj ? *(short*)teamObj : -1;
        logFile << "    Team: " << team << "\n";

        // get_IsDead
        void* deadObj = mono_runtime_invoke(methodGetIsDead, player, nullptr, &exc);
        bool isDead = deadObj ? *(bool*)deadObj : false;
        logFile << "    IsDead: " << (isDead ? "true" : "false") << "\n";

        // get_Biped
        void* bipedObj = mono_runtime_invoke(methodGetBiped, player, nullptr, &exc);
        void* biped = bipedObj; // это указатель на Transform
        logFile << "    Biped: " << biped << "\n";

        // get_position (если Biped валиден)
        if (biped && methodGetPos) {
            void* posObj = mono_runtime_invoke(methodGetPos, biped, nullptr, &exc);
            if (posObj) {
                // Vector3 — структура из 3 float. В Mono она возвращается как
                // указатель на буфер или через регистры. Попробуем прочитать как float*.
                float* pos = (float*)posObj;
                logFile << "    Position: (" << pos[0] << ", " << pos[1] << ", " << pos[2] << ")\n";
            } else {
                logFile << "    Position: get_position returned null\n";
            }
        }
    }

    logFile << "\n[+] DumpAllPlayers finished\n";
}

// ============================================================
// ТОЧКА ВХОДА
// ============================================================
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        // Открываем лог-файл
        logFile.open("C:\\Users\\Unknown\\Desktop\\Новая папка (8)\\КОК.txt", std::ios::out | std::ios::trunc);
        if (!logFile.is_open()) {
            MessageBoxA(nullptr, "Не удалось открыть лог-файл", "Error", MB_OK);
            return FALSE;
        }

        logFile << "[+] DLL loaded\n";

        // Загружаем Mono
        if (!LoadMono()) {
            logFile << "[-] Не удалось загрузить mono.dll или найти экспорты\n";
            logFile.close();
            return FALSE;
        }
        logFile << "[+] Mono loaded\n";

        // Запускаем дамп в отдельном потоке (чтобы не блокировать DllMain)
        CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)DumpAllPlayers, nullptr, 0, nullptr);
    }
    return TRUE;
}
