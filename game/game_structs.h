#pragma once
#include "../core/types.h"

struct Ped {
    uintptr_t address   = 0;
    float health        = 0.f;
    float armor         = 0.f;
    vec3  position{};
    vec3  velocity{};
    int   team          = 0;
    bool  is_alive      = false;
    bool  is_valid      = false;

    uint32_t model_hash = 0;

    uintptr_t weapon_manager      = 0;
    uint32_t  current_weapon_hash = 0;
    int       ammo                = 0;
    int       max_ammo            = 0;

    uintptr_t player_info = 0;
    char      player_name[64]{};
    int       player_id   = -1;
};

struct Vehicle {
    uintptr_t address   = 0;
    float health        = 0.f;
    vec3  position{};
    vec3  velocity{};
    bool  is_valid      = false;

    uint32_t model_hash = 0;
    float speed         = 0.f;

    float engine_health = 0.f;
    bool  doors_locked  = false;
};

struct BoneData {
    vec3  screen_pos{};
    bool  on_screen   = false;
    float screen_x    = 0.f;
    float screen_y    = 0.f;
};

struct EspData {
    Ped      ped{};
    BoneData bones[9]{};

    float    box_x          = 0.f;
    float    box_y          = 0.f;
    float    box_w          = 0.f;
    float    box_h          = 0.f;
    bool     box_on_screen  = false;

    float    distance       = 0.f;
    float    health_pct     = 0.f;
    float    armor_pct      = 0.f;   // 0..1
    float    alpha_scale    = 1.f;   // distance fade için
    char     display_name[64]{};
    char     weapon_name[64]{};
};

struct ViewMatrix {
    float m[4][4]{};
};
