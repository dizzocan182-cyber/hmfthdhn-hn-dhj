#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <atomic>

class Overlay {
public:
    bool Initialize();
    void Shutdown();
    void Run();

    ID3D11Device* GetDevice() const { return m_device; }
    ID3D11DeviceContext* GetContext() const { return m_context; }
    ID3D11RenderTargetView* GetRenderTarget() const { return m_render_target; }
    HWND GetWindow() const { return m_hwnd; }
    bool IsRunning() const { return m_running; }

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    bool CreateOverlayWindow();
    bool CreateDevice();
    bool CreateRenderTarget();
    void CleanupRenderTarget();

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd = NULL;
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swap_chain = nullptr;
    ID3D11RenderTargetView* m_render_target = nullptr;

    int m_width = 0;
    int m_height = 0;
    std::atomic<bool> m_running{ false };
};

extern Overlay g_overlay;
