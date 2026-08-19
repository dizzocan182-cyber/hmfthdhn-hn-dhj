#pragma once
#include "imgui.h"

namespace Widgets {

    struct ColorRGBA { int r, g, b, a; };

    // ── Temel kontroller ──────────────────────────────────────────────────────
    bool Toggle(const char* label, bool* value);
    bool Hotkey(const char* label, int* key);
    bool ColorEdit3(const char* label, int color[3]);
    bool ColorEdit4(const char* label, ColorRGBA& color);

    // ── Section başlığı (separator + başlık metni) ────────────────────────────
    void SectionHeader(const char* title);

    // ── Durum göstergesi ──────────────────────────────────────────────────────
    void StatusIndicator(const char* label, bool active,
                         const char* active_text, const char* inactive_text);

    // ── Sidebar navigasyon butonu ─────────────────────────────────────────────
    // selected: bu buton seçili mi
    // Döner: tıklandıysa true
    bool SidebarButton(const char* label, const char* icon,
                       bool selected, ImVec2 size);

    // ── Tam genişlik buton (accent renkli) ────────────────────────────────────
    bool PrimaryButton(const char* label);

    // ── Noktalı liste item (teleport vs.) ────────────────────────────────────
    bool ListItem(const char* label, bool selected);

} // namespace Widgets
