#include "dx11.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

static void SetupNoxStyle();

bool InitializeImGui(ID3D11Device* device, ID3D11DeviceContext* context, HWND hwnd) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // .ini kaydetme

    // ── DPI scale ──
    float dpi = 96.f;
    HDC hdc = GetDC(NULL);
    if (hdc) { dpi = (float)GetDeviceCaps(hdc, LOGPIXELSX); ReleaseDC(NULL, hdc); }
    float scale = dpi / 96.f;
    ImFontConfig fc; fc.SizePixels = 14.f * scale;
    io.Fonts->AddFontDefault(&fc);

    SetupNoxStyle();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, context);

    return true;
}

void ShutdownImGui() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

static void SetupNoxStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    // ── geometry ─────────────────────────────────────────────────────────────
    style.WindowRounding    = 10.0f;
    style.ChildRounding     = 8.0f;
    style.FrameRounding     = 6.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.PopupRounding     = 8.0f;

    style.WindowPadding     = ImVec2(0, 0);   // manuel layout yapacağız
    style.FramePadding      = ImVec2(10, 5);
    style.ItemSpacing       = ImVec2(8, 7);
    style.ItemInnerSpacing  = ImVec2(6, 4);
    style.IndentSpacing     = 18.0f;
    style.ScrollbarSize     = 6.0f;
    style.GrabMinSize       = 10.0f;
    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;

    // ── palette ───────────────────────────────────────────────────────────────
    // bg layerları
    const ImVec4 bg0      = ImVec4(0.055f, 0.055f, 0.075f, 1.00f); // en koyu — pencere
    const ImVec4 bg1      = ImVec4(0.075f, 0.075f, 0.100f, 1.00f); // sidebar
    const ImVec4 bg2      = ImVec4(0.095f, 0.095f, 0.130f, 1.00f); // içerik alanı
    const ImVec4 bg3      = ImVec4(0.120f, 0.120f, 0.160f, 1.00f); // frame bg
    const ImVec4 bg3h     = ImVec4(0.150f, 0.150f, 0.200f, 1.00f); // hover
    const ImVec4 bg3a     = ImVec4(0.170f, 0.170f, 0.230f, 1.00f); // active

    // accent — elektrik mor/mavi
    const ImVec4 acc      = ImVec4(0.45f, 0.25f, 0.95f, 1.00f);
    const ImVec4 acc_h    = ImVec4(0.55f, 0.35f, 1.00f, 1.00f);
    const ImVec4 acc_a    = ImVec4(0.38f, 0.18f, 0.82f, 1.00f);
    const ImVec4 acc_dim  = ImVec4(0.45f, 0.25f, 0.95f, 0.20f);

    // border
    const ImVec4 border   = ImVec4(0.22f, 0.22f, 0.32f, 0.70f);
    const ImVec4 border_a = ImVec4(0.45f, 0.25f, 0.95f, 0.60f);

    // text
    const ImVec4 text     = ImVec4(0.90f, 0.90f, 0.95f, 1.00f);
    const ImVec4 text_dim = ImVec4(0.50f, 0.50f, 0.60f, 1.00f);

    c[ImGuiCol_Text]                 = text;
    c[ImGuiCol_TextDisabled]         = text_dim;
    c[ImGuiCol_WindowBg]             = bg0;
    c[ImGuiCol_ChildBg]              = bg2;
    c[ImGuiCol_PopupBg]              = bg1;
    c[ImGuiCol_Border]               = border;
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_FrameBg]              = bg3;
    c[ImGuiCol_FrameBgHovered]       = bg3h;
    c[ImGuiCol_FrameBgActive]        = bg3a;
    c[ImGuiCol_TitleBg]              = bg0;
    c[ImGuiCol_TitleBgActive]        = bg0;
    c[ImGuiCol_TitleBgCollapsed]     = bg0;
    c[ImGuiCol_MenuBarBg]            = bg1;
    c[ImGuiCol_ScrollbarBg]          = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab]        = bg3h;
    c[ImGuiCol_ScrollbarGrabHovered] = acc_dim;
    c[ImGuiCol_ScrollbarGrabActive]  = acc;
    c[ImGuiCol_CheckMark]            = acc;
    c[ImGuiCol_SliderGrab]           = acc;
    c[ImGuiCol_SliderGrabActive]     = acc_h;
    c[ImGuiCol_Button]               = bg3;
    c[ImGuiCol_ButtonHovered]        = acc_dim;
    c[ImGuiCol_ButtonActive]         = acc_a;
    c[ImGuiCol_Header]               = acc_dim;
    c[ImGuiCol_HeaderHovered]        = ImVec4(0.45f, 0.25f, 0.95f, 0.35f);
    c[ImGuiCol_HeaderActive]         = acc_a;
    c[ImGuiCol_Separator]            = border;
    c[ImGuiCol_SeparatorHovered]     = acc;
    c[ImGuiCol_SeparatorActive]      = acc_h;
    c[ImGuiCol_ResizeGrip]           = acc_dim;
    c[ImGuiCol_ResizeGripHovered]    = acc;
    c[ImGuiCol_ResizeGripActive]     = acc_h;
    c[ImGuiCol_Tab]                  = bg1;
    c[ImGuiCol_TabHovered]           = acc_dim;
    c[ImGuiCol_TabActive]            = bg3;
    c[ImGuiCol_TabUnfocused]         = bg1;
    c[ImGuiCol_TabUnfocusedActive]   = bg2;
    c[ImGuiCol_PlotLines]            = acc;
    c[ImGuiCol_PlotLinesHovered]     = acc_h;
    c[ImGuiCol_PlotHistogram]        = acc;
    c[ImGuiCol_PlotHistogramHovered] = acc_h;
    c[ImGuiCol_TextSelectedBg]       = acc_dim;
}
