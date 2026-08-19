#pragma once

struct PlayerFeaturesConfig {
    bool  god_mode             = false;
    bool  unlimited_health     = false;
    bool  no_clip              = false;
    bool  super_jump           = false;
    bool  run_speed            = false;
    float run_speed_multiplier = 2.0f;
    float jump_multiplier      = 2.0f;
};

class PlayerFeatures {
public:
    void Update();
    PlayerFeaturesConfig& GetConfig() { return m_config; }

private:
    PlayerFeaturesConfig m_config;
};
