#pragma once
#include <windows.h>

typedef void* (*mono_get_root_domain_t)();
typedef void  (*mono_thread_attach_t)(void* domain);
typedef void* (*mono_class_from_name_t)(void* image, const char* ns, const char* name);
typedef void* (*mono_class_get_method_from_name_t)(void* klass, const char* name, int param_count);
typedef void* (*mono_runtime_invoke_t)(void* method, void* obj, void** params, void** exc);
typedef void* (*mono_class_get_type_t)(void* klass);
typedef void* (*mono_type_get_object_t)(void* domain, void* type);
typedef int   (*mono_array_length_t)(void* arr);
typedef char* (*mono_string_to_utf8_t)(void* str);
typedef void* (*mono_image_loaded_t)(const char* name);

extern mono_get_root_domain_t          mono_get_root_domain;
extern mono_thread_attach_t            mono_thread_attach;
extern mono_class_from_name_t          mono_class_from_name;
extern mono_class_get_method_from_name_t mono_class_get_method_from_name;
extern mono_runtime_invoke_t           mono_runtime_invoke;
extern mono_class_get_type_t           mono_class_get_type;
extern mono_type_get_object_t          mono_type_get_object;
extern mono_array_length_t             mono_array_length;
extern mono_string_to_utf8_t           mono_string_to_utf8;
extern mono_image_loaded_t             mono_image_loaded;

bool LoadMono();
void* FindClass(const char* ns, const char* name);
