#pragma once
#include "../core/types.h"
#include "../game/game_structs.h"
#include <vector>

struct AimbotConfig {
    bool  enabled        = false;
    bool  active         = false;   // set by Update — menu status göstergesi için

    // hedef seçim
    int   bone_index     = 0;       // 0 = head (silent aim için head bone önerilir)
    float fov            = 180.f;   // silent aim'de geniş FOV mantıklı — ekran piksel değil derece
    float max_distance   = 200.f;
    int   key            = 0x02;    // RMB
    bool  team_check     = true;
    bool  visible_check  = false;   // silent aim'de visibility check gerekmez

    // prediction
    bool  prediction     = true;    // hedef velocity'sine göre lead point hesapla
    float bullet_speed   = 300.f;   // m/s — silaha göre ayarla (pistol ~370, rifle ~600)
};

class Aimbot {
public:
    void Update(const std::vector<Ped>& players, const ViewMatrix& vm,
                float sw, float sh);

    AimbotConfig&       GetConfig()       { return m_config; }
    const AimbotConfig& GetConfig() const { return m_config; }

    bool IsActive() const       { return m_aiming; }
    int  GetTargetIndex() const { return m_target_index; }

private:
    AimbotConfig m_config;
    bool  m_aiming       = false;
    int   m_target_index = -1;

    // ekran merkezine derece cinsinden açısal mesafe
    float GetFovDeg(const vec3& local_pos, const vec3& target_pos,
                    const ViewMatrix& vm, float sw, float sh);

    // velocity prediction — hedef hareket ediyorsa lead point
    vec3  PredictPosition(const vec3& pos, const vec3& vel,
                          const vec3& shooter_pos, float bullet_speed);
};
