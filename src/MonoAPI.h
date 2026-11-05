#pragma once
#include <windows.h>

// Заглушка для MonoObject (нужна для unboxing)
struct MonoObject {
    void* vtable;
    void* sync;
};

// Структура Vector3 для Unity
struct Vector3 {
    float x, y, z;
};

struct Vector2 {
    float x, y;
};

// Указатели на функции Mono
extern void* (*mono_get_root_domain)();
extern void* (*mono_thread_attach)(void*);
extern void* (*mono_domain_assembly_open)(void*, const char*);
extern void* (*mono_assembly_get_image)(void*);
extern void* (*mono_class_from_name)(void*, const char*, const char*);
extern void* (*mono_class_get_method_from_name)(void*, const char*, int);
extern void* (*mono_runtime_invoke)(void*, void*, void**, void**);
extern void* (*mono_object_get_class)(void*);
extern int   (*mono_array_length)(void*);
extern void* (*mono_array_addr)(void*, int);
extern void* (*mono_object_unbox)(void*);

// Инициализация
bool InitMono();
