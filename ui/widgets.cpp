#include "widgets.h"
#include "imgui_internal.h"
#include <Windows.h>
#include <cmath>
#include <cstring>

// ── renk sabitleri (dx11.cpp'deki palette ile eşleşmeli) ─────────────────────
static inline ImU32 AccentCol()      { return ImColor(0.45f, 0.25f, 0.95f, 1.00f); }
static inline ImU32 AccentHov()      { return ImColor(0.55f, 0.35f, 1.00f, 1.00f); }
static inline ImU32 AccentDim()      { return ImColor(0.45f, 0.25f, 0.95f, 0.25f); }
static inline ImU32 Bg3Col()         { return ImColor(0.120f, 0.120f, 0.160f, 1.00f); }
static inline ImU32 TextCol()        { return ImColor(0.90f, 0.90f, 0.95f, 1.00f); }
static inline ImU32 TextDimCol()     { return ImColor(0.50f, 0.50f, 0.60f, 1.00f); }
static inline ImU32 BorderCol()      { return ImColor(0.22f, 0.22f, 0.32f, 0.70f); }
static inline ImU32 GreenOn()        { return ImColor(0.20f, 0.85f, 0.45f, 1.00f); }
static inline ImU32 RedOff()         { return ImColor(0.75f, 0.20f, 0.25f, 1.00f); }
static inline ImU32 SidebarBg()      { return ImColor(0.075f, 0.075f, 0.100f, 1.00f); }
static inline ImU32 SidebarSelBg()   { return ImColor(0.45f, 0.25f, 0.95f, 0.18f); }
static inline ImU32 SidebarSelBar()  { return ImColor(0.45f, 0.25f, 0.95f, 1.00f); }

namespace Widgets {

// ── Toggle (iOS tarzı, glow efektli) ─────────────────────────────────────────
bool Toggle(const char* label, bool* value) {
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const float track_h   = 20.0f;
    const float track_w   = track_h * 1.9f;
    const float knob_r    = track_h * 0.38f;
    const float padding   = (track_h - knob_r * 2.f) * 0.5f;

    ImVec2 cursor = ImGui::GetCursorScreenPos();
    float avail_w = ImGui::GetContentRegionAvail().x;

    // invisible button tüm satır
    ImGui::PushID(label);
    bool clicked = ImGui::InvisibleButton("##tog", ImVec2(avail_w, track_h));
    ImGui::PopID();

    if (clicked) *value = !(*value);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // track
    ImU32 track_col = *value ? AccentCol() : Bg3Col();
    dl->AddRectFilled(cursor,
                      ImVec2(cursor.x + track_w, cursor.y + track_h),
                      track_col, track_h * 0.5f);

    // track border
    dl->AddRect(cursor,
                ImVec2(cursor.x + track_w, cursor.y + track_h),
                *value ? AccentHov() : BorderCol(),
                track_h * 0.5f, 0, 1.0f);

    // knob
    float knob_x = *value
        ? (cursor.x + track_w - padding - knob_r)
        : (cursor.x + padding + knob_r);
    float knob_y = cursor.y + track_h * 0.5f;
    dl->AddCircleFilled(ImVec2(knob_x, knob_y), knob_r, IM_COL32(255, 255, 255, 245));

    // glow when on
    if (*value)
        dl->AddCircleFilled(ImVec2(knob_x, knob_y), knob_r + 3.f,
                            ImColor(0.45f, 0.25f, 0.95f, 0.20f));

    // label — sağda hizalı değil, track'in yanında
    ImGui::SameLine(track_w + 12.0f);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - (track_h - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::TextUnformatted(label);

    return clicked;
}

// ── Hotkey ───────────────────────────────────────────────────────────────────
bool Hotkey(const char* label, int* key) {
    ImGui::PushID(label);

    const float btn_w = 90.0f;
    const float btn_h = ImGui::GetFrameHeight();

    char key_name[64] = "None";
    if (*key != 0) {
        UINT vk = static_cast<UINT>(*key);
        int  sc = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
        GetKeyNameTextA(sc << 16, key_name, sizeof(key_name));
    }

    // her hotkey widget kendi ID'si üzerinden bekliyor
    static ImGuiID s_waiting_id = 0;
    ImGuiID this_id = ImGui::GetID("##hk");
    bool waiting = (s_waiting_id == this_id);

    ImGui::TextUnformatted(label);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - btn_w + ImGui::GetCursorPosX());

    char btn_label[128];
    snprintf(btn_label, sizeof(btn_label), waiting ? "..." : key_name);

    if (waiting) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.45f, 0.25f, 0.95f, 0.40f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.35f, 1.00f, 0.55f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.38f, 0.18f, 0.82f, 0.70f));
        ImGui::PushStyleColor(ImGuiCol_Border,        ImVec4(0.45f, 0.25f, 0.95f, 0.80f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    }

    if (ImGui::Button(btn_label, ImVec2(btn_w, btn_h)))
        s_waiting_id = this_id;

    if (waiting) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);

        for (int vk = 1; vk < 256; ++vk) {
            if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON) continue;
            if (GetAsyncKeyState(vk) & 0x8000) {
                *key = (vk == VK_ESCAPE) ? 0 : vk;
                s_waiting_id = 0;
                break;
            }
        }
    }

    ImGui::PopID();
    return false;
}

// ── ColorEdit3 ───────────────────────────────────────────────────────────────
bool ColorEdit3(const char* label, int color[3]) {
    float f[3] = { color[0] / 255.f, color[1] / 255.f, color[2] / 255.f };
    bool changed = ImGui::ColorEdit3(label, f);
    if (changed) {
        color[0] = (int)(f[0] * 255.f);
        color[1] = (int)(f[1] * 255.f);
        color[2] = (int)(f[2] * 255.f);
    }
    return changed;
}

// ── ColorEdit4 ───────────────────────────────────────────────────────────────
bool ColorEdit4(const char* label, ColorRGBA& color) {
    float f[4] = { color.r / 255.f, color.g / 255.f,
                   color.b / 255.f, color.a / 255.f };
    bool changed = ImGui::ColorEdit4(label, f);
    if (changed) {
        color.r = (int)(f[0] * 255.f);
        color.g = (int)(f[1] * 255.f);
        color.b = (int)(f[2] * 255.f);
        color.a = (int)(f[3] * 255.f);
    }
    return changed;
}

// ── Section header (ince çizgi + küçük başlık) ────────────────────────────────
void SectionHeader(const char* title) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float avail = ImGui::GetContentRegionAvail().x;
    float text_w = ImGui::CalcTextSize(title).x;
    float line_y = pos.y + ImGui::GetTextLineHeight() * 0.5f;

    // sol çizgi
    dl->AddLine(ImVec2(pos.x, line_y),
                ImVec2(pos.x + 8.f, line_y),
                AccentCol(), 1.5f);

    // sağ çizgi
    float text_start = pos.x + 8.f + 6.f;
    float text_end   = text_start + text_w + 6.f;
    dl->AddLine(ImVec2(text_end, line_y),
                ImVec2(pos.x + avail, line_y),
                BorderCol(), 1.0f);

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 14.f);
    ImGui::TextColored(ImVec4(0.55f, 0.40f, 0.85f, 1.00f), title);
    ImGui::Spacing();
}

// ── Status indicator ─────────────────────────────────────────────────────────
void StatusIndicator(const char* label, bool active,
                     const char* active_text, const char* inactive_text) {
    ImDrawList* dl    = ImGui::GetWindowDrawList();
    ImVec2      pos   = ImGui::GetCursorScreenPos();
    float       th    = ImGui::GetTextLineHeight();
    float       dot_r = th * 0.28f;

    ImU32 dot_col = active ? GreenOn() : RedOff();

    // outer glow
    dl->AddCircleFilled(ImVec2(pos.x + dot_r + 2.f, pos.y + th * 0.5f),
                        dot_r + 3.f,
                        active ? ImColor(0.20f, 0.85f, 0.45f, 0.20f)
                               : ImColor(0.75f, 0.20f, 0.25f, 0.20f));
    // dot
    dl->AddCircleFilled(ImVec2(pos.x + dot_r + 2.f, pos.y + th * 0.5f),
                        dot_r, dot_col);

    ImGui::Dummy(ImVec2(dot_r * 2.f + 8.f, th));
    ImGui::SameLine();
    ImGui::Text("%s: %s", label, active ? active_text : inactive_text);
}

// ── Sidebar button ────────────────────────────────────────────────────────────
bool SidebarButton(const char* label, const char* /*icon*/,
                   bool selected, ImVec2 size) {
    ImDrawList* dl  = ImGui::GetWindowDrawList();
    ImVec2      pos = ImGui::GetCursorScreenPos();

    ImGui::PushID(label);
    bool clicked = ImGui::InvisibleButton("##sb", size);
    ImGui::PopID();

    bool hovered = ImGui::IsItemHovered();

    // bg
    if (selected)
        dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                          SidebarSelBg(), 6.f);
    else if (hovered)
        dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                          ImColor(1.f, 1.f, 1.f, 0.04f), 6.f);

    // sol accent bar
    if (selected)
        dl->AddRectFilled(ImVec2(pos.x, pos.y + 4.f),
                          ImVec2(pos.x + 3.f, pos.y + size.y - 4.f),
                          SidebarSelBar(), 2.f);

    // text
    ImVec2 text_sz = ImGui::CalcTextSize(label);
    ImVec2 text_pos = ImVec2(
        pos.x + 18.f,
        pos.y + (size.y - text_sz.y) * 0.5f
    );

    ImU32 text_col = selected ? AccentHov()
                   : hovered  ? TextCol()
                              : TextDimCol();
    dl->AddText(text_pos, text_col, label);

    return clicked;
}

// ── Primary (accent) button ───────────────────────────────────────────────────
bool PrimaryButton(const char* label) {
    float w = ImGui::GetContentRegionAvail().x;
    float h = 32.f;

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::PushID(label);
    bool clicked = ImGui::InvisibleButton("##pb", ImVec2(w, h));
    ImGui::PopID();

    bool hov = ImGui::IsItemHovered();
    bool act = ImGui::IsItemActive();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 bg = act ? ImColor(0.38f, 0.18f, 0.82f, 1.00f)
             : hov ? ImColor(0.55f, 0.35f, 1.00f, 1.00f)
                   : AccentCol();

    dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), bg, 7.f);
    dl->AddRect      (pos, ImVec2(pos.x + w, pos.y + h),
                      ImColor(0.65f, 0.50f, 1.00f, 0.40f), 7.f, 0, 1.f);

    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(pos.x + (w - ts.x) * 0.5f,
                       pos.y + (h - ts.y) * 0.5f),
                IM_COL32(255, 255, 255, 255), label);

    return clicked;
}

// ── List item (teleport lokasyonları için) ───────────────────────────────────
bool ListItem(const char* label, bool selected) {
    float w = ImGui::GetContentRegionAvail().x;
    float h = 28.f;
    ImVec2 pos = ImGui::GetCursorScreenPos();

    ImGui::PushID(label);
    bool clicked = ImGui::InvisibleButton("##li", ImVec2(w, h));
    ImGui::PopID();

    bool hov = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (selected)
        dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h),
                          SidebarSelBg(), 5.f);
    else if (hov)
        dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h),
                          ImColor(1.f, 1.f, 1.f, 0.04f), 5.f);

    // küçük bullet
    float bx = pos.x + 10.f;
    float by = pos.y + h * 0.5f;
    dl->AddCircleFilled(ImVec2(bx, by), 3.f,
                        selected ? AccentCol() : TextDimCol());

    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(bx + 10.f, pos.y + (h - ts.y) * 0.5f),
                selected ? AccentHov() : hov ? TextCol() : TextDimCol(),
                label);

    return clicked;
}

} // namespace Widgets
