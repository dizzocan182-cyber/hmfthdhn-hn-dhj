#include "esp.h"
#include "../game/game_memory.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>
#include <cstring>

extern GameMemory g_game;

// ── Tam GTA5 silah hash tablosu ───────────────────────────────────────────────
struct WeaponEntry { uint32_t hash; const char* name; };
static constexpr WeaponEntry kWeaponTable[] = {
    { 0x1B06D571, "Pistol"         },
    { 0x22D8FE39, "Pistol .50"     },
    { 0x99AEEB3B, "SNS Pistol"     },
    { 0xBFD21232, "Heavy Pistol"   },
    { 0x678B81B1, "Vintage Pistol" },
    { 0xC1AE4519, "Flare Gun"      },
    { 0x16274B23, "SMG"            },
    { 0x2BE6766B, "Micro SMG"      },
    { 0xDB1AA450, "Assault SMG"    },
    { 0x13532244, "Combat PDW"     },
    { 0x78A97CD0, "Shotgun"        },
    { 0x1D073A89, "Sawn-Off"       },
    { 0xE284C527, "Assault Shotgun"},
    { 0x9D61E50F, "Bullpup Shotgun"},
    { 0xBFE256D4, "Carbine Rifle"  },
    { 0x83BF0278, "Assault Rifle"  },
    { 0xAF113F99, "Advanced Rifle" },
    { 0x1F92308D, "Special Carbine"},
    { 0xC472FE2F, "Sniper Rifle"   },
    { 0x05FC3C11, "Heavy Sniper"   },
    { 0x6E7DDDEC, "Marksman Rifle" },
    { 0x7F74F14A, "RPG"            },
    { 0xB1CA77B1, "Minigun"        },
    { 0x0A3D4D34, "Grenade Launcher"},
    { 0x93E220BD, "Grenade"        },
    { 0xBA45E8B8, "Sticky Bomb"    },
    { 0xFBAB5776, "Molotov"        },
    { 0x060EC506, "Knife"          },
    { 0x92A27487, "Nightstick"     },
    { 0x958A4A8F, "Baseball Bat"   },
    { 0xF9E6AA4B, "Crowbar"        },
    { 0x84BD7BFD, "Golf Club"      },
};
static constexpr int kWeaponTableSize = static_cast<int>(
    sizeof(kWeaponTable) / sizeof(kWeaponTable[0]));

static const char* GetWeaponName(uint32_t hash) {
    if (hash == 0) return "Unarmed";
    for (int i = 0; i < kWeaponTableSize; ++i) {
        if (kWeaponTable[i].hash == hash)
            return kWeaponTable[i].name;
    }
    return "Unknown";
}

// ── HP yüzdesine göre renk (yeşil → sarı → kırmızı) ─────────────────────────
static ImU32 HealthColor(float pct, int base_alpha) {
    pct = std::clamp(pct, 0.f, 1.f);
    int r, g;
    if (pct > 0.5f) {
        // yeşil → sarı
        float t = (1.f - pct) * 2.f;
        r = static_cast<int>(255 * t);
        g = 255;
    } else {
        // sarı → kırmızı
        float t = pct * 2.f;
        r = 255;
        g = static_cast<int>(255 * t);
    }
    return IM_COL32(r, g, 0, base_alpha);
}

// ── Update — mutex korumalı ───────────────────────────────────────────────────
void Esp::Update(const std::vector<Ped>& players, const ViewMatrix& vm,
                 float sw, float sh) {
    std::vector<EspData> new_data;
    new_data.reserve(players.size());

    if (!m_config.enabled) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_render_data.clear();
        return;
    }

    Ped local = g_game.ReadLocalPlayer();

    for (const auto& p : players) {
        if (!p.is_valid || !p.is_alive) continue;
        if (p.address == local.address) continue;
        if (m_config.team_check && p.team == local.team && p.team != 0) continue;

        float dist = local.position.distance_to(p.position);
        if (dist > m_config.max_distance) continue;

        EspData data{};
        data.ped      = p;
        data.distance = dist;

        // ── HP / Armor yüzdesi ────────────────────────────────────────────
        data.health_pct = std::clamp(p.health / 200.f, 0.f, 1.f);
        data.armor_pct  = std::clamp(p.armor  / 100.f, 0.f, 1.f);

        // ── Distance fade ─────────────────────────────────────────────────
        // max_distance'ın %60'ından sonra yavaşça saydam ol
        if (m_config.distance_fade) {
            float fade_start = m_config.max_distance * 0.60f;
            if (dist > fade_start) {
                float t = (dist - fade_start) / (m_config.max_distance - fade_start);
                data.alpha_scale = 1.f - std::clamp(t, 0.f, 1.f) * 0.6f; // min %40 alpha
            } else {
                data.alpha_scale = 1.f;
            }
        } else {
            data.alpha_scale = 1.f;
        }

        // ── Bounding box ──────────────────────────────────────────────────
        float bx, by, bw, bh;
        data.box_on_screen = CalculateBoundingBox(p.address, vm, sw, sh,
                                                   bx, by, bw, bh);
        data.box_x = bx;
        data.box_y = by;
        data.box_w = bw;
        data.box_h = bh;

        if (!data.box_on_screen) continue;

        // ── İsim ─────────────────────────────────────────────────────────
        strncpy_s(data.display_name, p.player_name, sizeof(data.display_name) - 1);

        // ── Silah adı ─────────────────────────────────────────────────────
        const char* wname = GetWeaponName(p.current_weapon_hash);
        strncpy_s(data.weapon_name, wname, sizeof(data.weapon_name) - 1);

        // ── Kemikler ──────────────────────────────────────────────────────
        // GTA5 bone index'leri:
        //  0=head, 1=neck, 2=spine2, 3=spine1, 4=pelvis
        //  5=L_upperarm, 6=R_upperarm, 7=L_hand, 8=R_hand
        //  9=L_foot, 10=R_foot  (BoneData[9] -> indices 0..10 map)
        static constexpr int kBoneIDs[9] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        for (int bi = 0; bi < 9; ++bi) {
            vec3 bpos = g_game.GetBonePosition(p.address, kBoneIDs[bi]);
            vec2 sc{};
            bool ok = g_game.WorldToScreen(vm, bpos, sc, sw, sh);
            data.bones[bi].on_screen = ok;
            data.bones[bi].screen_x  = sc.x;
            data.bones[bi].screen_y  = sc.y;
        }

        new_data.push_back(data);
    }

    // logic thread tamamen bitince render thread'e ver
    std::lock_guard<std::mutex> lock(m_mutex);
    m_render_data = std::move(new_data);
}

// ── Gerçek bounding box — tüm kemiklerin min/max'ı ───────────────────────────
bool Esp::CalculateBoundingBox(uintptr_t ped_addr, const ViewMatrix& vm,
                                float sw, float sh,
                                float& out_x, float& out_y,
                                float& out_w, float& out_h) {
    // Kullanılan kemikler: head(0), pelvis(4), L_hand(7), R_hand(8)
    // + feet için ped pozisyonu
    static constexpr int kBoxBones[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };

    float min_x =  1e9f, min_y =  1e9f;
    float max_x = -1e9f, max_y = -1e9f;
    int   valid  = 0;

    for (int b : kBoxBones) {
        vec3 bpos = g_game.GetBonePosition(ped_addr, b);
        if (bpos.x == 0.f && bpos.y == 0.f && bpos.z == 0.f) continue;

        vec2 sc{};
        if (!g_game.WorldToScreen(vm, bpos, sc, sw, sh)) continue;

        min_x = std::min(min_x, sc.x);
        min_y = std::min(min_y, sc.y);
        max_x = std::max(max_x, sc.x);
        max_y = std::max(max_y, sc.y);
        ++valid;
    }

    if (valid < 2) return false;

    // biraz padding
    float pad_x = (max_x - min_x) * 0.12f;
    float pad_y = (max_y - min_y) * 0.06f;

    out_x = min_x - pad_x;
    out_y = min_y - pad_y;
    out_w = (max_x - min_x) + pad_x * 2.f;
    out_h = (max_y - min_y) + pad_y * 2.f;

    // ekran sınırı kontrolü — çözünürlüğe göre
    bool on = (out_x + out_w > 0.f && out_x < sw &&
               out_y + out_h > 0.f && out_y < sh);
    return on;
}
