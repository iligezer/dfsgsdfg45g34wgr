#include "Overlay.h"
#include "GameReader.h"
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <fstream>

#pragma comment(lib, "d3d11.lib")

// Указатели для хука
typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
Present_t oPresent = nullptr;
WNDPROC oWndProc = nullptr;

bool showESP = true;
bool showLines = true;
bool aimbotEnabled = false;
float espColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

// Оригинальный WndProc
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

// Хук Present
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    static bool init = false;
    if (!init) {
        // Получаем устройство и контекст
        ID3D11Device* device = nullptr;
        pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&device);
        ID3D11DeviceContext* context = nullptr;
        device->GetImmediateContext(&context);

        // Инициализация ImGui
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(GetActiveWindow());
        ImGui_ImplDX11_Init(device, context);

        // Хук на WndProc (для кликов)
        oWndProc = (WNDPROC)SetWindowLongPtr(GetActiveWindow(), GWLP_WNDPROC, (LONG_PTR)WndProc);
        init = true;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // --- ESP ЛОГИКА ---
    auto players = GetPlayers();
    if (showESP) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        for (const auto& p : players) {
            Vector2 screenPos = WorldToScreen(p.position, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            Vector2 headPos = WorldToScreen(p.head, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            if (screenPos.x > 0 && screenPos.y > 0) {
                char buf[128];
                sprintf_s(buf, "Player: %s", p.name.c_str());
                drawList->AddText(ImVec2(screenPos.x, screenPos.y), IM_COL32(255, 255, 0, 255), buf);
                if (showLines) {
                    drawList->AddLine(ImVec2(screenPos.x, screenPos.y), ImVec2(headPos.x, headPos.y), IM_COL32(255, 0, 0, 255), 2.0f);
                }
            }
        }
    }

    // Меню
    ImGui::Begin("Cheat Menu");
    ImGui::Checkbox("Enable ESP", &showESP);
    ImGui::Checkbox("Show Lines", &showLines);
    ImGui::Checkbox("Enable Aimbot", &aimbotEnabled);
    ImGui::Text("Players found: %d", (int)players.size());
    ImGui::End();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}

// Функция для создания хука
void StartOverlay() {
    // Создаем фиктивное устройство, чтобы получить указатель на Present
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"DX11Test", NULL };
    RegisterClassEx(&wc);
    HWND hWnd = CreateWindow(L"DX11Test", L"DX11Test", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;
    IDXGISwapChain* d3dSwapChain = nullptr;
    D3D_FEATURE_LEVEL featureLevel;
    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &d3dSwapChain, &d3dDevice, &featureLevel, &d3dContext);

    // Получаем адрес Present из виртуальной таблицы
    void** pVTable = *(void***)d3dSwapChain;
    void* pPresent = pVTable[8]; // Present — 8-й индекс

    // Устанавливаем хук
    MH_Initialize();
    MH_CreateHook(pPresent, &hkPresent, (void**)&oPresent);
    MH_EnableHook(pPresent);

    // Очистка фиктивного устройства
    d3dSwapChain->Release();
    d3dDevice->Release();
    d3dContext->Release();
    DestroyWindow(hWnd);
    UnregisterClass(L"DX11Test", wc.hInstance);
}
