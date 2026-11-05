#pragma once
#include <d3d11.h>
#include <dxgi.h>

bool InitImGui(IDXGISwapChain* swapChain, ID3D11Device* device, ID3D11DeviceContext* context);
void RenderImGui();
void ShutdownImGui();
void SetMenuVisible(bool visible);
bool IsMenuVisible();
