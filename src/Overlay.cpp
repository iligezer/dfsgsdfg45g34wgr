#include "Overlay.h"
#include "GameReader.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <vector>

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static bool g_MenuVisible = true;

// WorldToScreen (упрощённо, нужна матрица камеры)
// Для реального ESP нужно получить view matrix через Camera.get_worldToCameraMatrix
// Здесь заглушка для демонстрации
static bool WorldToScreen(float x, float y, float z, float& sx, float& sy) {
    // TODO: использовать реальную матрицу камеры
    // Пока просто заглушка
    sx = 0; sy = 0;
    return false;
}

bool InitImGui(IDXGISwapChain* swapChain, ID3D11Device* device, ID3D11DeviceContext* context) {
    g_pd3dDevice = device;
    g_pd3dDeviceContext = context;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Красивая тёмная тема с акцентом
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.95f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.35f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.35f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2f, 0.8f, 0.4f, 1.0f);

    // Получаем HWND из swapChain
    DXGI_SWAP_CHAIN_DESC desc;
    swapChain->GetDesc(&desc);
    HWND hwnd = desc.OutputWindow;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    return true;
}

void RenderImGui() {
    if (!g_MenuVisible) return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // ---- МЕНЮ ----
    ImGui::Begin("Mono Cheat", &g_MenuVisible, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("Players: %d", (int)GetPlayers().size());
    ImGui::Separator();

    auto players = GetPlayers();
    for (auto& p : players) {
        if (!p.valid) continue;
        ImGui::Text("%s | HP: %d | Team: %d", p.name.c_str(), p.health, p.team);
        if (p.isDead) ImGui::SameLine(), ImGui::TextColored(ImVec4(1,0,0,1), "DEAD");
        ImGui::Text("  Pos: (%.1f, %.1f, %.1f)", p.x, p.y, p.z);
    }

    ImGui::End();

    // ---- ESP (рисование поверх игры) ----
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    for (auto& p : players) {
        if (!p.valid || p.isDead) continue;
        float sx, sy;
        if (WorldToScreen(p.x, p.y + 1.8f, p.z, sx, sy)) {
            char buf[128];
            sprintf_s(buf, "%s [%d]", p.name.c_str(), p.health);
            draw->AddText(ImVec2(sx, sy), IM_COL32(0, 255, 0, 255), buf);
        }
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ShutdownImGui() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void SetMenuVisible(bool v) { g_MenuVisible = v; }
bool IsMenuVisible() { return g_MenuVisible; }
