#pragma once
#include <cstdint>

namespace Offsets {
    struct VersionOffsets {
        uintptr_t world;
        uintptr_t replay;
        uintptr_t local_ped;
        uintptr_t camera;
    };

    inline constexpr VersionOffsets kVersions[] = {
        { 0x25ec580, 0x1fb0418, 0x2058ba0, 0x2059a48 },
        { 0x25d7108, 0x1f9a9d8, 0x20431c0, 0x2043df8 },
        { 0x25c15b0, 0x1f85458, 0x202dc50, 0x202e878 },
        { 0x25b14b0, 0x1fbd4f0, 0x201dba0, 0x201e7d0 },
        { 0x2593320, 0x1f58b58, 0x20019e0, 0x20025b8 },
        { 0x257bea0, 0x1f42068, 0x1feaac0, 0x1feb968 },
        { 0x254d448, 0x1f5b820, 0x1fbc100, 0x1fbccd8 },
        { 0x26684d8, 0x20304c8, 0x20d8c90, 0x20d9868 },
    };

    namespace Patterns {
        inline constexpr const char* kEntityListPattern = "4c 8b 94 c7 ? ? ? ? eb ? 4d 8b d6 41 8a 82";
        inline constexpr const char* kPlayerInfoPattern = "74 ? 83 b9 ? ? ? ? ? 75 ? 45 84 f6";

        // World pointer: mov rax, [rip+disp32]
        inline constexpr const char* kWorldPattern_v1 = "48 8b 05 ? ? ? ? 48 8b 48 08 48 85 c9 74 ? 48 8b 01";
        inline constexpr const char* kWorldPattern_v2 = "48 8b 05 ? ? ? ? 48 8b 40 10 48 85 c0";
        inline constexpr const char* kWorldPattern_v3 = "4c 8b 0d ? ? ? ? 48 8b 54 24";
        inline constexpr const char* kWorldPattern_v4 = "48 8b 3d ? ? ? ? 48 85 ff 74 ? 48 8b 47";

        // Camera pointer: lea rcx, [rip+disp32]
        inline constexpr const char* kCameraPattern_v1 = "48 8d 0d ? ? ? ? e8 ? ? ? ? 48 8b 05";
        inline constexpr const char* kCameraPattern_v2 = "48 8b 0d ? ? ? ? 48 89 44 24";
    }

    namespace Common {
        constexpr int kEntityListStride     = 0x10a8;
        constexpr int kPlayerInfoStride     = 0xe8;
        constexpr int kTeamOffset           = 0x280;
        constexpr int kPedHealthOffset      = 0x10b8;
        constexpr int kPedArmorOffset       = 0x410;
        constexpr int kVelocityOffset       = 0xd10;
        constexpr int kWeaponManagerOffset  = 0x145c;
        constexpr int kBoneArrayBase        = 0x1d0;
        constexpr int kBoneStride           = 0x30;
    }

    // ── CWeaponManager silent aim offset'leri ─────────────────────────────────
    // GTA5'te CWeaponManager struct içinde silahın baktığı world koordinatı.
    // Hangi offset'in doğru olduğunu ReClass.NET ile doğrula:
    //   1. GTAProcess'e attach ol
    //   2. local_ped + kWeaponManagerOffset adresini aç
    //   3. +0x60'ta current_weapon_hash görüyorsan doğru struct'tasın
    //   4. Çevresindeki vec3'leri kontrol et — oyun içinde nişan alırken
    //      değişen vec3 = m_aimingAt
    //
    // Aktif offset: kWMAimingAt_v1 — çalışmıyorsa v2/v3 dene
    namespace WeaponMgr {
        constexpr int kCurrentWeaponHash = 0x60;   // uint32 — zaten doğrulandı

        // m_aimingFrom: silahın çıktığı nokta (muzzle world pos)
        constexpr int kAimingFrom_v1     = 0x120;
        constexpr int kAimingFrom_v2     = 0x198;

        // m_aimingAt: merminin gittiği hedef nokta — BİZİM OVERRIDE ETTİĞİMİZ YER
        constexpr int kAimingAt_v1       = 0x12C;  // b2060–b2372 civarı
        constexpr int kAimingAt_v2       = 0x1A4;  // b2545+ civarı
        constexpr int kAimingAt_v3       = 0x130;  // bazı FiveM build'leri

        // Aktif olarak kullanılan offset — çalışmıyorsa bunu değiştir
        constexpr int kAimingAt          = kAimingAt_v1;
        constexpr int kAimingFrom        = kAimingFrom_v1;
    }

    constexpr uintptr_t kViewMatrixOffset  = 0x24c;
    constexpr uintptr_t kEntityListStart   = 0x10a8;
    constexpr uintptr_t kEntityListStride  = 0x10;

    // Weapon hash lookup table (index = category for esp display)
    inline constexpr uint32_t kWeaponHashes[] = {
        0x00000000, // 0: Unknown/None
        0x00000000, // 1: Melee (unarmed)
        0x1B06D571, // 2: Pistol (WEAPON_PISTOL)
        0x16274B23, // 3: SMG (WEAPON_SMG)
        0x78A97CD0, // 4: Shotgun (WEAPON_SHOTGUN)
        0xBFE256D4, // 5: Rifle (WEAPON_CARBINERIFLE)
        0xC472FE2F, // 6: Sniper (WEAPON_SNIPERRIFLE)
        0x7F74F14A, // 7: Heavy (WEAPON_RPG)
        0x93E220BD, // 8: Throwables (WEAPON_GRENADE)
        0xA0973D5E, // 9: Misc
    };
}
