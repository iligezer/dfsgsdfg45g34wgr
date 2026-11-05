#include "Hook.h"
#include "Overlay.h"
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>

// MinHook
#include "MinHook.h"

// ============================================================
// УКАЗАТЕЛИ НА ОРИГИНАЛЬНЫЕ ФУНКЦИИ
// ============================================================
typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffers_t)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);

static Present_t       oPresent       = nullptr;
static ResizeBuffers_t oResizeBuffers = nullptr;

static ID3D11Device*        g_pd3dDevice        = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*      g_pSwapChain        = nullptr;
static bool                 g_ImGuiInitialized  = false;

// ============================================================
// ХУК PRESENT
// ============================================================
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    // Инициализация ImGui при первом вызове
    if (!g_ImGuiInitialized)
    {
        g_pSwapChain = pSwapChain;

        // Получаем устройство и контекст
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice)))
        {
            g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);

            // Инициализируем ImGui
            if (InitImGui(pSwapChain, g_pd3dDevice, g_pd3dDeviceContext))
            {
                g_ImGuiInitialized = true;
            }
        }
    }

    // Рендерим меню и ESP
    if (g_ImGuiInitialized)
    {
        RenderImGui();
    }

    // Вызываем оригинальный Present
    return oPresent(pSwapChain, SyncInterval, Flags);
}

// ============================================================
// ХУК RESIZEBUFFERS (чтобы ImGui не ломался при ресайзе окна)
// ============================================================
HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    // Сбрасываем ImGui-инициализацию, чтобы он пересоздал рендер-таргеты
    if (g_ImGuiInitialized)
    {
        ShutdownImGui();
        g_ImGuiInitialized = false;
    }
    return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

// ============================================================
// ПОЛУЧЕНИЕ АДРЕСА PRESENT ЧЕРЕЗ ВРЕМЕННЫЙ SWAPCHAIN
// ============================================================
static void* GetPresentAddress()
{
    // Создаём временное окно
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DefWindowProc, 0L, 0L,
                      GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
                      L"DX_TEMP", nullptr };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
                             0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    // Создаём временный swapchain
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 100;
    sd.BufferDesc.Height = 100;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* swapChain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, &swapChain, &device, &featureLevel, &context);

    if (FAILED(hr) || !swapChain)
    {
        DestroyWindow(hwnd);
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return nullptr;
    }

    // Адрес Present — это 8-й метод в vtable IDXGISwapChain
    void* presentAddr = (*(void***)swapChain)[8];

    // Чистим за собой
    swapChain->Release();
    device->Release();
    context->Release();
    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return presentAddr;
}

// ============================================================
// ИНИЦИАЛИЗАЦИЯ ХУКА
// ============================================================
bool InitHook()
{
    // Инициализация MinHook
    if (MH_Initialize() != MH_OK)
    {
        return false;
    }

    // Получаем адрес Present
    void* presentAddr = GetPresentAddress();
    if (!presentAddr)
    {
        MH_Uninitialize();
        return false;
    }

    // Ставим хук на Present
    if (MH_CreateHook(presentAddr, &hkPresent, (void**)&oPresent) != MH_OK)
    {
        MH_Uninitialize();
        return false;
    }

    // Включаем хук
    if (MH_EnableHook(presentAddr) != MH_OK)
    {
        MH_Uninitialize();
        return false;
    }

    // Дополнительно ставим хук на ResizeBuffers (если нужно)
    // (для простоты можно пропустить, но с ним стабильнее)
    // void* resizeAddr = (*(void***)g_pSwapChain)[13]; // ResizeBuffers
    // MH_CreateHook(resizeAddr, &hkResizeBuffers, (void**)&oResizeBuffers);
    // MH_EnableHook(resizeAddr);

    return true;
}

// ============================================================
// СНЯТИЕ ХУКА
// ============================================================
void ShutdownHook()
{
    // Отключаем все хуки
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    // Освобождаем ImGui
    if (g_ImGuiInitialized)
    {
        ShutdownImGui();
        g_ImGuiInitialized = false;
    }

    // Освобождаем D3D11-ресурсы
    if (g_pd3dDeviceContext)
    {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }
    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
    g_pSwapChain = nullptr;
}
