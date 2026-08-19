#pragma once
#include <Windows.h>
#include <string>

class ProcessDiscovery {
public:
    // Find GTAProcess.exe by enumerating running processes
    bool FindGameProcess();

    // Open game process with PROCESS_ALL_ACCESS
    bool OpenGameProcess();

    // Find game window (class name "grcWindow")
    bool FindGameWindow();

    // Find citizen-playernames-five.dll module
    bool FindCitizenModule();

    // Run the full discovery sequence
    bool Discover();

    // ── Getters ──────────────────────────────────────────────────────────────
    HANDLE   GetProcessHandle() const { return m_process_handle; }
    DWORD    GetProcessId()     const { return m_pid; }
    HWND     GetGameWindow()    const { return m_game_window; }
    uintptr_t GetCitizenBase()  const { return m_citizen_base; }
    size_t   GetCitizenSize()   const { return m_citizen_size; }

private:
    HANDLE   m_process_handle = NULL;
    DWORD    m_pid            = 0;
    HWND     m_game_window    = NULL;
    uintptr_t m_citizen_base  = 0;
    size_t   m_citizen_size   = 0;
};

extern ProcessDiscovery g_process;
