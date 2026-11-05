#include "MonoAPI.h"
#include <iostream>

void* (*mono_get_root_domain)() = nullptr;
void* (*mono_thread_attach)(void*) = nullptr;
void* (*mono_domain_assembly_open)(void*, const char*) = nullptr;
void* (*mono_assembly_get_image)(void*) = nullptr;
void* (*mono_class_from_name)(void*, const char*, const char*) = nullptr;
void* (*mono_class_get_method_from_name)(void*, const char*, int) = nullptr;
void* (*mono_runtime_invoke)(void*, void*, void**, void**) = nullptr;
void* (*mono_object_get_class)(void*) = nullptr;
int   (*mono_array_length)(void*) = nullptr;
void* (*mono_array_addr)(void*, int) = nullptr;
void* (*mono_object_unbox)(void*) = nullptr;

bool InitMono() {
    HMODULE hMono = GetModuleHandleA("mono.dll");
    if (!hMono) {
        hMono = LoadLibraryA("mono.dll");
        if (!hMono) {
            MessageBoxA(0, "mono.dll not found!", "Error", MB_ICONERROR);
            return false;
        }
    }

    mono_get_root_domain = (decltype(mono_get_root_domain))GetProcAddress(hMono, "mono_get_root_domain");
    mono_thread_attach = (decltype(mono_thread_attach))GetProcAddress(hMono, "mono_thread_attach");
    mono_domain_assembly_open = (decltype(mono_domain_assembly_open))GetProcAddress(hMono, "mono_domain_assembly_open");
    mono_assembly_get_image = (decltype(mono_assembly_get_image))GetProcAddress(hMono, "mono_assembly_get_image");
    mono_class_from_name = (decltype(mono_class_from_name))GetProcAddress(hMono, "mono_class_from_name");
    mono_class_get_method_from_name = (decltype(mono_class_get_method_from_name))GetProcAddress(hMono, "mono_class_get_method_from_name");
    mono_runtime_invoke = (decltype(mono_runtime_invoke))GetProcAddress(hMono, "mono_runtime_invoke");
    mono_object_get_class = (decltype(mono_object_get_class))GetProcAddress(hMono, "mono_object_get_class");
    mono_array_length = (decltype(mono_array_length))GetProcAddress(hMono, "mono_array_length");
    mono_array_addr = (decltype(mono_array_addr))GetProcAddress(hMono, "mono_array_addr_with_size");
    mono_object_unbox = (decltype(mono_object_unbox))GetProcAddress(hMono, "mono_object_unbox");

    return mono_get_root_domain != nullptr && mono_class_from_name != nullptr;
}
