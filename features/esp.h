#pragma once
#include "../core/types.h"
#include "../game/game_structs.h"
#include <vector>
#include <mutex>

struct EspConfig {
    bool enabled     = false;

    // box
    bool box         = true;
    bool box_filled  = false;
    bool corner_box  = true;    // L-şekilli köşe kutusu (box yerine veya beraber)

    // barlar
    bool health_bar  = true;
    bool armor_bar   = true;

    // text
    bool name        = true;
    bool distance    = true;
    bool weapon      = true;

    // görsel
    bool snaplines   = false;
    bool skeleton    = false;
    bool head_dot    = true;
    bool distance_fade = true;  // uzaktaki oyuncular daha saydam

    // renkler
    struct RGBA { int r, g, b, a; };
    RGBA box_color      = {  80, 180, 255, 220 };
    RGBA health_color   = {   0, 255,   0, 255 };  // base — runtime'da HP'ye göre değişir
    RGBA armor_color    = {  80, 180, 255, 220 };
    RGBA name_color     = { 255, 255, 255, 255 };
    RGBA distance_color = { 180, 180, 180, 200 };
    RGBA snapline_color = { 255, 255, 255, 120 };
    RGBA skeleton_color = { 255, 255, 255, 160 };

    float max_distance  = 200.f;
    bool  team_check    = true;
};

class Esp {
public:
    void Update(const std::vector<Ped>& players, const ViewMatrix& vm,
                float sw, float sh);

    EspConfig&       GetConfig()       { return m_config; }
    const EspConfig& GetConfig() const { return m_config; }

    // render thread güvenli kopya al
    std::vector<EspData> GetRenderDataCopy() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_render_data;
    }

    // eski erişim — sadece render thread'den çağrılıyorsa OK
    const std::vector<EspData>& GetRenderData() const { return m_render_data; }

private:
    EspConfig            m_config;
    std::vector<EspData> m_render_data;
    std::mutex           m_mutex;

    void ProcessEntity(const Ped& ped, const ViewMatrix& vm,
                       float sw, float sh, const Ped& local);

    // gerçek min/max bazlı bounding box
    bool CalculateBoundingBox(uintptr_t ped_addr, const ViewMatrix& vm,
                              float sw, float sh,
                              float& x, float& y, float& w, float& h);
};

extern Esp g_esp;
