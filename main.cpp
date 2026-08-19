#include <Windows.h>
#include <thread>
#include <chrono>
#include <iostream>
#include "core/globals.h"
#include "core/memory.h"
#include "core/process.h"
#include "game/game_memory.h"
#include "features/aimbot.h"
#include "features/esp.h"
#include "features/player_features.h"
#include "features/vehicle_features.h"
#include "features/teleport.h"
#include "overlay/overlay.h"
#include "overlay/dx11.h"
#include "ui/menu.h"
#include "config/config.h"

Aimbot g_aimbot;
Esp g_esp;
PlayerFeatures g_player_features;
VehicleFeatures g_vehicle_features;
Teleport g_teleport;

void CheatLoop() {
    std::cout << "[NOX] CheatLoop started" << std::endl;
    
    g.status_msg = "Searching for the game process...";
    g.status_type = 5;
    std::cout << "[NOX] " << g.status_msg << std::endl;

    if (!g_process.Discover()) {
        g.status_msg = "Failed to find game process!";
        g.status_type = 2;
        std::cerr << "[NOX] ERROR: " << g.status_msg << std::endl;
        Sleep(3000);
        return;
    }
    g.status_msg = "Game process found!";
    g.status_type = 1;
    std::cout << "[NOX] " << g.status_msg << std::endl;

    g.status_msg = "Initializing game memory...";
    std::cout << "[NOX] " << g.status_msg << std::endl;
    if (!g_game.Initialize()) {
        g.status_msg = "Failed to initialize game memory!";
        g.status_type = 2;
        std::cerr << "[NOX] ERROR: " << g.status_msg << std::endl;
        return;
    }
    g.status_msg = "Game memory initialized!";
    g.status_type = 1;
    std::cout << "[NOX] " << g.status_msg << std::endl;

    g.status_msg = "Creating overlay...";
    g.status_type = 5;
    std::cout << "[NOX] " << g.status_msg << std::endl;
    if (!g_overlay.Initialize()) {
        g.status_msg = "Failed to create overlay!";
        g.status_type = 2;
        std::cerr << "[NOX] ERROR: " << g.status_msg << std::endl;
        return;
    }
    g.status_msg = "Overlay created successfully!";
    g.status_type = 1;
    std::cout << "[NOX] " << g.status_msg << std::endl;

    if (!InitializeImGui(g_overlay.GetDevice(), g_overlay.GetContext(), g_overlay.GetWindow())) {
        g.status_msg = "Failed to initialize ImGui!";
        g.status_type = 2;
        std::cerr << "[NOX] ERROR: " << g.status_msg << std::endl;
        return;
    }
    std::cout << "[NOX] ImGui initialized!" << std::endl;

    g_config.aimbot = &g_aimbot.GetConfig();
    g_config.esp = &g_esp.GetConfig();
    g_config.player = &g_player_features.GetConfig();
    g_config.vehicle = &g_vehicle_features.GetConfig();
    g_config.Load();

    g_menu = new Menu(g_aimbot.GetConfig(), g_esp.GetConfig(), g_player_features.GetConfig(), g_vehicle_features.GetConfig(), g_teleport);

    g_teleport.Initialize();

    FreeConsole();

    g.overlay_running = true;

    std::thread([]() {
        while (g.overlay_running) {
            static bool insert_was_down = false;
            bool insert_down = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
            if (insert_down && !insert_was_down) {
                g.menu_open = !g.menu_open;
                LONG style = GetWindowLong(g_overlay.GetWindow(), GWL_EXSTYLE);
                if (g.menu_open) {
                    style &= ~WS_EX_TRANSPARENT;
                } else {
                    style |= WS_EX_TRANSPARENT;
                }
                SetWindowLong(g_overlay.GetWindow(), GWL_EXSTYLE, style);
            }
            insert_was_down = insert_down;

            auto local_player = g_game.ReadLocalPlayer();
            auto view_matrix = g_game.ReadViewMatrix();

            std::vector<Ped> player_list;
            int max = g_game.GetMaxPlayers();
            if (max <= 0 || max > 256) max = 32;
            for (int i = 0; i < max; ++i) {
                Ped p = g_game.ReadPlayer(i);
                if (p.is_valid) player_list.push_back(p);
            }

            if (g_aimbot.GetConfig().enabled) {
                g_aimbot.Update(player_list, view_matrix,
                    static_cast<float>(g_overlay.GetWidth()),
                    static_cast<float>(g_overlay.GetHeight()));
            }
            if (g_esp.GetConfig().enabled) {
                g_esp.Update(player_list, view_matrix,
                    static_cast<float>(g_overlay.GetWidth()),
                    static_cast<float>(g_overlay.GetHeight()));
            }
            g_player_features.Update();
            g_vehicle_features.Update();

            Sleep(16);
        }
    }).detach();

    g_overlay.Run();

    g.overlay_running = false;
    delete g_menu;
    g_menu = nullptr;
    ShutdownImGui();
    g_config.Save();
    g_overlay.Shutdown();
    if (g.game_process) {
        CloseHandle(g.game_process);
        g.game_process = NULL;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // ── DPI awareness — PER_MONITOR_V2 ──
    HMODULE u32 = GetModuleHandleW(L"user32.dll");
    if (u32) {
        typedef BOOL(WINAPI* pSDAC)(DPI_AWARENESS_CONTEXT);
        auto fn = (pSDAC)GetProcAddress(u32, "SetProcessDpiAwarenessContext");
        if (fn) fn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        else SetProcessDPIAware();   // win8 fallback
    }
    AllocConsole();
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();
    
    std::cout << "[NOX] Starting cheat..." << std::endl;

    if (IsDebuggerPresent()) {
        std::cerr << "[NOX] Debugger detected!" << std::endl;
        MessageBoxA(NULL, "Debugger detected!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    CheatLoop();
    
    std::cout << "[NOX] Exiting..." << std::endl;
    Sleep(2000);
    return 0;
}
