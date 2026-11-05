#include "Overlay.h"
#include "GameReader.h"
#include <d3d11.h>
#include <dxgi.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")

// Указатели на оригинальные функции
typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
Present_t oPresent = nullptr;
WNDPROC oWndProc = nullptr;

bool showESP = true;
bool showLines = true;
bool aimbotEnabled = false;

// Объявление функции ImGui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Наш WndProc для перехвата кликов мыши
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

// Наш хук Present (рисует поверх игры)
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    static bool init = false;
    if (!init) {
        ID3D11Device* device = nullptr;
        pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&device);
        ID3D11DeviceContext* context = nullptr;
        device->GetImmediateContext(&context);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplWin32_Init(GetActiveWindow());
        ImGui_ImplDX11_Init(device, context);

        oWndProc = (WNDPROC)SetWindowLongPtr(GetActiveWindow(), GWLP_WNDPROC, (LONG_PTR)WndProc);
        init = true;
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // --- ЛОГИКА ESP ---
    auto players = GetPlayers();
    if (showESP) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);
        for (const auto& p : players) {
            Vector2 pos = WorldToScreen(p.position, sw, sh);
            Vector2 head = WorldToScreen(p.head, sw, sh);
            if (pos.x > 0 && pos.y > 0) {
                char buf[128]; sprintf_s(buf, "Player: %s", p.name.c_str());
                drawList->AddText(ImVec2(pos.x, pos.y), IM_COL32(255, 255, 0, 255), buf);
                if (showLines) drawList->AddLine(ImVec2(pos.x, pos.y), ImVec2(head.x, head.y), IM_COL32(255, 0, 0, 255), 2.0f);
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

void StartOverlay() {
    // 1. Создаем фиктивное окно и устройство, чтобы достать VTable
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "DX11Test", NULL };
    RegisterClassEx(&wc);
    HWND hWnd = CreateWindow("DX11Test", "DX11Test", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);

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

    // 2. Получаем адрес Present (8-й индекс в VTable)
    void** pVTable = *(void***)d3dSwapChain;
    void* pPresent = pVTable[8];

    // 3. Меняем его на наш хук (VirtualProtect для снятия защиты памяти)
    DWORD oldProtect;
    VirtualProtect(&pVTable[8], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
    oPresent = (Present_t)pPresent;
    pVTable[8] = &hkPresent;
    VirtualProtect(&pVTable[8], sizeof(void*), oldProtect, &oldProtect);

    // Очистка фиктивного окна
    d3dSwapChain->Release();
    d3dDevice->Release();
    d3dContext->Release();
    DestroyWindow(hWnd);
    UnregisterClass("DX11Test", wc.hInstance);
}
