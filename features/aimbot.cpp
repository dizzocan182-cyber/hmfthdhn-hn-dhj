#include "aimbot.h"
#include "../game/game_memory.h"
#include "../core/globals.h"
#include <cmath>
#include <algorithm>
#include <Windows.h>

extern GameMemory g_game;

// ─────────────────────────────────────────────────────────────────────────────
//  Silent Aim — Update
//
//  Ne yapar:
//    1. Tuşa basılı mı kontrol et
//    2. FOV içindeki en yakın hedefi bul (ekran pixel değil, world açısı)
//    3. Hedefin head bone pozisyonunu al
//    4. Velocity prediction varsa lead point hesapla
//    5. CWeaponManager::m_aimingAt'e hedef world pos'u yaz
//    6. Kamera hiç hareket etmez — tamamen silent
//
//  Offset doğrulama notu:
//    game_offsets.h → Offsets::WeaponMgr::kAimingAt
//    Çalışmıyorsa kAimingAt_v2 veya kAimingAt_v3'ü dene
// ─────────────────────────────────────────────────────────────────────────────
void Aimbot::Update(const std::vector<Ped>& players, const ViewMatrix& vm,
                    float sw, float sh) {
    if (!m_config.enabled) {
        m_aiming = false;
        m_config.active = false;
        m_target_index = -1;
        return;
    }

    // tuş basılı değilse sıfırla
    if (!(GetAsyncKeyState(m_config.key) & 0x8000)) {
        m_aiming = false;
        m_config.active = false;
        m_target_index = -1;
        return;
    }

    Ped local = g_game.ReadLocalPlayer();
    if (!local.is_valid || !local.is_alive) return;

    float best_fov   = m_config.fov;
    int   best_index = -1;
    vec3  best_pos{};

    for (int i = 0; i < static_cast<int>(players.size()); ++i) {
        const auto& p = players[i];
        if (!p.is_valid || !p.is_alive) continue;
        if (p.address == local.address) continue;
        if (m_config.team_check && p.team == local.team && p.team != 0) continue;

        float dist = local.position.distance_to(p.position);
        if (dist > m_config.max_distance) continue;

        // head bone al
        vec3 bone = g_game.GetBonePosition(p.address, m_config.bone_index);
        if (bone.x == 0.f && bone.y == 0.f && bone.z == 0.f) {
            // bone başarısız — ped pozisyonuna fall back
            bone = p.position;
            bone.z += 0.7f; // yaklaşık kafa yüksekliği
        }

        // FOV kontrolü — world space'te açı (derece)
        float fov = GetFovDeg(local.position, bone, vm, sw, sh);
        if (fov < best_fov) {
            best_fov   = fov;
            best_index = i;
            best_pos   = bone;
        }
    }

    if (best_index < 0) {
        m_aiming = false;
        m_config.active = false;
        m_target_index = -1;
        return;
    }

    m_target_index = best_index;
    m_aiming = true;
    m_config.active = true;

    // prediction — hedef hızına göre lead point
    vec3 aim_pos = best_pos;
    if (m_config.prediction) {
        const Ped& target = players[best_index];
        aim_pos = PredictPosition(best_pos, target.velocity,
                                  local.position, m_config.bullet_speed);
    }

    // ── Silent aim core ───────────────────────────────────────────────────────
    //  local ped'in CWeaponManager::m_aimingAt'ini override et.
    //  Kamera hareket etmez. Mermi aim_pos'a gider.
    g_game.WriteAimingAt(local.address, aim_pos);
}

// ── FOV hesabı — world space'te derece cinsinden ─────────────────────────────
//
//  Ekran piksel bazlı FOV çözünürlüğe göre değişir.
//  Bu versiyon view matrix'i kullanarak kamera yönüne göre
//  hedefin açısal mesafesini hesaplar — çözünürlükten bağımsız.
//
float Aimbot::GetFovDeg(const vec3& local_pos, const vec3& target_pos,
                         const ViewMatrix& vm, float sw, float sh) {
    // kamera forward vektörü: view matrix'in 3. satırı (z axis)
    vec3 cam_fwd = {
        vm.m[0][2],
        vm.m[1][2],
        vm.m[2][2]
    };

    // local'den hedefe yön
    vec3 to_target = (target_pos - local_pos).normalized();

    // dot product → cos(açı)
    float dot = cam_fwd.dot(to_target);
    dot = std::clamp(dot, -1.f, 1.f);

    // radyandan dereceye
    float angle_deg = std::acos(dot) * (180.f / 3.14159265f);
    return angle_deg;
}

// ── Velocity prediction ───────────────────────────────────────────────────────
//
//  Hedef sabit duruyorsa bu fonksiyon pos'u değiştirmez.
//  Koşuyorsa: mermi uçuş süresi * hedef velocity = lead offset
//
vec3 Aimbot::PredictPosition(const vec3& pos, const vec3& vel,
                              const vec3& shooter_pos, float bullet_speed) {
    if (bullet_speed <= 0.f) return pos;

    float dist = shooter_pos.distance_to(pos);
    if (dist < 0.1f) return pos;

    // mermi uçuş süresi (saniye)
    float flight_time = dist / bullet_speed;

    // lead point
    return {
        pos.x + vel.x * flight_time,
        pos.y + vel.y * flight_time,
        pos.z + vel.z * flight_time
    };
}
