#pragma once
#include <d3d11.h>

bool InitializeImGui(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd);
void ShutdownImGui();
