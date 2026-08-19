#pragma once
#include "../core/memory.h"
#include "../core/globals.h"
#include "game_offsets.h"
#include "game_structs.h"
#include <string>
#include <vector>

class GameMemory {
public:
    bool Initialize();

    Ped ReadLocalPlayer();
    Ped ReadPlayer(int index);
    Vehicle ReadVehicle(uintptr_t address);
    ViewMatrix ReadViewMatrix();

    std::string ReadPlayerName(uintptr_t player_info);

    bool WorldToScreen(const ViewMatrix& vm, const vec3& world,
                       vec2& screen, float screen_w, float screen_h);

    vec3 GetBonePosition(uintptr_t ped_address, int bone_index);

    int GetMaxPlayers();
    std::vector<uintptr_t> GetEntityList();

    bool TeleportTo(vec3 position);
    bool WriteAimAngle(vec3 angle);          // eski — kullanılmıyor, referans için kalıyor
    bool WriteAimingAt(uintptr_t ped_addr, vec3 target_world_pos);  // silent aim
    bool WriteAimingFrom(uintptr_t ped_addr, vec3 from_world_pos);  // isteğe bağlı

    bool IsInitialized() const  { return m_initialized; }
    int  GetVersionIndex() const { return m_version_index; }

private:
    bool m_initialized  = false;
    int  m_version_index = -1;
    ViewMatrix m_cached_view{};

    bool DetectVersion();
    bool ResolveOffsets();
};

extern GameMemory g_game;
