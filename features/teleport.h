#pragma once
#include "../core/types.h"
#include <vector>
#include <string>

struct TeleportLocation {
    char name[64];
    vec3 position;
};

class Teleport {
public:
    void Initialize();
    void TeleportToLocation(int index);
    void TeleportToCoords(vec3 pos);

    const std::vector<TeleportLocation>& GetLocations() const { return m_locations; }
    int  GetSelectedIndex() const       { return m_selected; }
    void SetSelectedIndex(int idx)      { m_selected = idx; }

private:
    std::vector<TeleportLocation> m_locations;
    int m_selected = 0;
};
