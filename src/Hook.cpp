// Hook.cpp
#include "Hook.h"
#include "Overlay.h"
#include <d3d11.h>
#include <dxgi.h>
#include "MinHook.h"

typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
static Present_t oPresent = nullptr;
static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_context = nullptr;
static bool g_initialized = false;

HRESULT __stdcall hkPresent(IDXGISwapChain* swapChain, UINT sync, UINT flags) {
    if (!g_initialized) {
        swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_device);
        g_device->GetImmediateContext(&g_context);
        InitImGui(swapChain, g_device, g_context);
        g_initialized = true;
    }
    RenderImGui();
    return oPresent(swapChain, sync, flags);
}

// Получение адреса Present через временный swapchain
static void* GetPresentAddress() {
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0,0, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"DX", nullptr };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0,0,100,100, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferCount = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* swap = nullptr;
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &desc, &swap, &dev, nullptr, &ctx);

    void* present = (*(void***)swap)[8]; // Present index 8
    swap->Release(); dev->Release(); ctx->Release();
    DestroyWindow(hwnd); UnregisterClass(wc.lpszClassName, wc.hInstance);
    return present;
}

bool InitHook() {
    if (MH_Initialize() != MH_OK) return false;
    void* present = GetPresentAddress();
    if (MH_CreateHook(present, &hkPresent, (void**)&oPresent) != MH_OK) return false;
    if (MH_EnableHook(present) != MH_OK) return false;
    return true;
}

void ShutdownHook() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    ShutdownImGui();
}
