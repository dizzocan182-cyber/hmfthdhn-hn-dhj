#include "teleport.h"
#include "../game/game_memory.h"
#include <cstring>

extern GameMemory g_game;

void Teleport::Initialize() {
    m_locations.clear();

    auto add = [&](const char* name, vec3 pos) {
        TeleportLocation loc{};
        strncpy_s(loc.name, name, sizeof(loc.name) - 1);
        loc.position = pos;
        m_locations.push_back(loc);
    };

    add("Legion Square",         { 148.f,    -1038.f,  29.3f });
    add("Maze Bank Tower",       { -76.f,    -818.f,   326.f });
    add("LS Airport",            { -1336.f,  -3044.f,  13.9f });
    add("Military Base",         { -2047.f,  3132.f,   32.8f });
    add("Sandy Shores",          { 1960.f,   3740.f,   32.2f });
    add("Del Perro Pier",        { -1850.f,  -1230.f,  8.6f });
    add("Vinewood Sign",         { 683.f,    1200.f,   344.8f });
    add("Mount Chiliad",         { 450.f,    5566.f,   826.f });
    add("Grove Street",          { -60.f,    -1690.f,  29.3f });
    add("Casino",                { 924.f,    47.f,     81.f });
    add("Paleto Bay",            { -178.f,   6334.f,   31.5f });
    add("Observatory",           { -440.f,   5900.f,   400.f });
}

void Teleport::TeleportToLocation(int index) {
    if (index < 0 || index >= static_cast<int>(m_locations.size())) return;
    g_game.TeleportTo(m_locations[index].position);
}

void Teleport::TeleportToCoords(vec3 pos) {
    g_game.TeleportTo(pos);
}
