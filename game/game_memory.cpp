#include "game_memory.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <iostream>

GameMemory g_game;

// ── Helpers ──────────────────────────────────────────────────────────────────
static bool IsValidPointer(uintptr_t addr, uintptr_t module_base, size_t module_size) {
    if (addr == 0) return false;
    if (addr >= module_base && addr < module_base + module_size) return false;
    return true;
}

static bool ReadChain(HANDLE proc, uintptr_t base, const std::vector<int>& offsets, uintptr_t& out) {
    uintptr_t cur = base;
    for (int off : offsets) {
        if (cur == 0) return false;
        cur = g_memory.Read<uintptr_t>(proc, cur + off);
    }
    out = cur;
    return cur != 0;
}

// ── Initialize ───────────────────────────────────────────────────────────────
bool GameMemory::Initialize() {
    std::cout << "[NOX] GameMemory::Initialize()" << std::endl;
    
    if (g.game_process == NULL) {
        std::cerr << "[NOX] ERROR: game_process is NULL" << std::endl;
        return false;
    }
    std::cout << "[NOX] Game process handle: " << g.game_process << std::endl;
    
    if (!DetectVersion()) {
        std::cerr << "[NOX] ERROR: DetectVersion() failed" << std::endl;
        return false;
    }
    std::cout << "[NOX] Version detected (index: " << m_version_index << ")" << std::endl;
    
    if (!ResolveOffsets()) {
        std::cerr << "[NOX] ERROR: ResolveOffsets() failed" << std::endl;
        return false;
    }
    std::cout << "[NOX] Offsets resolved" << std::endl;
    
    m_initialized = true;
    return true;
}

// ── Version detection ────────────────────────────────────────────────────────
bool GameMemory::DetectVersion() {
    HANDLE proc = g.game_process;
    auto mod = g.game_module_base;
    auto sz  = g.game_module_size;

    // ── Step 1: try hardcoded versions ──
    std::cout << "[NOX] Trying hardcoded version offsets (" << std::size(Offsets::kVersions) << " versions)..." << std::endl;
    for (int i = 0; i < static_cast<int>(std::size(Offsets::kVersions)); ++i) {
        const auto& v = Offsets::kVersions[i];
        uintptr_t world_ptr = g_memory.Read<uintptr_t>(proc, mod + v.world);
        if (world_ptr != 0 && IsValidPointer(world_ptr, mod, sz)) {
            m_version_index = i;
            g.offset_world     = v.world;
            g.offset_replay    = v.replay;
            g.offset_local_ped = v.local_ped;
            g.offset_camera    = v.camera;
            std::cout << "[NOX] Hardcoded version matched: index " << i
                      << " world=0x" << std::hex << v.world << std::dec << std::endl;
            return true;
        }
    }
    std::cout << "[NOX] No hardcoded version matched, trying AOB patterns..." << std::endl;

    // ── Step 2: AOB scan for world pointer ──
    const char* world_patterns[] = {
        Offsets::Patterns::kWorldPattern_v1,
        Offsets::Patterns::kWorldPattern_v2,
        Offsets::Patterns::kWorldPattern_v3,
        Offsets::Patterns::kWorldPattern_v4,
    };

    uintptr_t world_rip = 0;
    for (auto pat : world_patterns) {
        world_rip = g_memory.PatternScanModule(proc, mod, sz, pat);
        if (world_rip) {
            std::cout << "[NOX] World pattern found at RIP 0x" << std::hex << world_rip << std::dec << std::endl;
            break;
        }
    }

    if (world_rip) {
        // RIP-relative: disp32 at offset 3, instruction size 7 (48 8b 0d XXXXXXXX)
        uintptr_t world_ptr_addr = g_memory.ResolveRipRelative(proc, world_rip, 3, 7);
        uintptr_t world_ptr = g_memory.Read<uintptr_t>(proc, world_ptr_addr);
        if (world_ptr != 0 && IsValidPointer(world_ptr, mod, sz)) {
            g.offset_world = world_ptr_addr - mod;
            std::cout << "[NOX] World offset (AOB): 0x" << std::hex << g.offset_world << std::dec << std::endl;

            // scan nearby for replay (usually within 0x80000 of world)
            g.offset_replay = 0;
            g.offset_local_ped = 0;
            g.offset_camera = 0;

            const char* cam_patterns[] = {
                Offsets::Patterns::kCameraPattern_v1,
                Offsets::Patterns::kCameraPattern_v2,
            };
            for (auto pat : cam_patterns) {
                uintptr_t cam_rip = g_memory.PatternScanModule(proc, mod, sz, pat);
                if (cam_rip) {
                    uintptr_t cam_addr = g_memory.ResolveRipRelative(proc, cam_rip, 3, 7);
                    g.offset_camera = cam_addr - mod;
                    std::cout << "[NOX] Camera offset (AOB): 0x" << std::hex << g.offset_camera << std::dec << std::endl;
                    break;
                }
            }

            // brute-force replay: scan module for a pointer that looks like replay
            for (uintptr_t off = 0; off < sz; off += 8) {
                uintptr_t ptr = g_memory.Read<uintptr_t>(proc, mod + off);
                if (ptr == 0 || !IsValidPointer(ptr, mod, sz) || ptr == world_ptr) continue;
                int32_t max_peds = g_memory.Read<int32_t>(proc, ptr + 0x18);
                if (max_peds < 1 || max_peds > 256) continue;
                // extra validation: check if there's a valid entity list pointer nearby
                uintptr_t ent_test = g_memory.Read<uintptr_t>(proc, ptr + 0x10);
                if (ent_test == 0 || !IsValidPointer(ent_test, mod, sz)) continue;
                g.offset_replay = off;
                std::cout << "[NOX] Replay offset (AOB brute-force): 0x" << std::hex << off
                          << " max_peds=" << std::dec << max_peds << std::endl;
                break;
            }

            // brute-force local_ped: scan world pointer area
            // local_ped is usually at world_ptr + some_offset
            for (int ped_off = 0x8; ped_off < 0x400; ped_off += 8) {
                uintptr_t ped_ptr = g_memory.Read<uintptr_t>(proc, world_ptr + ped_off);
                if (ped_ptr == 0 || !IsValidPointer(ped_ptr, mod, sz)) continue;
                float hp = g_memory.Read<float>(proc, ped_ptr + Offsets::Common::kPedHealthOffset);
                if (hp < 50.f || hp > 200.f) continue;
                float armor = g_memory.Read<float>(proc, ped_ptr + Offsets::Common::kPedArmorOffset);
                if (armor < 0.f || armor > 100.f) continue;
                g.local_ped_sub_offset = ped_off;
                g.local_ped_is_sub = true;
                g.offset_local_ped = 0;
                std::cout << "[NOX] Local ped offset (AOB brute-force): 0x" << std::hex << ped_off
                          << " hp=" << std::dec << hp << " armor=" << armor << std::endl;
                break;
            }

            m_version_index = -1;
            return true;
        }
    }

    // ── Step 3: brute-force scan for world pointer ──
    // Strategy: scan module for pointers to heap, find one pointing to a ped-like struct
    std::cout << "[NOX] AOB failed, brute-force scanning module..." << std::endl;

    int ped_offs[] = { 0x8, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70 };
    int hp_offs[]  = { 0x10b8, 0x10a8, 0x10c0, 0x10a0, 0x1098, 0x1090, 0x1080, 0x10d0, 0x10e0, 0x1100, 0x1000, 0xf00, 0xe00, 0xd00, 0xc00, 0xb00 };

    for (uintptr_t off = 0; off < sz; off += 8) {
        uintptr_t ptr = g_memory.Read<uintptr_t>(proc, mod + off);
        if (ptr == 0 || !IsValidPointer(ptr, mod, sz)) continue;

        // Quick check: read a few bytes at ptr to see if it looks like a struct (non-zero pointer at offset 0)
        uintptr_t first_field = g_memory.Read<uintptr_t>(proc, ptr);
        if (first_field == 0 || IsValidPointer(first_field, mod, sz)) continue;

        for (int po : ped_offs) {
            uintptr_t local = g_memory.Read<uintptr_t>(proc, ptr + po);
            if (local == 0 || !IsValidPointer(local, mod, sz)) continue;

            for (int ho : hp_offs) {
                float hp = g_memory.Read<float>(proc, local + ho);
                if (hp < 50.f || hp > 200.f) continue;

                // Triple verify HP
                if (g_memory.Read<float>(proc, local + ho) != hp) continue;
                if (g_memory.Read<float>(proc, local + ho) != hp) continue;

                // Model hash at 0x20 — should be non-zero
                uint32_t model = g_memory.Read<uint32_t>(proc, local + 0x20);
                if (model == 0) continue;

                // Position: try common velocity offsets minus 0x50
                int vel_offs[] = { 0xd10, 0xd00, 0xd20, 0xcf0, 0xce0, 0xcd0, 0xc00, 0xb00, 0xa00, 0x900 };
                for (int vo : vel_offs) {
                    vec3 pos = g_memory.Read<vec3>(proc, local + vo - 0x50);
                    if (std::isnan(pos.x) || std::isnan(pos.y) || std::isnan(pos.z)) continue;
                    if (std::abs(pos.x) > 8000.f || std::abs(pos.y) > 8000.f || std::abs(pos.z) > 2000.f) continue;
                    if (pos.x == 0.f && pos.y == 0.f && pos.z == 0.f) continue;

                    // Verify position stability
                    vec3 pos2 = g_memory.Read<vec3>(proc, local + vo - 0x50);
                    if (std::abs(pos.x - pos2.x) > 0.01f || std::abs(pos.y - pos2.y) > 0.01f) continue;

                    g.offset_world = off;
                    g.offset_local_ped = 0;
                    g.local_ped_sub_offset = po;
                    g.local_ped_is_sub = true;
                    const_cast<int&>(Offsets::Common::kPedHealthOffset) = ho;
                    const_cast<int&>(Offsets::Common::kVelocityOffset) = vo;

                    std::cout << "[NOX] World offset: 0x" << std::hex << off << std::dec << std::endl;
                    std::cout << "[NOX] Local ped offset: 0x" << std::hex << po << std::dec << std::endl;
                    std::cout << "[NOX] Health offset: 0x" << std::hex << ho << std::dec << " HP=" << hp << std::endl;
                    std::cout << "[NOX] Velocity offset: 0x" << std::hex << vo << std::dec << std::endl;
                    std::cout << "[NOX] Position: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
                    std::cout << "[NOX] Model hash: 0x" << std::hex << model << std::dec << std::endl;

                    // Find replay
                    for (uintptr_t roff = 0; roff < sz; roff += 8) {
                        if (roff == off) continue;
                        uintptr_t rptr = g_memory.Read<uintptr_t>(proc, mod + roff);
                        if (rptr == 0 || !IsValidPointer(rptr, mod, sz)) continue;
                        int32_t mp = g_memory.Read<int32_t>(proc, rptr + 0x18);
                        if (mp < 1 || mp > 256) continue;
                        g.offset_replay = roff;
                        std::cout << "[NOX] Replay offset: 0x" << std::hex << roff
                                  << " max_peds=" << std::dec << mp << std::endl;
                        break;
                    }

                    m_version_index = -1;
                    return true;
                }
            }
        }
    }

    std::cerr << "[NOX] All detection methods failed!" << std::endl;
    return false;
}

// ── AOB resolution (with fallback) ──────────────────────────────────────────
bool GameMemory::ResolveOffsets() {
    HANDLE proc = g.game_process;
    auto& mod = g.game_module_base;
    auto& sz  = g.game_module_size;

    uintptr_t ent = g_memory.PatternScanModule(proc, mod, sz,
                                               Offsets::Patterns::kEntityListPattern);
    if (ent) {
        g.offset_entity_list = g_memory.ResolveRipRelative(proc, ent, 4, 8);
    }
    if (g.offset_entity_list == 0) {
        g.offset_entity_list = g.offset_world + Offsets::Common::kEntityListStride;
    }

    uintptr_t pi = g_memory.PatternScanModule(proc, mod, sz,
                                              Offsets::Patterns::kPlayerInfoPattern);
    if (pi) {
        g.offset_player_info = g_memory.ResolveRipRelative(proc, pi, 4, 8);
    }
    if (g.offset_player_info == 0) {
        g.offset_player_info = g.offset_entity_list + Offsets::Common::kPlayerInfoStride;
    }

    return true;
}

// ── Read local player ────────────────────────────────────────────────────────
Ped GameMemory::ReadLocalPlayer() {
    Ped ped{};
    HANDLE proc = g.game_process;

    // Hardcoded: offset_local_ped is an RVA → game_module_base + offset
    // AOB/brute: local_ped_is_sub → world_ptr + local_ped_sub_offset
    uintptr_t local_ped_ptr = 0;
    if (g.local_ped_is_sub) {
        uintptr_t world_ptr = g_memory.Read<uintptr_t>(proc, g.game_module_base + g.offset_world);
        if (world_ptr)
            local_ped_ptr = g_memory.Read<uintptr_t>(proc, world_ptr + g.local_ped_sub_offset);
    } else {
        local_ped_ptr = g_memory.Read<uintptr_t>(proc, g.game_module_base + g.offset_local_ped);
    }
    if (local_ped_ptr == 0) return ped;

    ped.address    = local_ped_ptr;
    ped.health     = g_memory.Read<float>(proc, local_ped_ptr + Offsets::Common::kPedHealthOffset);
    ped.armor      = g_memory.Read<float>(proc, local_ped_ptr + Offsets::Common::kPedArmorOffset);
    ped.velocity   = g_memory.Read<vec3> (proc, local_ped_ptr + Offsets::Common::kVelocityOffset);
    ped.position   = g_memory.Read<vec3> (proc, local_ped_ptr + 0x90);   // CPhysical::m_vecPosition
    ped.team       = g_memory.Read<int>  (proc, local_ped_ptr + Offsets::Common::kTeamOffset);
    ped.model_hash = g_memory.Read<uint32_t>(proc, local_ped_ptr + 0x20);
    ped.is_alive   = ped.health > 0.f;
    ped.is_valid   = true;

    ped.weapon_manager = g_memory.Read<uintptr_t>(proc, local_ped_ptr + Offsets::Common::kWeaponManagerOffset);
    if (ped.weapon_manager) {
        ped.current_weapon_hash = g_memory.Read<uint32_t>(proc, ped.weapon_manager + 0x60);
    }

    return ped;
}

// ── Read player by index ─────────────────────────────────────────────────────
Ped GameMemory::ReadPlayer(int index) {
    Ped ped{};
    HANDLE proc = g.game_process;

    uintptr_t replay_ptr = g_memory.Read<uintptr_t>(proc, g.game_module_base + g.offset_replay);
    if (replay_ptr == 0) return ped;

    uintptr_t pool_base = g_memory.Read<uintptr_t>(proc, replay_ptr + 0x10);
    int32_t max_count   = g_memory.Read<int32_t>(proc, replay_ptr + 0x18);
    if (pool_base == 0 || index < 0 || index >= max_count) return ped;

    uintptr_t ped_ptr = g_memory.Read<uintptr_t>(proc, pool_base + (uintptr_t)index * 8);
    if (ped_ptr == 0) return ped;

    ped.address    = ped_ptr;
    ped.health     = g_memory.Read<float>(proc, ped_ptr + Offsets::Common::kPedHealthOffset);
    ped.armor      = g_memory.Read<float>(proc, ped_ptr + Offsets::Common::kPedArmorOffset);
    ped.velocity   = g_memory.Read<vec3> (proc, ped_ptr + Offsets::Common::kVelocityOffset);
    ped.position   = g_memory.Read<vec3> (proc, ped_ptr + 0x90);
    ped.team       = g_memory.Read<int>  (proc, ped_ptr + Offsets::Common::kTeamOffset);
    ped.model_hash = g_memory.Read<uint32_t>(proc, ped_ptr + 0x20);
    ped.is_alive   = ped.health > 0.f;
    ped.is_valid   = (ped.address != 0);

    ped.player_info = g_memory.Read<uintptr_t>(proc, ped_ptr + Offsets::Common::kPlayerInfoStride);
    if (ped.player_info) {
        ped.player_id = g_memory.Read<int>(proc, ped.player_info + 0x78);
        std::string name = g_memory.ReadString(proc, ped.player_info + 0x84, 64);
        strncpy_s(ped.player_name, name.c_str(), sizeof(ped.player_name) - 1);
    }

    ped.weapon_manager = g_memory.Read<uintptr_t>(proc, ped_ptr + Offsets::Common::kWeaponManagerOffset);
    if (ped.weapon_manager) {
        ped.current_weapon_hash = g_memory.Read<uint32_t>(proc, ped.weapon_manager + 0x60);
    }

    return ped;
}

// ── Read vehicle ─────────────────────────────────────────────────────────────
Vehicle GameMemory::ReadVehicle(uintptr_t address) {
    Vehicle veh{};
    if (address == 0) return veh;
    HANDLE proc = g.game_process;

    veh.address      = address;
    veh.health       = g_memory.Read<float>(proc, address + 0x908);
    veh.position     = g_memory.Read<vec3>(proc, address + 0x90);
    veh.velocity     = g_memory.Read<vec3>(proc, address + 0x60);
    veh.model_hash   = g_memory.Read<uint32_t>(proc, address + 0x20);
    veh.engine_health = g_memory.Read<float>(proc, address + 0x8e4);
    veh.doors_locked = g_memory.Read<int>(proc, address + 0x1380) == 2;

    float speed_mph = g_memory.Read<float>(proc, address + 0xD84);
    veh.speed = speed_mph;
    veh.is_valid = true;

    return veh;
}

// ── Read view matrix ─────────────────────────────────────────────────────────
ViewMatrix GameMemory::ReadViewMatrix() {
    ViewMatrix vm{};
    HANDLE proc = g.game_process;

    uintptr_t cam_ptr = g_memory.Read<uintptr_t>(proc, g.game_module_base + g.offset_camera);
    if (cam_ptr == 0) return vm;

    uintptr_t cam_game = g_memory.Read<uintptr_t>(proc, cam_ptr + 0x30);
    if (cam_game == 0) return vm;

    uintptr_t cam_matrix = g_memory.Read<uintptr_t>(proc, cam_game + 0x40);
    if (cam_matrix == 0) return vm;

    g_memory.ReadBuffer(proc, cam_matrix + Offsets::kViewMatrixOffset, &vm, sizeof(ViewMatrix));
    return vm;
}

// ── Read player name ─────────────────────────────────────────────────────────
std::string GameMemory::ReadPlayerName(uintptr_t player_info) {
    if (player_info == 0) return "";
    return g_memory.ReadString(g.game_process, player_info + 0x84, 64);
}

// ── World-to-screen ──────────────────────────────────────────────────────────
bool GameMemory::WorldToScreen(const ViewMatrix& vm, const vec3& world,
                               vec2& screen, float sw, float sh) {
    float sx = sw * 0.5f;
    float sy = sh * 0.5f;

    float cx = vm.m[0][0] * world.x + vm.m[1][0] * world.y + vm.m[2][0] * world.z + vm.m[3][0];
    float cy = vm.m[0][1] * world.x + vm.m[1][1] * world.y + vm.m[2][1] * world.z + vm.m[3][1];
    float cz = vm.m[0][2] * world.x + vm.m[1][2] * world.y + vm.m[2][2] * world.z + vm.m[3][2];
    float cw = vm.m[0][3] * world.x + vm.m[1][3] * world.y + vm.m[2][3] * world.z + vm.m[3][3];

    if (cw < 0.001f) return false;

    float inv = 1.f / cw;
    float ndc_x = cx * inv;
    float ndc_y = cy * inv;

    screen.x = sx + (ndc_x * sx);
    screen.y = sy - (ndc_y * sy);

    return true;
}

// ── Bone position ────────────────────────────────────────────────────────────
vec3 GameMemory::GetBonePosition(uintptr_t ped_address, int bone_index) {
    if (ped_address == 0) return {};
    HANDLE proc = g.game_process;

    uintptr_t bone_array = g_memory.Read<uintptr_t>(proc, ped_address + Offsets::Common::kBoneArrayBase);
    if (bone_array == 0) return {};

    uintptr_t bone_entry = bone_array + bone_index * Offsets::Common::kBoneStride;
    vec3 pos = g_memory.Read<vec3>(proc, bone_entry);
    return pos;
}

// ── Max players ──────────────────────────────────────────────────────────────
int GameMemory::GetMaxPlayers() {
    HANDLE proc = g.game_process;
    uintptr_t replay_ptr = g_memory.Read<uintptr_t>(proc, g.game_module_base + g.offset_replay);
    if (replay_ptr == 0) return 0;
    return g_memory.Read<int>(proc, replay_ptr + 0x18);
}

// ── Entity list (replay ped pool) ─────────────────────────────────────────────
std::vector<uintptr_t> GameMemory::GetEntityList() {
    std::vector<uintptr_t> list;
    HANDLE proc = g.game_process;

    uintptr_t replay_ptr = g_memory.Read<uintptr_t>(proc, g.game_module_base + g.offset_replay);
    if (replay_ptr == 0) return list;

    uintptr_t pool_base = g_memory.Read<uintptr_t>(proc, replay_ptr + 0x10);
    int32_t max_count   = g_memory.Read<int32_t>(proc, replay_ptr + 0x18);
    if (pool_base == 0 || max_count <= 0 || max_count > 256) return list;

    for (int i = 0; i < max_count; ++i) {
        uintptr_t entry = g_memory.Read<uintptr_t>(proc, pool_base + (uintptr_t)i * 8);
        if (entry != 0) list.push_back(entry);
    }
    return list;
}

// ── Teleport ─────────────────────────────────────────────────────────────────
bool GameMemory::TeleportTo(vec3 position) {
    Ped local = ReadLocalPlayer();
    if (!local.is_valid) return false;
    return g_memory.Write<vec3>(g.game_process,
        local.address + 0x90, position);   // CPhysical::m_vecPosition
}

// ── Write aim angle (eski yöntem — kullanılmıyor) ─────────────────────────────
bool GameMemory::WriteAimAngle(vec3 angle) {
    Ped local = ReadLocalPlayer();
    if (!local.is_valid) return false;
    uintptr_t aim_addr = local.address + 0x408;
    return WriteProcessMemory(g.game_process,
        reinterpret_cast<LPVOID>(aim_addr), &angle, sizeof(vec3), nullptr) != 0;
}

// ── Silent aim: CWeaponManager::m_aimingAt override ──────────────────────────
//
//  GTA5 ateş pipeline'ı:
//    CPed → CWeaponManager → aktif silah raycast'ı
//    Raycast başlangıç = m_aimingFrom, bitiş = m_aimingAt
//    m_aimingAt'i hedef bone world pos'uyla override edince
//    kamera hareket etmez ama mermi oraya gider — gerçek silent aim.
//
//  Doğrulama adımları (ReClass.NET ile):
//    1. ped_addr + 0x145C = CWeaponManager*
//    2. +0x60 = current_weapon_hash (uint32) — bilinen değer, doğrulama noktası
//    3. Oyun içinde nişan alırken +kAimingAt adresindeki vec3 hedefe bakmalı
//
bool GameMemory::WriteAimingAt(uintptr_t ped_addr, vec3 target_world_pos) {
    if (ped_addr == 0) return false;
    HANDLE proc = g.game_process;

    uintptr_t wm = g_memory.Read<uintptr_t>(proc,
        ped_addr + Offsets::Common::kWeaponManagerOffset);
    if (wm == 0) return false;

    return g_memory.Write<vec3>(proc, wm + Offsets::WeaponMgr::kAimingAt,
                                target_world_pos);
}

bool GameMemory::WriteAimingFrom(uintptr_t ped_addr, vec3 from_world_pos) {
    if (ped_addr == 0) return false;
    HANDLE proc = g.game_process;

    uintptr_t wm = g_memory.Read<uintptr_t>(proc,
        ped_addr + Offsets::Common::kWeaponManagerOffset);
    if (wm == 0) return false;

    return g_memory.Write<vec3>(proc, wm + Offsets::WeaponMgr::kAimingFrom,
                                from_world_pos);
}
