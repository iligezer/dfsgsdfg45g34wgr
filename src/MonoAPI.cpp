#include "MonoAPI.h"

mono_get_root_domain_t          mono_get_root_domain = nullptr;
mono_thread_attach_t            mono_thread_attach = nullptr;
mono_class_from_name_t          mono_class_from_name = nullptr;
mono_class_get_method_from_name_t mono_class_get_method_from_name = nullptr;
mono_runtime_invoke_t           mono_runtime_invoke = nullptr;
mono_class_get_type_t           mono_class_get_type = nullptr;
mono_type_get_object_t          mono_type_get_object = nullptr;
mono_array_length_t             mono_array_length = nullptr;
mono_string_to_utf8_t           mono_string_to_utf8 = nullptr;
mono_image_loaded_t             mono_image_loaded = nullptr;

bool LoadMono() {
    HMODULE hMono = GetModuleHandleA("mono.dll");
    if (!hMono) hMono = GetModuleHandleA("mono-2.0-bdwgc.dll");
    if (!hMono) hMono = GetModuleHandleA("mono-2.0-sgen.dll");
    if (!hMono) return false;

    mono_get_root_domain = (mono_get_root_domain_t)GetProcAddress(hMono, "mono_get_root_domain");
    mono_thread_attach = (mono_thread_attach_t)GetProcAddress(hMono, "mono_thread_attach");
    mono_class_from_name = (mono_class_from_name_t)GetProcAddress(hMono, "mono_class_from_name");
    mono_class_get_method_from_name = (mono_class_get_method_from_name_t)GetProcAddress(hMono, "mono_class_get_method_from_name");
    mono_runtime_invoke = (mono_runtime_invoke_t)GetProcAddress(hMono, "mono_runtime_invoke");
    mono_class_get_type = (mono_class_get_type_t)GetProcAddress(hMono, "mono_class_get_type");
    mono_type_get_object = (mono_type_get_object_t)GetProcAddress(hMono, "mono_type_get_object");
    mono_array_length = (mono_array_length_t)GetProcAddress(hMono, "mono_array_length");
    mono_string_to_utf8 = (mono_string_to_utf8_t)GetProcAddress(hMono, "mono_string_to_utf8");
    mono_image_loaded = (mono_image_loaded_t)GetProcAddress(hMono, "mono_image_loaded");

    return mono_get_root_domain && mono_thread_attach && mono_class_from_name;
}

void* FindClass(const char* ns, const char* name) {
    void* image = mono_image_loaded("Assembly-CSharp");
    if (!image) image = mono_image_loaded("Assembly-CSharp.dll");
    if (!image) return nullptr;
    return mono_class_from_name(image, ns, name);
}
