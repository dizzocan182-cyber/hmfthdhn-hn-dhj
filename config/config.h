#pragma once
#include <string>
#include "../features/aimbot.h"
#include "../features/esp.h"
#include "../features/player_features.h"
#include "../features/vehicle_features.h"

class Config {
public:
    void Save(const std::string& path = "config.json");
    void Load(const std::string& path = "config.json");

    AimbotConfig* aimbot = nullptr;
    EspConfig* esp = nullptr;
    PlayerFeaturesConfig* player = nullptr;
    VehicleFeaturesConfig* vehicle = nullptr;
};

extern Config g_config;
