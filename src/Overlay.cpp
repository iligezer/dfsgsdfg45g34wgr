#include "Overlay.h"
#include "GameReader.h"
#include <d3d11.h>
#include <dxgi.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <iostream>

#pragma comment(lib, "d3d11.lib")

typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
Present_t oPresent = nullptr;
WNDPROC oWndProc = nullptr;

// Настройки
bool showMenu = true;
bool espEnabled = true;
bool espBox = true;
bool espLines = true;
bool espHealth = true;
bool espDistance = true;
bool espName = true;
bool aimbotEnabled = false;
float aimbotFov = 90.0f;
bool aimbotKey = false;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
}

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

    // Обработка открытия меню по клавише '='
    if (GetAsyncKeyState(VK_OEM_PLUS) & 1) showMenu = !showMenu;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // --- ЛОГИКА ESP ---
    auto players = GetPlayers();
    if (espEnabled) {
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        float screenW = ImGui::GetIO().DisplaySize.x;
        float screenH = ImGui::GetIO().DisplaySize.y;

        for (const auto& p : players) {
            if (p.isDead) continue; // Мёртвых не рисуем

            Vector2 screenPos = WorldToScreen(p.position);
            Vector2 headPos = WorldToScreen(p.head);

            // Если игрок на экране
            if (screenPos.x > 0 && screenPos.x < screenW && screenPos.y > 0 && screenPos.y < screenH) {
                // Бокс (простейший)
                if (espBox) {
                    float boxHeight = abs(screenPos.y - headPos.y);
                    float boxWidth = boxHeight * 0.6f;
                    drawList->AddRect(
                        ImVec2(screenPos.x - boxWidth / 2, headPos.y),
                        ImVec2(screenPos.x + boxWidth / 2, screenPos.y),
                        IM_COL32(255, 255, 255, 255)
                    );
                }

                // Линия от центра к игроку
                if (espLines) {
                    drawList->AddLine(ImVec2(screenW / 2, screenH), ImVec2(screenPos.x, screenPos.y), IM_COL32(255, 255, 255, 200), 1.0f);
                }

                // Здоровье
                if (espHealth) {
                    char hpBuf[32];
                    sprintf_s(hpBuf, "HP: %d", p.health);
                    drawList->AddText(ImVec2(screenPos.x, headPos.y - 20), IM_COL32(0, 255, 0, 255), hpBuf);
                }

                // Имя
                if (espName) {
                    drawList->AddText(ImVec2(screenPos.x, headPos.y - 35), IM_COL32(255, 255, 0, 255), p.name.c_str());
                }

                // Дистанция
                if (espDistance) {
                    // Упрощенно, просто выводим координаты
                    char distBuf[64];
                    sprintf_s(distBuf, "%.1f, %.1f, %.1f", p.position.x, p.position.y, p.position.z);
                    drawList->AddText(ImVec2(screenPos.x, screenPos.y + 5), IM_COL32(200, 200, 200, 255), distBuf);
                }
            }
        }
    }

    // --- МЕНЮ ---
    if (showMenu) {
        ImGui::Begin("Cheat Menu", &showMenu, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Press '=' to toggle menu");
        
        if (ImGui::CollapsingHeader("ESP Settings")) {
            ImGui::Checkbox("Enable ESP", &espEnabled);
            ImGui::Checkbox("Draw Box", &espBox);
            ImGui::Checkbox("Draw Lines", &espLines);
            ImGui::Checkbox("Draw Health", &espHealth);
            ImGui::Checkbox("Draw Distance", &espDistance);
            ImGui::Checkbox("Draw Name", &espName);
        }
        
        if (ImGui::CollapsingHeader("Aimbot Settings")) {
            ImGui::Checkbox("Enable Aimbot", &aimbotEnabled);
            ImGui::SliderFloat("FOV", &aimbotFov, 1.0f, 360.0f, "%.0f");
            ImGui::Text("Hold V to aim");
        }
        
        ImGui::Text("Players found: %d", (int)players.size());
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    return oPresent(pSwapChain, SyncInterval, Flags);
}

void StartOverlay() {
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "DX11Test", NULL };
    RegisterClassEx(&wc);
    HWND hWnd = CreateWindow("DX11Test", "DX11Test", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, wc.hInstance, NULL);

    DXGI_SWAP_CHAIN_DESC sd = { 0 };
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

    void** pVTable = *(void***)d3dSwapChain;
    void* pPresent = pVTable[8];

    DWORD oldProtect;
    VirtualProtect(&pVTable[8], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
    oPresent = (Present_t)pPresent;
    pVTable[8] = &hkPresent;
    VirtualProtect(&pVTable[8], sizeof(void*), oldProtect, &oldProtect);

    d3dSwapChain->Release();
    d3dDevice->Release();
    d3dContext->Release();
    DestroyWindow(hWnd);
    UnregisterClass("DX11Test", wc.hInstance);
}
