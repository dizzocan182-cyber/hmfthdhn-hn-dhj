#include "player_features.h"
#include "../game/game_memory.h"

extern GameMemory g_game;

static constexpr float kMaxHealth     = 200.0f;
static constexpr float kHealthBytes   = 200.0f; // 0x43480000
static constexpr float kZeroVelocity  = 0.0f;

void PlayerFeatures::Update() {
    if (!g_game.IsInitialized()) return;

    Ped local = g_game.ReadLocalPlayer();
    if (!local.is_valid || local.address == 0) return;

    HANDLE proc = g.game_process;
    uintptr_t addr = local.address;

    if (m_config.god_mode) {
        g_memory.Write<float>(proc, addr + Offsets::Common::kPedHealthOffset, kMaxHealth);
        g_memory.Write<int>(proc, addr + 0x188, 1);
    }

    if (m_config.unlimited_health) {
        g_memory.Write<float>(proc, addr + Offsets::Common::kPedHealthOffset, kMaxHealth);
    }

    if (m_config.no_clip) {
        g_memory.Write<vec3>(proc, addr + Offsets::Common::kVelocityOffset, { 0.f, 0.f, 0.f });
        g_memory.Write<int>(proc, addr + 0x1480, 0);
        g_memory.Write<int>(proc, addr + 0x1484, 0);
    }

    if (m_config.super_jump) {
        g_memory.Write<float>(proc, addr + 0xd80, m_config.jump_multiplier);
        g_memory.Write<int>(proc, addr + 0x1480, 2);
    }

    if (m_config.run_speed) {
        g_memory.Write<float>(proc, addr + 0x1ac0, m_config.run_speed_multiplier);
    }
}
