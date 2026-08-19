#pragma once
#include "imgui.h"
#include "../features/aimbot.h"
#include "../features/esp.h"
#include "../features/player_features.h"
#include "../features/vehicle_features.h"
#include "../features/teleport.h"

class Menu {
public:
    Menu(AimbotConfig& ac, EspConfig& ec,
         PlayerFeaturesConfig& pc, VehicleFeaturesConfig& vc,
         Teleport& tp)
        : aimbot_cfg(ac), esp_cfg(ec),
          player_cfg(pc), vehicle_cfg(vc),
          teleport(tp) {}

    void Render();

    AimbotConfig&         aimbot_cfg;
    EspConfig&            esp_cfg;
    PlayerFeaturesConfig& player_cfg;
    VehicleFeaturesConfig& vehicle_cfg;
    Teleport&             teleport;

    enum class Tab { Aimbot = 0, ESP, Player, Vehicle, Teleport, Config, COUNT };
    int current_tab = 0;

private:
    void RenderHeader(ImDrawList* dl, ImVec2 win_pos);
    void RenderSidebar(ImDrawList* dl, ImVec2 win_pos, float sidebar_w, float content_h);
    void RenderContent();

    void RenderAimbotTab();
    void RenderEspTab();
    void RenderPlayerTab();
    void RenderVehicleTab();
    void RenderTeleportTab();
    void RenderConfigTab();

    static constexpr float kMenuW     = 600.f;
    static constexpr float kMenuH     = 440.f;
    static constexpr float kHeaderH   = 52.f;
    static constexpr float kSidebarW  = 148.f;
    static constexpr float kPadding   = 14.f;

    float custom_x = 0.f, custom_y = 0.f, custom_z = 0.f;
};

extern Menu* g_menu;
