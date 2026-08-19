#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <atomic>

struct Globals {
    // ── Process state ────────────────────────────────────────────────────────
    HANDLE game_process   = NULL;
    DWORD  game_pid       = 0;
    HWND   game_window    = NULL;

    uintptr_t game_module_base  = 0;
    size_t    game_module_size  = 0;

    // ── Citizen-FiveM module ──────────────────────────────────────────────────
    uintptr_t citizen_module_base = 0;
    size_t    citizen_module_size = 0;

    // ── Resolved offsets (set by pattern scanner) ─────────────────────────────
    uintptr_t offset_entity_list = 0;   // DAT_140246400  (AOB pattern 1)
    uintptr_t offset_player_info = 0;   // DAT_140246408  (AOB pattern 2)

    // ── Hardcoded offsets (version-dependent, set during version matching) ─────
    uintptr_t offset_world     = 0;     // DAT_140246370
    uintptr_t offset_replay    = 0;     // DAT_140246378
    uintptr_t offset_local_ped = 0;     // DAT_140246380
    uintptr_t offset_camera    = 0;     // DAT_140246390
    int  local_ped_sub_offset  = 0;     // AOB/brute-force: world_ptr içindeki sub-offset
    bool local_ped_is_sub      = false; // true ise sub-offset mode

    // ── Sub-offsets (common across versions) ──────────────────────────────────
    int entity_list_offset      = 0x10a8;   // DAT_1402463a0
    int player_info_offset      = 0xe8;     // DAT_1402463a8
    int team_offset             = 0x280;    // DAT_1402463b0
    int ped_health_offset       = 0x10b8;   // DAT_1402463d0
    int ped_armor_offset        = 0x410;    // DAT_1402463d8
    int ped_velocity_offset     = 0xd10;    // DAT_1402463f0
    int weapon_manager_offset   = 0x145c;   // DAT_1402463f8

    // ── Overlay state ─────────────────────────────────────────────────────────
    std::atomic<bool> overlay_running{false};
    std::atomic<bool> menu_open{true};
    bool esp_enabled     = false;

    // ── Session / hesap bilgisi (ileriye dönük — auth sistemi gelince doldurulur) ──
    struct SessionInfo {
        char  username[64]  = "Guest";      // auth'dan gelecek
        char  hwid_tag[16]  = "FREE";       // lisans tipi: FREE / PRO / LIFETIME
        char  version[16]   = "v2.0";
        bool  logged_in     = false;        // auth başarılıysa true
        bool  show_hud      = true;         // HUD göster/gizle
    } session;

    // ── Status messages ───────────────────────────────────────────────────────
    std::string status_msg;
    int         status_type = 0;   // 1=success, 2=error, 5=info
};

extern Globals g;
