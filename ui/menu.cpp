#include "menu.h"
#include "widgets.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "../core/globals.h"
#include "../config/config.h"

Menu* g_menu = nullptr;

// ── palette shortcuts (dx11.cpp ile eşleşmeli) ───────────────────────────────
static inline ImU32 C_BG0()       { return ImColor(0.055f, 0.055f, 0.075f, 1.f); }
static inline ImU32 C_BG1()       { return ImColor(0.075f, 0.075f, 0.100f, 1.f); }
static inline ImU32 C_BG2()       { return ImColor(0.095f, 0.095f, 0.130f, 1.f); }
static inline ImU32 C_ACCENT()    { return ImColor(0.45f,  0.25f,  0.95f,  1.f); }
static inline ImU32 C_ACCENT2()   { return ImColor(0.22f,  0.55f,  1.00f,  1.f); }
static inline ImU32 C_BORDER()    { return ImColor(0.22f,  0.22f,  0.32f,  0.7f); }
static inline ImU32 C_BORDER_A()  { return ImColor(0.45f,  0.25f,  0.95f,  0.5f); }
static inline ImU32 C_TEXT()      { return ImColor(0.90f,  0.90f,  0.95f,  1.f); }
static inline ImU32 C_TEXT_DIM()  { return ImColor(0.45f,  0.45f,  0.55f,  1.f); }

// ── ana render ───────────────────────────────────────────────────────────────
void Menu::Render() {
    if (!g.menu_open) return;

    ImGui::SetNextWindowSize(ImVec2(kMenuW, kMenuH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(120.f, 80.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.f); // manuel çiziyoruz

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

    if (!ImGui::Begin("##nox_menu", nullptr, flags)) {
        ImGui::End();
        ImGui::PopStyleVar(3);
        return;
    }
    ImGui::PopStyleVar(3);

    ImDrawList* dl      = ImGui::GetWindowDrawList();
    ImVec2      win_pos = ImGui::GetWindowPos();

    const float content_h = kMenuH - kHeaderH;

    // ── Pencere arka planı ───────────────────────────────────────────────────
    // ana bg
    dl->AddRectFilled(win_pos,
                      ImVec2(win_pos.x + kMenuW, win_pos.y + kMenuH),
                      C_BG0(), 12.f);

    // dış kenar glow
    dl->AddRect(win_pos,
                ImVec2(win_pos.x + kMenuW, win_pos.y + kMenuH),
                C_BORDER_A(), 12.f, 0, 1.5f);

    RenderHeader(dl, win_pos);
    RenderSidebar(dl, win_pos, kSidebarW, content_h);

    // ── içerik alanı bg ──────────────────────────────────────────────────────
    ImVec2 content_tl = ImVec2(win_pos.x + kSidebarW, win_pos.y + kHeaderH);
    dl->AddRectFilled(content_tl,
                      ImVec2(win_pos.x + kMenuW, win_pos.y + kMenuH),
                      C_BG2(), 0.f);

    // sidebar / content ayırıcı çizgi
    dl->AddLine(ImVec2(win_pos.x + kSidebarW, win_pos.y + kHeaderH),
                ImVec2(win_pos.x + kSidebarW, win_pos.y + kMenuH),
                C_BORDER(), 1.f);

    // içerik child
    ImGui::SetCursorPos(ImVec2(kSidebarW + kPadding, kHeaderH + kPadding));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    if (ImGui::BeginChild("##content",
            ImVec2(kMenuW - kSidebarW - kPadding * 2.f,
                   content_h - kPadding * 2.f),
            false,
            ImGuiWindowFlags_NoScrollbar)) {
        RenderContent();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::End();
}

// ── Header: NOX branding + versiyon + FPS ────────────────────────────────────
void Menu::RenderHeader(ImDrawList* dl, ImVec2 wp) {
    // header bg — biraz daha koyu
    dl->AddRectFilled(wp,
                      ImVec2(wp.x + kMenuW, wp.y + kHeaderH),
                      C_BG1(), 0.f);

    // alt kenarda ince accent çizgi
    dl->AddLine(ImVec2(wp.x, wp.y + kHeaderH - 1.f),
                ImVec2(wp.x + kMenuW, wp.y + kHeaderH - 1.f),
                C_BORDER_A(), 1.5f);

    // sol köşe accent bar (dekoratif)
    dl->AddRectFilled(ImVec2(wp.x, wp.y),
                      ImVec2(wp.x + 4.f, wp.y + kHeaderH),
                      C_ACCENT(), 0.f);

    // NOX - büyük gradient hissi için iki renk üst üste
    float name_x = wp.x + 20.f;
    float name_y = wp.y + (kHeaderH - 22.f) * 0.5f;

    // gölge
    dl->AddText(ImGui::GetFont(), 22.f,
                ImVec2(name_x + 1.f, name_y + 1.f),
                ImColor(0, 0, 0, 120), "NOX");
    // ana
    dl->AddText(ImGui::GetFont(), 22.f,
                ImVec2(name_x, name_y),
                C_ACCENT(), "NOX");

    // "by nox" alt yazı
    dl->AddText(ImGui::GetFont(), 11.f,
                ImVec2(name_x + 2.f, name_y + 24.f),
                C_TEXT_DIM(), "premium cheat");

    // sağ taraf: versiyon + fps
    char fps_buf[32];
    snprintf(fps_buf, sizeof(fps_buf), "%.0f fps", ImGui::GetIO().Framerate);

    float ver_x = wp.x + kMenuW - 90.f;
    float ver_y = wp.y + 10.f;
    dl->AddText(ImGui::GetFont(), 11.f,
                ImVec2(ver_x, ver_y),
                C_TEXT_DIM(), "v2.0");
    dl->AddText(ImGui::GetFont(), 11.f,
                ImVec2(ver_x, ver_y + 14.f),
                C_TEXT_DIM(), fps_buf);

    // kapatma butonu (X)
    ImVec2 close_pos = ImVec2(wp.x + kMenuW - 26.f, wp.y + 10.f);
    ImGui::SetCursorPos(ImVec2(kMenuW - 26.f, 10.f));
    ImGui::PushID("##close");
    if (ImGui::InvisibleButton("x", ImVec2(18.f, 18.f)))
        g.menu_open = false;
    bool hov = ImGui::IsItemHovered();
    ImGui::PopID();

    ImU32 x_col = hov ? ImColor(1.f, 0.3f, 0.3f, 1.f) : C_TEXT_DIM();
    dl->AddText(ImGui::GetFont(), 14.f,
                ImVec2(close_pos.x + 2.f, close_pos.y + 1.f),
                x_col, "x");
}

// ── Sidebar: navigasyon butonları ────────────────────────────────────────────
void Menu::RenderSidebar(ImDrawList* dl, ImVec2 wp,
                          float sidebar_w, float content_h) {
    dl->AddRectFilled(ImVec2(wp.x, wp.y + kHeaderH),
                      ImVec2(wp.x + sidebar_w, wp.y + kMenuH),
                      C_BG1(), 0.f);

    struct TabDef { const char* label; Tab tab; };
    static constexpr TabDef tabs[] = {
        { "  Aimbot",   Tab::Aimbot   },
        { "  ESP",      Tab::ESP      },
        { "  Player",   Tab::Player   },
        { "  Vehicle",  Tab::Vehicle  },
        { "  Teleport", Tab::Teleport },
        { "  Config",   Tab::Config   },
    };

    const float btn_h   = 36.f;
    const float btn_gap = 4.f;
    float       start_y = kHeaderH + 12.f;

    ImGui::SetCursorPos(ImVec2(8.f, start_y));

    for (const auto& t : tabs) {
        bool sel = (current_tab == static_cast<int>(t.tab));
        ImGui::SetCursorPos(ImVec2(8.f, start_y));

        if (Widgets::SidebarButton(t.label, nullptr, sel,
                                    ImVec2(sidebar_w - 16.f, btn_h)))
            current_tab = static_cast<int>(t.tab);

        start_y += btn_h + btn_gap;
    }

    // alt: "NOX" küçük watermark
    dl->AddText(ImGui::GetFont(), 10.f,
                ImVec2(wp.x + sidebar_w * 0.5f - 10.f,
                       wp.y + kMenuH - 18.f),
                ImColor(0.25f, 0.25f, 0.35f, 1.f),
                "NOX 2025");
}

// ── Tab içerik dispatch ───────────────────────────────────────────────────────
void Menu::RenderContent() {
    switch (static_cast<Tab>(current_tab)) {
        case Tab::Aimbot:   RenderAimbotTab();   break;
        case Tab::ESP:      RenderEspTab();      break;
        case Tab::Player:   RenderPlayerTab();   break;
        case Tab::Vehicle:  RenderVehicleTab();  break;
        case Tab::Teleport: RenderTeleportTab(); break;
        case Tab::Config:   RenderConfigTab();   break;
        default: break;
    }
}

// ────────────────────────────────────────────────────────────────────────────
// TAB: AIMBOT (Silent Aim)
// ────────────────────────────────────────────────────────────────────────────
void Menu::RenderAimbotTab() {
    Widgets::Toggle("Silent Aim", &aimbot_cfg.enabled);
    ImGui::Spacing();

    Widgets::SectionHeader("TARGET");
    static const char* bone_names[] = { "Head", "Neck", "Spine", "Pelvis" };
    ImGui::SetNextItemWidth(-1.f);
    ImGui::Combo("##bone", &aimbot_cfg.bone_index, bone_names, 4);
    ImGui::Spacing();

    Widgets::SectionHeader("SETTINGS");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("FOV##aim", &aimbot_cfg.fov, 5.f, 180.f, "%.0f deg");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("Max Dist##aim", &aimbot_cfg.max_distance, 10.f, 500.f, "%.0f m");
    ImGui::Spacing();
    Widgets::Hotkey("Aim Key", &aimbot_cfg.key);
    ImGui::Spacing();

    Widgets::SectionHeader("PREDICTION");
    Widgets::Toggle("Bullet Prediction", &aimbot_cfg.prediction);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("Bullet Speed##aim", &aimbot_cfg.bullet_speed, 50.f, 1000.f, "%.0f m/s");
    ImGui::Spacing();

    Widgets::SectionHeader("FILTERS");
    Widgets::Toggle("Team Check", &aimbot_cfg.team_check);
    ImGui::Spacing();

    Widgets::SectionHeader("STATUS");
    Widgets::StatusIndicator("Silent Aim", aimbot_cfg.active, "Locking", "Idle");
}

// ────────────────────────────────────────────────────────────────────────────
// TAB: ESP
// ────────────────────────────────────────────────────────────────────────────
void Menu::RenderEspTab() {
    Widgets::Toggle("Enable ESP", &esp_cfg.enabled);
    ImGui::Spacing();

    Widgets::SectionHeader("BOX");
    Widgets::Toggle("Corner Box",  &esp_cfg.corner_box);
    Widgets::Toggle("Full Box",    &esp_cfg.box);
    Widgets::Toggle("Box Filled",  &esp_cfg.box_filled);
    ImGui::Spacing();

    Widgets::SectionHeader("BARS");
    Widgets::Toggle("Health Bar",  &esp_cfg.health_bar);
    Widgets::Toggle("Armor Bar",   &esp_cfg.armor_bar);
    ImGui::Spacing();

    Widgets::SectionHeader("INFO");
    Widgets::Toggle("Name",        &esp_cfg.name);
    Widgets::Toggle("Distance",    &esp_cfg.distance);
    Widgets::Toggle("Weapon",      &esp_cfg.weapon);
    ImGui::Spacing();

    Widgets::SectionHeader("VISUAL");
    Widgets::Toggle("Snaplines",      &esp_cfg.snaplines);
    Widgets::Toggle("Skeleton",       &esp_cfg.skeleton);
    Widgets::Toggle("Head Dot",       &esp_cfg.head_dot);
    Widgets::Toggle("Distance Fade",  &esp_cfg.distance_fade);
    ImGui::Spacing();

    Widgets::SectionHeader("OPTIONS");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("Max Dist##esp", &esp_cfg.max_distance, 10.f, 500.f, "%.0f m");
    Widgets::Toggle("Team Check", &esp_cfg.team_check);
    ImGui::Spacing();

    Widgets::SectionHeader("COLORS");
    Widgets::ColorEdit4("Box Color",
        reinterpret_cast<Widgets::ColorRGBA&>(esp_cfg.box_color));
    Widgets::ColorEdit4("Armor Color",
        reinterpret_cast<Widgets::ColorRGBA&>(esp_cfg.armor_color));
    Widgets::ColorEdit4("Name Color",
        reinterpret_cast<Widgets::ColorRGBA&>(esp_cfg.name_color));
}

// ────────────────────────────────────────────────────────────────────────────
// TAB: PLAYER
// ────────────────────────────────────────────────────────────────────────────
void Menu::RenderPlayerTab() {
    Widgets::SectionHeader("PROTECTION");
    Widgets::Toggle("God Mode",         &player_cfg.god_mode);
    Widgets::Toggle("Unlimited Health", &player_cfg.unlimited_health);
    Widgets::Toggle("No Clip",          &player_cfg.no_clip);
    ImGui::Spacing();

    Widgets::SectionHeader("MOVEMENT");
    Widgets::Toggle("Super Jump", &player_cfg.super_jump);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("Jump Mult##pl", &player_cfg.jump_multiplier, 1.f, 10.f, "x%.1f");
    ImGui::Spacing();
    Widgets::Toggle("Run Speed", &player_cfg.run_speed);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("Speed Mult##pl", &player_cfg.run_speed_multiplier, 1.f, 10.f, "x%.1f");
}

// ────────────────────────────────────────────────────────────────────────────
// TAB: VEHICLE
// ────────────────────────────────────────────────────────────────────────────
void Menu::RenderVehicleTab() {
    Widgets::SectionHeader("PROTECTION");
    Widgets::Toggle("Vehicle God Mode", &vehicle_cfg.god_mode);
    Widgets::Toggle("Fix on Damage",    &vehicle_cfg.fix_on_damage);
    Widgets::Toggle("Seatbelt",         &vehicle_cfg.seatbelt);
    ImGui::Spacing();

    Widgets::SectionHeader("PERFORMANCE");
    Widgets::Toggle("Speed Boost", &vehicle_cfg.speed_boost);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderFloat("Boost##veh", &vehicle_cfg.boost_speed, 50.f, 500.f, "%.0f");
}

// ────────────────────────────────────────────────────────────────────────────
// TAB: TELEPORT
// ────────────────────────────────────────────────────────────────────────────
void Menu::RenderTeleportTab() {
    Widgets::SectionHeader("LOCATIONS");

    const auto& locs = teleport.GetLocations();
    for (int i = 0; i < (int)locs.size(); ++i) {
        if (Widgets::ListItem(locs[i].name, false))
            teleport.TeleportToLocation(i);
    }

    ImGui::Spacing();
    Widgets::SectionHeader("CUSTOM");

    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputFloat("X##tp", &custom_x, 0.f, 0.f, "%.1f");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputFloat("Y##tp", &custom_y, 0.f, 0.f, "%.1f");
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputFloat("Z##tp", &custom_z, 0.f, 0.f, "%.1f");
    ImGui::Spacing();

    if (Widgets::PrimaryButton("Teleport"))
        teleport.TeleportToCoords({ custom_x, custom_y, custom_z });
}

// ────────────────────────────────────────────────────────────────────────────
// TAB: CONFIG
// ────────────────────────────────────────────────────────────────────────────
void Menu::RenderConfigTab() {
    Widgets::SectionHeader("KEYBINDS");
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.65f, 1.f), "Menu Toggle");
    ImGui::SameLine(120.f);
    ImGui::TextColored(ImVec4(0.85f, 0.85f, 1.f, 1.f), "INSERT");

    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.65f, 1.f), "Aim Key");
    ImGui::SameLine(120.f);
    ImGui::TextColored(ImVec4(0.85f, 0.85f, 1.f, 1.f), "RMB");
    ImGui::Spacing();

    Widgets::SectionHeader("PROFILE");
    if (Widgets::PrimaryButton("Save Config"))
        g_config.Save();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button,
        ImVec4(0.12f, 0.12f, 0.18f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        ImVec4(0.18f, 0.18f, 0.26f, 1.f));
    if (ImGui::Button("Load Config", ImVec2(ImGui::GetContentRegionAvail().x, 32.f)))
        g_config.Load();
    ImGui::PopStyleColor(2);
    ImGui::Spacing();

    Widgets::SectionHeader("INFO");
    // status dot
    Widgets::StatusIndicator("NOX", true, "Online", "Offline");

    ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.55f, 1.f),
                       "Version: NOX v2.0");
}
