#include "vehicle_features.h"
#include "../game/game_memory.h"

extern GameMemory g_game;

void VehicleFeatures::Update() {
    if (!g_game.IsInitialized()) return;

    Ped local = g_game.ReadLocalPlayer();
    if (!local.is_valid || local.address == 0) return;

    HANDLE proc = g.game_process;
    uintptr_t vehicle_ptr = g_memory.Read<uintptr_t>(proc, local.address + 0xd28);
    if (vehicle_ptr == 0) return;

    Vehicle veh = g_game.ReadVehicle(vehicle_ptr);
    if (!veh.is_valid) return;

    if (m_config.god_mode) {
        g_memory.Write<float>(proc, vehicle_ptr + 0x908, 1000.0f);
    }

    if (m_config.fix_on_damage) {
        g_memory.Write<float>(proc, vehicle_ptr + 0x8e4, 1000.0f);
    }

    if (m_config.seatbelt) {
        g_memory.Write<int>(proc, vehicle_ptr + 0xc9c, 0);
        g_memory.Write<int>(proc, vehicle_ptr + 0xc9d, 1);
    }

    if (m_config.speed_boost) {
        vec3 forward = g_memory.Read<vec3>(proc, vehicle_ptr + 0x70);
        vec3 velocity = g_memory.Read<vec3>(proc, vehicle_ptr + 0x60);
        vec3 boosted = velocity + forward * m_config.boost_speed;
        g_memory.Write<vec3>(proc, vehicle_ptr + 0x60, boosted);
    }
}
