#include "memory.h"
#include <TlHelp32.h>
#include <sstream>
#include <algorithm>

Memory g_memory;

// ── Buffer read ──────────────────────────────────────────────────────────────
bool Memory::ReadBuffer(HANDLE proc, uintptr_t addr, void* buffer, size_t size) {
    return ReadProcessMemory(proc, reinterpret_cast<LPCVOID>(addr), buffer, size, nullptr) != 0;
}

// ── String read ──────────────────────────────────────────────────────────────
std::string Memory::ReadString(HANDLE proc, uintptr_t addr, size_t max_len) {
    std::string result;
    result.reserve(max_len);

    for (size_t i = 0; i < max_len; ++i) {
        char ch = Read<char>(proc, addr + i);
        if (ch == '\0') break;
        result.push_back(ch);
    }
    return result;
}

// ── Pattern scanner (local buffer) ───────────────────────────────────────────
//
//  Tokenises the pattern string on spaces.
//  Tokens of "?" represent wildcard bytes and are stored as 0x00 with a
//  parallel boolean mask so they are never compared literally.
//
uintptr_t Memory::PatternScan(const uint8_t* buffer, size_t buffer_size, const std::string& pattern) {
    // ── tokenise ──
    std::vector<uint8_t>  bytes;
    std::vector<bool>     mask;       // true = must match

    std::istringstream ss(pattern);
    std::string token;
    while (ss >> token) {
        if (token == "?" || token == "??") {
            bytes.push_back(0x00);
            mask.push_back(false);
        } else {
            bytes.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 16)));
            mask.push_back(true);
        }
    }

    if (bytes.empty() || buffer_size < bytes.size())
        return (uintptr_t)-1;

    // ── linear scan ──
    for (size_t i = 0; i <= buffer_size - bytes.size(); ++i) {
        bool found = true;
        for (size_t j = 0; j < bytes.size(); ++j) {
            if (mask[j] && buffer[i + j] != bytes[j]) {
                found = false;
                break;
            }
        }
        if (found)
            return i;   // offset within buffer
    }
    return (uintptr_t)-1;   // not found — 0 is a valid offset, use sentinel
}

// ── Pattern scanner (remote module) ──────────────────────────────────────────
uintptr_t Memory::PatternScanModule(HANDLE proc, uintptr_t module_base, size_t module_size, const std::string& pattern) {
    std::vector<uint8_t> buf(module_size);
    if (!ReadBuffer(proc, module_base, buf.data(), module_size))
        return 0;

    uintptr_t offset = PatternScan(buf.data(), module_size, pattern);
    if (offset == (uintptr_t)-1)
        return 0;   // not found — callers check for 0

    return module_base + offset;
}

// ── Module base / size via Toolhelp32 ────────────────────────────────────────
uintptr_t Memory::GetModuleBase(DWORD pid, const char* module_name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);

    uintptr_t result = 0;
    if (Module32First(snap, &me)) {
        do {
            if (_stricmp(me.szModule, module_name) == 0) {
                result = reinterpret_cast<uintptr_t>(me.modBaseAddr);
                break;
            }
        } while (Module32Next(snap, &me));
    }

    CloseHandle(snap);
    return result;
}

size_t Memory::GetModuleSize(DWORD pid, const char* module_name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;

    MODULEENTRY32 me{};
    me.dwSize = sizeof(me);

    size_t result = 0;
    if (Module32First(snap, &me)) {
        do {
            if (_stricmp(me.szModule, module_name) == 0) {
                result = me.modBaseSize;
                break;
            }
        } while (Module32Next(snap, &me));
    }

    CloseHandle(snap);
    return result;
}

// ── Resolve x64 RIP-relative address ─────────────────────────────────────────
//
//  Reads the 4-byte signed displacement at `instruction_addr + operand_offset`,
//  then adds the displacement + instruction_size to the instruction address
//  to produce the absolute target.
//
uintptr_t Memory::ResolveRipRelative(HANDLE proc, uintptr_t instruction_addr,
                                      int operand_offset, int instruction_size) {
    int32_t displacement = g_memory.Read<int32_t>(proc, instruction_addr + operand_offset);
    return instruction_addr + instruction_size + displacement;
}
