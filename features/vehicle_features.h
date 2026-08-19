#pragma once

struct VehicleFeaturesConfig {
    bool  god_mode        = false;
    bool  speed_boost     = false;
    float boost_speed     = 100.f;
    bool  fix_on_damage   = false;
    bool  seatbelt        = false;
};

class VehicleFeatures {
public:
    void Update();
    VehicleFeaturesConfig& GetConfig() { return m_config; }

private:
    VehicleFeaturesConfig m_config;
};
