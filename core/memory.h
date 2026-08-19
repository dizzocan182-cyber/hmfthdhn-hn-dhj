#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <cstdint>

class Memory {
public:
    // ── Typed read / write ───────────────────────────────────────────────────
    template<typename T>
    T Read(HANDLE proc, uintptr_t addr) {
        T val{};
        ReadProcessMemory(proc, reinterpret_cast<LPCVOID>(addr), &val, sizeof(T), nullptr);
        return val;
    }

    template<typename T>
    bool Write(HANDLE proc, uintptr_t addr, const T& val) {
        return WriteProcessMemory(proc, reinterpret_cast<LPVOID>(addr), &val, sizeof(T), nullptr) != 0;
    }

    // ── Buffer read ──────────────────────────────────────────────────────────
    bool ReadBuffer(HANDLE proc, uintptr_t addr, void* buffer, size_t size);

    // ── String read (null-terminated, remote process) ─────────────────────────
    std::string ReadString(HANDLE proc, uintptr_t addr, size_t max_len = 256);

    // ── Pattern / AOB scanner ────────────────────────────────────────────────
    //  pattern format: "48 8b ? ? ? 4c 8b"  where '?' is a wildcard byte
    //  Returns byte offset within buffer, or (uintptr_t)-1 if not found.
    //  NOTE: 0 is a valid offset (match at start of buffer), do NOT use 0 as sentinel.
    uintptr_t PatternScan(const uint8_t* buffer, size_t buffer_size, const std::string& pattern);

    //  Dump remote module into local buffer, then scan it.
    //  Returns absolute address (module_base + offset), or 0 if not found.
    uintptr_t PatternScanModule(HANDLE proc, uintptr_t module_base, size_t module_size, const std::string& pattern);

    // ── Module enumeration helpers ────────────────────────────────────────────
    static uintptr_t GetModuleBase(DWORD pid, const char* module_name);
    static size_t    GetModuleSize(DWORD pid, const char* module_name);

    // ── x64 RIP-relative address resolver ────────────────────────────────────
    static uintptr_t ResolveRipRelative(HANDLE proc, uintptr_t instruction_addr,
                                         int operand_offset, int instruction_size);
};

extern Memory g_memory;
