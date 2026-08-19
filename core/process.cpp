#include "process.h"
#include "globals.h"
#include "memory.h"
#include <TlHelp32.h>
#include <string>
#include <iostream>
#include <iostream>

ProcessDiscovery g_process;

// ── Find the game process by name ────────────────────────────────────────────
bool ProcessDiscovery::FindGameProcess() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32 pe{};
    pe.dwSize = sizeof(pe);

    std::cout << "[NOX] Searching for game process..." << std::endl;
    
    bool found = false;
    if (Process32First(snap, &pe)) {
        do {
            // Debug: FiveM, GTA veya Process kelimelerini içeren tüm processleri listele
            std::string name = pe.szExeFile;
            if (name.find("FiveM") != std::string::npos ||
                name.find("GTA") != std::string::npos ||
                name.find("Process") != std::string::npos) {
                std::cout << "[NOX]   Found: " << pe.szExeFile << " (PID: " << pe.th32ProcessID << ")" << std::endl;
            }
            
            if (_stricmp(pe.szExeFile, "GTAProcess.exe") == 0 ||
                _stricmp(pe.szExeFile, "FiveM.exe") == 0 ||
                _stricmp(pe.szExeFile, "GTA5.exe") == 0) {
                m_pid = pe.th32ProcessID;
                found = true;
                std::cout << "[NOX]   >> Selected: " << pe.szExeFile << std::endl;
                break;
            }
        } while (Process32Next(snap, &pe));
    }

    if (!found) {
        std::cerr << "[NOX] No game process found!" << std::endl;
    }

    CloseHandle(snap);
    return found;
}

// ── Open the game process with full access ───────────────────────────────────
//
//  The original code sleeps for 5 seconds before opening, presumably to
//  wait for the process to fully initialise after detection.
//
bool ProcessDiscovery::OpenGameProcess() {
    for (int i = 0; i < 10; ++i) {
        m_process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_pid);
        if (m_process_handle != NULL)
            return true;
        Sleep(500);
    }
    return false;
}

// ── Find the game window ─────────────────────────────────────────────────────
bool ProcessDiscovery::FindGameWindow() {
    for (int i = 0; i < 10; ++i) {
        m_game_window = FindWindowA("grcWindow", NULL);
        if (m_game_window)
            return true;
        Sleep(1000);
    }
    return false;
}

// ── Find citizen-playernames-five.dll ────────────────────────────────────────
bool ProcessDiscovery::FindCitizenModule() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_pid);
    if (snap == INVALID_HANDLE_VALUE) {
        std::cerr << "[NOX] Failed to create module snapshot" << std::endl;
        return false;
    }

    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);

    std::cout << "[NOX] Searching for citizen modules..." << std::endl;
    
    bool found = false;
    if (Module32First(snap, &me)) {
        do {
            std::string name = me.szModule;
            // Debug: citizen içeren tüm DLL'leri listele
            if (name.find("citizen") != std::string::npos || 
                name.find("FiveM") != std::string::npos) {
                std::cout << "[NOX]   Found: " << me.szModule << std::endl;
            }
            
            if (_stricmp(me.szModule, "citizen-playernames-five.dll") == 0 ||
                _stricmp(me.szModule, "FiveM_GTAProcess.exe") == 0 ||
                _stricmp(me.szModule, "FiveM.exe") == 0) {
                m_citizen_base = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                m_citizen_size = me.modBaseSize;
                found = true;
                std::cout << "[NOX]   >> Selected: " << me.szModule << std::endl;
                break;
            }
        } while (Module32Next(snap, &me));
    }

    if (!found) {
        std::cerr << "[NOX] No citizen module found! Listing all modules:" << std::endl;
        // Tüm modülleri listele
        MODULEENTRY32 me2{};
        me2.dwSize = sizeof(me2);
        if (Module32First(snap, &me2)) {
            int count = 0;
            do {
                std::cout << "[NOX]     - " << me2.szModule << std::endl;
                if (++count > 20) {
                    std::cout << "[NOX]     ... (truncated)" << std::endl;
                    break;
                }
            } while (Module32Next(snap, &me2));
        }
    }

    CloseHandle(snap);
    return found;
}

// ── Full discovery sequence ──────────────────────────────────────────────────
bool ProcessDiscovery::Discover() {
    std::cout << "[NOX] Starting process discovery..." << std::endl;
    
    if (!FindGameProcess()) {
        g.status_msg  = "Failed to find GTAProcess.exe";
        g.status_type = 2;
        std::cerr << "[NOX] " << g.status_msg << std::endl;
        return false;
    }
    std::cout << "[NOX] Game process found (PID: " << m_pid << ")" << std::endl;

    if (!OpenGameProcess()) {
        g.status_msg  = "Failed to open game process";
        g.status_type = 2;
        std::cerr << "[NOX] " << g.status_msg << std::endl;
        return false;
    }
    std::cout << "[NOX] Game process opened" << std::endl;

    if (!FindGameWindow()) {
        g.status_msg  = "Failed to find game window (grcWindow)";
        g.status_type = 2;
        std::cerr << "[NOX] " << g.status_msg << std::endl;
        return false;
    }
    std::cout << "[NOX] Game window found" << std::endl;

    if (!FindCitizenModule()) {
        g.status_msg  = "Failed to find citizen-playernames-five.dll";
        g.status_type = 2;
        std::cerr << "[NOX] " << g.status_msg << std::endl;
        return false;
    }
    std::cout << "[NOX] Citizen module found" << std::endl;

    // ── populate globals ─────────────────────────────────────────────────────
    g.game_process        = m_process_handle;
    g.game_pid            = m_pid;
    g.game_window         = m_game_window;
    g.citizen_module_base = m_citizen_base;
    g.citizen_module_size = m_citizen_size;

    std::cout << "[NOX] Finding GTA5 main module..." << std::endl;
    
    // GTA5 ana modülü — tüm offset'ler bunun üzerinden hesaplanır
    g.game_module_base = Memory::GetModuleBase(m_pid, "GTA5.exe");
    g.game_module_size = Memory::GetModuleSize(m_pid, "GTA5.exe");

    // FiveM, GTA5.exe yerine GTAProcess.exe olarak çalıştırabilir;
    // her ikisini de dene
    if (g.game_module_base == 0) {
        std::cout << "[NOX] GTA5.exe not found, trying GTAProcess.exe..." << std::endl;
        g.game_module_base = Memory::GetModuleBase(m_pid, "GTAProcess.exe");
        g.game_module_size = Memory::GetModuleSize(m_pid, "GTAProcess.exe");
    }
    
    // FiveM.exe'yi de dene
    if (g.game_module_base == 0) {
        std::cout << "[NOX] GTAProcess.exe not found, trying FiveM.exe..." << std::endl;
        g.game_module_base = Memory::GetModuleBase(m_pid, "FiveM.exe");
        g.game_module_size = Memory::GetModuleSize(m_pid, "FiveM.exe");
    }

    if (g.game_module_base == 0) {
        g.status_msg  = "Failed to find GTA5 main module (GTA5.exe / GTAProcess.exe / FiveM.exe)";
        g.status_type = 2;
        std::cerr << "[NOX] " << g.status_msg << std::endl;
        return false;
    }

    std::cout << "[NOX] Main module found at 0x" << std::hex << g.game_module_base 
              << " (size: 0x" << g.game_module_size << ")" << std::dec << std::endl;

    g.status_msg  = "Process discovery complete";
    g.status_type = 1;
    return true;
}
