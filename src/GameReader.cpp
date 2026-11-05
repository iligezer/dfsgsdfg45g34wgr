std::vector<PlayerData> GetPlayers() {
    std::vector<PlayerData> players;
    void* domain = mono_get_root_domain();
    if (!domain) return players;
    mono_thread_attach(domain);

    void* assembly = mono_domain_assembly_open(domain, "Assembly-CSharp");
    if (!assembly) {
        g_log << "[-] Assembly-CSharp not found" << std::endl;
        return players;
    }
    void* image = mono_assembly_get_image(assembly);

    void* klassPlayerManager = mono_class_from_name(image, "", "PlayerManager");
    if (!klassPlayerManager) {
        g_log << "[-] PlayerManager class not found" << std::endl;
        return players;
    }

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

    // === ХИТРЫЙ ПУТЬ: Читаем внутренние поля Dictionary ===
    
    // Получаем класс Dictionary
    void* klassDict = mono_object_get_class(playersDictObj);
    
    // Ищем поле "_entries" (в старых версиях Mono оно может называться "entries" или "m_Entries")
    void* fieldEntries = mono_class_get_field_from_name(klassDict, "_entries");
    if (!fieldEntries) fieldEntries = mono_class_get_field_from_name(klassDict, "entries");
    if (!fieldEntries) {
        g_log << "[-] Dictionary._entries field not found" << std::endl;
        return players;
    }
    int entriesOffset = mono_field_get_offset(fieldEntries);
    
    // Ищем поле "_count" (количество элементов)
    void* fieldCount = mono_class_get_field_from_name(klassDict, "_count");
    if (!fieldCount) fieldCount = mono_class_get_field_from_name(klassDict, "count");
    if (!fieldCount) {
        g_log << "[-] Dictionary._count field not found" << std::endl;
        return players;
    }
    int countOffset = mono_field_get_offset(fieldCount);

    // Читаем длину массива entries
    int count = *(int*)((uintptr_t)playersDictObj + countOffset);
    // Читаем указатель на массив entries
    void* entriesArrayObj = *(void**)((uintptr_t)playersDictObj + entriesOffset);

    if (!entriesArrayObj || count == 0) {
        g_log << "[-] Dictionary is empty or entries array is null" << std::endl;
        return players;
    }

    // В Mono массив структур Entry имеет заголовок. Данные начинаются после 0x18 (или 0x10 в 32-бит).
    // В 32-битной системе смещение данных в массиве обычно 0x10 (16 байт).
    // Но проще использовать mono_array_addr_with_size, если он у нас есть.
    // Если нет, то можно вычислить вручную. 
    // Каждый Entry в Dictionary<TKey, TValue> занимает: int hashCode (4), int next (4), TKey key (4), TValue value (4) = 16 байт.
    
    // В нашем MonoAPI есть mono_array_addr. Используем его.
    // Но mono_array_addr для массива структур требует знания размера элемента. 
    // В нашем случае размер Entry = 16 байт.
    
    // Однако, для 32-битной игры, заголовок массива занимает 16 байт (2 указателя).
    // Данные начинаются с +0x10 (16).
    // И каждый Entry весит 16 байт.
    
    // Проходим по count элементов
    for (int i = 0; i < count; i++) {
        // Получаем адрес i-го элемента
        // Смещение данных: 0x10 (заголовок) + i * 16 (размер Entry)
        uintptr_t entryAddr = (uintptr_t)entriesArrayObj + 0x10 + (i * 16);
        
        // В структуре Entry поле value идет после hashCode (4) + next (4) + key (4) = 12 байт.
        // Но из-за выравнивания, value может начинаться с 12 или 16.
        // Посмотрим на структуру: int hashCode, int next, TKey key, TValue value.
        // В 32-битной системе: 4 + 4 + 4 + 4 = 16. value на смещении 12 (0xC).
        
        void* playerObj = *(void**)(entryAddr + 12);
        
        // Проверяем, что это не null (в Dictionary могут быть пустые ячейки)
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

        // Дополнительно можно получить имя игрока, если нужно
        // (поле Name в CombatPlayer)
        // void* fieldName = mono_class_get_field_from_name(klassPlayer, "Name");
        // ...

        players.push_back(pData);
    }

    g_log << "[+] Found " << players.size() << " players via Dictionary entries" << std::endl;
    return players;
}
