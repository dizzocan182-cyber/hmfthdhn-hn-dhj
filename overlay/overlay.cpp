#include "overlay.h"
#include "../core/globals.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include "../ui/menu.h"
#include "../features/esp.h"
#include <dwmapi.h>
#include <cstring>

// imgui_impl_win32.h'da #if 0 içinde — manuel declare gerekiyor
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

Overlay g_overlay;

LRESULT CALLBACK Overlay::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_overlay.m_device && wParam != SIZE_MINIMIZED) {
            g_overlay.CleanupRenderTarget();
            g_overlay.m_swap_chain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            g_overlay.CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

bool Overlay::Initialize() {
    if (!CreateOverlayWindow()) return false;
    if (!CreateDevice()) return false;
    if (!CreateRenderTarget()) return false;
    return true;
}

bool Overlay::CreateOverlayWindow() {
    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "dx_overlay_wndclass";

    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "Window class registration failed!", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    HWND target_window = g.game_window;
    RECT target_rect{};
    int win_x = 0, win_y = 0;
    if (target_window && GetWindowRect(target_window, &target_rect)) {
        m_width  = target_rect.right  - target_rect.left;   // artık physical (DPI-aware olduk)
        m_height = target_rect.bottom - target_rect.top;
        win_x = target_rect.left;
        win_y = target_rect.top;
    } else {
        m_width = 1920; m_height = 1080;
    }
    m_hwnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        wc.lpszClassName, "NOX Overlay",
        WS_POPUP,
        win_x, win_y,           // ← game window'a kilit
        m_width, m_height,
        NULL, NULL, wc.hInstance, NULL);
    if (!m_hwnd) {
        MessageBoxA(NULL, "Window creation failed!", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    if (!SetLayeredWindowAttributes(m_hwnd, RGB(0, 0, 0), 255, LWA_ALPHA)) {
        MessageBoxA(NULL, "Layered window attributes setup failed!", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hwnd);

    return true;
}

bool Overlay::CreateDevice() {
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = m_width;
    sd.BufferDesc.Height = m_height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL feature_levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL obtained_level;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        0,
        feature_levels,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &m_swap_chain,
        &m_device,
        &obtained_level,
        &m_context
    );

    if (FAILED(hr)) {
        MessageBoxA(NULL, "Direct3D device and swap chain creation failed!", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

bool Overlay::CreateRenderTarget() {
    ID3D11Texture2D* back_buffer = nullptr;
    HRESULT hr = m_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    if (FAILED(hr)) {
        MessageBoxA(NULL, "Failed to retrieve back buffer!", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    hr = m_device->CreateRenderTargetView(back_buffer, NULL, &m_render_target);
    back_buffer->Release();
    if (FAILED(hr)) {
        MessageBoxA(NULL, "Render target view creation failed!", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

void Overlay::CleanupRenderTarget() {
    if (m_render_target) {
        m_render_target->Release();
        m_render_target = nullptr;
    }
}

void Overlay::Run() {
    m_running = true;

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(m_hwnd, &margins);

    MSG msg{};
    while (m_running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                m_running = false;
                break;
            }
        }
        // ── game window'u takip et ──
        if (g.game_window) {
            RECT r{};
            if (GetWindowRect(g.game_window, &r)) {
                int w = r.right - r.left, h = r.bottom - r.top;
                if (w != m_width || h != m_height || r.left != m_last_x || r.top != m_last_y) {
                    m_width = w; m_height = h; m_last_x = r.left; m_last_y = r.top;
                    MoveWindow(m_hwnd, r.left, r.top, w, h, TRUE);
                    if (m_swap_chain) {
                        CleanupRenderTarget();
                        m_swap_chain->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
                        CreateRenderTarget();
                    }
                }
            }
        }
        if (!m_running) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g.menu_open && g_menu) {
            g_menu->Render();
        }

        // ── HUD — sol üst köşe watermark ─────────────────────────────────────
        if (g.session.show_hud) {
            ImDrawList* dl = ImGui::GetBackgroundDrawList();

            // ── saat al ──────────────────────────────────────────────────────
            SYSTEMTIME st{};
            GetLocalTime(&st);
            char time_buf[16];
            snprintf(time_buf, sizeof(time_buf), "%02d:%02d", st.wHour, st.wMinute);

            // ── içerik string'leri ────────────────────────────────────────────
            const char* logo     = "NOX";
            const char* username = g.session.username;
            const char* tier     = g.session.hwid_tag;
            const char* ver      = g.session.version;
            const char* time_str = time_buf;

            // ── boyutlar ─────────────────────────────────────────────────────
            const float MARGIN   = 12.f;
            const float PAD_X    = 10.f;
            const float PAD_Y    =  6.f;
            const float SEP_W    =  1.f;
            const float SEP_GAP  =  8.f;
            const float FONT_SZ  = 13.f;  // default font size
            const float LOGO_SZ  = 15.f;

            ImVec2 sz_logo  = ImGui::CalcTextSize(logo);
            ImVec2 sz_user  = ImGui::CalcTextSize(username);
            ImVec2 sz_tier  = ImGui::CalcTextSize(tier);
            ImVec2 sz_time  = ImGui::CalcTextSize(time_str);
            ImVec2 sz_ver   = ImGui::CalcTextSize(ver);

            // toplam genişlik: logo | sep | user | sep | tier | sep | time | sep | ver
            float total_w = PAD_X
                + sz_logo.x + SEP_GAP + SEP_W + SEP_GAP
                + sz_user.x + SEP_GAP + SEP_W + SEP_GAP
                + sz_tier.x + SEP_GAP + SEP_W + SEP_GAP
                + sz_time.x + SEP_GAP + SEP_W + SEP_GAP
                + sz_ver.x  + PAD_X;

            float row_h    = sz_user.y;
            float total_h  = PAD_Y * 2.f + row_h;

            ImVec2 tl = ImVec2(MARGIN, MARGIN);
            ImVec2 br = ImVec2(tl.x + total_w, tl.y + total_h);

            // ── arka plan — pill şeklinde yuvarlatılmış ───────────────────────
            dl->AddRectFilled(tl, br,
                IM_COL32(8, 8, 14, 210), total_h * 0.5f);

            // sol accent bar
            dl->AddRectFilled(
                ImVec2(tl.x, tl.y + 4.f),
                ImVec2(tl.x + 3.f, br.y - 4.f),
                IM_COL32(115, 64, 242, 255), 2.f);

            // dış çerçeve
            dl->AddRect(tl, br,
                IM_COL32(115, 64, 242, 100), total_h * 0.5f, 0, 1.f);

            // ── renkler ───────────────────────────────────────────────────────
            ImU32 c_logo   = IM_COL32(140,  90, 255, 255);
            ImU32 c_user   = IM_COL32(230, 230, 240, 255);
            ImU32 c_tier   = IM_COL32( 80, 180, 255, 255);   // mavi — PRO/FREE
            ImU32 c_time   = IM_COL32(200, 200, 210, 200);
            ImU32 c_ver    = IM_COL32(120, 120, 140, 180);
            ImU32 c_sep    = IM_COL32( 80,  80, 100, 160);

            // tier rengini tipine göre ayarla
            if (strcmp(tier, "LIFETIME") == 0)
                c_tier = IM_COL32(255, 180,  60, 255);  // altın
            else if (strcmp(tier, "PRO") == 0)
                c_tier = IM_COL32( 80, 200, 120, 255);  // yeşil
            else
                c_tier = IM_COL32(160, 160, 180, 200);  // gri — FREE

            // ── elemanları sırayla yerleştir ──────────────────────────────────
            float cx = tl.x + PAD_X;
            float cy = tl.y + PAD_Y;

            // logo
            dl->AddText(ImGui::GetFont(), LOGO_SZ,
                ImVec2(cx, cy + (row_h - LOGO_SZ) * 0.5f + 1.f),
                c_logo, logo);
            cx += sz_logo.x + SEP_GAP;

            // separator
            dl->AddLine(ImVec2(cx, tl.y + 5.f), ImVec2(cx, br.y - 5.f), c_sep, SEP_W);
            cx += SEP_W + SEP_GAP;

            // kullanıcı adı
            dl->AddText(ImVec2(cx, cy), c_user, username);
            cx += sz_user.x + SEP_GAP;

            // separator
            dl->AddLine(ImVec2(cx, tl.y + 5.f), ImVec2(cx, br.y - 5.f), c_sep, SEP_W);
            cx += SEP_W + SEP_GAP;

            // tier
            dl->AddText(ImVec2(cx, cy), c_tier, tier);
            cx += sz_tier.x + SEP_GAP;

            // separator
            dl->AddLine(ImVec2(cx, tl.y + 5.f), ImVec2(cx, br.y - 5.f), c_sep, SEP_W);
            cx += SEP_W + SEP_GAP;

            // saat
            dl->AddText(ImVec2(cx, cy), c_time, time_str);
            cx += sz_time.x + SEP_GAP;

            // separator
            dl->AddLine(ImVec2(cx, tl.y + 5.f), ImVec2(cx, br.y - 5.f), c_sep, SEP_W);
            cx += SEP_W + SEP_GAP;

            // versiyon
            dl->AddText(ImVec2(cx, cy), c_ver, ver);
        }

        // ── ESP render ────────────────────────────────────────────────────────
        if (g_esp.GetConfig().enabled) {
            ImDrawList* dl       = ImGui::GetBackgroundDrawList();
            const EspConfig& cfg = g_esp.GetConfig();
            float sw             = static_cast<float>(m_width);
            float sh             = static_cast<float>(m_height);

            // mutex korumalı kopya al
            auto esp_data = g_esp.GetRenderDataCopy();

            for (const auto& e : esp_data) {
                if (!e.box_on_screen) continue;

                float bx = e.box_x, by = e.box_y;
                float bw = e.box_w, bh = e.box_h;

                // alpha_scale — distance fade
                float as = e.alpha_scale;
                auto  A  = [&](int base) -> int {
                    return static_cast<int>(base * as);
                };

                // ── Corner box ────────────────────────────────────────────────
                if (cfg.corner_box) {
                    ImU32 cc = IM_COL32(cfg.box_color.r,
                                        cfg.box_color.g,
                                        cfg.box_color.b,
                                        A(cfg.box_color.a));
                    float cs = std::min(bw, bh) * 0.22f; // köşe uzunluğu
                    float lw = 1.8f;

                    // sol üst
                    dl->AddLine(ImVec2(bx,      by),      ImVec2(bx+cs,   by),      cc, lw);
                    dl->AddLine(ImVec2(bx,      by),      ImVec2(bx,      by+cs),   cc, lw);
                    // sağ üst
                    dl->AddLine(ImVec2(bx+bw,   by),      ImVec2(bx+bw-cs,by),      cc, lw);
                    dl->AddLine(ImVec2(bx+bw,   by),      ImVec2(bx+bw,   by+cs),   cc, lw);
                    // sol alt
                    dl->AddLine(ImVec2(bx,      by+bh),   ImVec2(bx+cs,   by+bh),   cc, lw);
                    dl->AddLine(ImVec2(bx,      by+bh),   ImVec2(bx,      by+bh-cs),cc, lw);
                    // sağ alt
                    dl->AddLine(ImVec2(bx+bw,   by+bh),   ImVec2(bx+bw-cs,by+bh),   cc, lw);
                    dl->AddLine(ImVec2(bx+bw,   by+bh),   ImVec2(bx+bw,   by+bh-cs),cc, lw);
                }

                // ── Tam kutu (corner_box açıksa arka plan için ince) ──────────
                if (cfg.box && !cfg.corner_box) {
                    ImU32 bc = IM_COL32(cfg.box_color.r,
                                        cfg.box_color.g,
                                        cfg.box_color.b,
                                        A(cfg.box_color.a));
                    dl->AddRect(ImVec2(bx, by), ImVec2(bx+bw, by+bh), bc, 0.f, 0, 1.5f);
                }

                // ── Filled box ────────────────────────────────────────────────
                if (cfg.box_filled) {
                    ImU32 fc = IM_COL32(cfg.box_color.r,
                                        cfg.box_color.g,
                                        cfg.box_color.b,
                                        A(30));
                    dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx+bw, by+bh), fc, 2.f);
                }

                // ── Health bar (sol kenar, HP'ye göre renk) ───────────────────
                if (cfg.health_bar && bh > 0.f) {
                    constexpr float BAR_W = 4.f;
                    constexpr float GAP   = 3.f;
                    float bar_x  = bx - BAR_W - GAP;
                    float bar_h  = bh * e.health_pct;

                    // arka plan
                    dl->AddRectFilled(ImVec2(bar_x,        by),
                                      ImVec2(bar_x+BAR_W,  by+bh),
                                      IM_COL32(0, 0, 0, A(160)), 2.f);

                    // yeşil→sarı→kırmızı
                    float hp = e.health_pct;
                    int   hr, hg;
                    if (hp > 0.5f) { hr = (int)(255*(1.f-hp)*2.f); hg = 255; }
                    else           { hr = 255; hg = (int)(255*hp*2.f); }
                    ImU32 hc = IM_COL32(hr, hg, 0, A(230));

                    dl->AddRectFilled(ImVec2(bar_x,        by+bh-bar_h),
                                      ImVec2(bar_x+BAR_W,  by+bh),
                                      hc, 2.f);

                    // ince beyaz kenar
                    dl->AddRect(ImVec2(bar_x, by),
                                ImVec2(bar_x+BAR_W, by+bh),
                                IM_COL32(255,255,255, A(40)), 2.f, 0, 0.5f);
                }

                // ── Armor bar (health barın solunda) ──────────────────────────
                if (cfg.armor_bar && bh > 0.f && e.armor_pct > 0.f) {
                    constexpr float HBAR_W = 4.f;
                    constexpr float ABAR_W = 3.f;
                    constexpr float GAP    = 3.f;
                    float bar_x  = bx - HBAR_W - GAP - ABAR_W - 2.f;
                    float bar_h  = bh * e.armor_pct;

                    dl->AddRectFilled(ImVec2(bar_x,       by),
                                      ImVec2(bar_x+ABAR_W,by+bh),
                                      IM_COL32(0, 0, 0, A(160)), 2.f);

                    dl->AddRectFilled(ImVec2(bar_x,       by+bh-bar_h),
                                      ImVec2(bar_x+ABAR_W,by+bh),
                                      IM_COL32(cfg.armor_color.r,
                                               cfg.armor_color.g,
                                               cfg.armor_color.b,
                                               A(200)), 2.f);

                    dl->AddRect(ImVec2(bar_x, by),
                                ImVec2(bar_x+ABAR_W, by+bh),
                                IM_COL32(255,255,255, A(40)), 2.f, 0, 0.5f);
                }

                // ── Head dot ──────────────────────────────────────────────────
                if (cfg.head_dot && e.bones[0].on_screen) {
                    dl->AddCircleFilled(ImVec2(e.bones[0].screen_x,
                                               e.bones[0].screen_y),
                                        3.5f,
                                        IM_COL32(255, 255, 255, A(240)));
                    dl->AddCircle(ImVec2(e.bones[0].screen_x,
                                         e.bones[0].screen_y),
                                  3.5f,
                                  IM_COL32(0, 0, 0, A(180)), 12, 1.f);
                }

                // ── İsim ─────────────────────────────────────────────────────
                if (cfg.name && e.display_name[0]) {
                    ImU32 nc = IM_COL32(cfg.name_color.r,
                                        cfg.name_color.g,
                                        cfg.name_color.b,
                                        A(cfg.name_color.a));
                    ImVec2 ts = ImGui::CalcTextSize(e.display_name);
                    dl->AddText(ImVec2(bx + bw*0.5f - ts.x*0.5f, by - 14.f),
                                nc, e.display_name);
                }

                // ── Mesafe ────────────────────────────────────────────────────
                if (cfg.distance) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.0fm", e.distance);
                    ImU32 dc = IM_COL32(cfg.distance_color.r,
                                        cfg.distance_color.g,
                                        cfg.distance_color.b,
                                        A(cfg.distance_color.a));
                    ImVec2 ts = ImGui::CalcTextSize(buf);
                    dl->AddText(ImVec2(bx + bw*0.5f - ts.x*0.5f, by+bh+3.f), dc, buf);
                }

                // ── Silah ─────────────────────────────────────────────────────
                if (cfg.weapon && e.weapon_name[0]) {
                    ImU32 wc = IM_COL32(cfg.name_color.r,
                                        cfg.name_color.g,
                                        cfg.name_color.b,
                                        A(160));
                    ImVec2 ts = ImGui::CalcTextSize(e.weapon_name);
                    float  wy = by + bh + (cfg.distance ? 16.f : 3.f);
                    dl->AddText(ImVec2(bx + bw*0.5f - ts.x*0.5f, wy), wc, e.weapon_name);
                }

                // ── Snaplines ─────────────────────────────────────────────────
                if (cfg.snaplines) {
                    ImU32 sc = IM_COL32(cfg.snapline_color.r,
                                        cfg.snapline_color.g,
                                        cfg.snapline_color.b,
                                        A(cfg.snapline_color.a));
                    dl->AddLine(ImVec2(sw*0.5f, sh),
                                ImVec2(bx+bw*0.5f, by+bh),
                                sc, 1.f);
                }

                // ── Skeleton — simetrik, 14 bağlantı ─────────────────────────
                if (cfg.skeleton) {
                    ImU32 skc = IM_COL32(cfg.skeleton_color.r,
                                         cfg.skeleton_color.g,
                                         cfg.skeleton_color.b,
                                         A(cfg.skeleton_color.a));
                    // bones[]: 0=head, 1=neck, 2=spine2, 3=spine1, 4=pelvis,
                    //          5=L_upper, 6=R_upper, 7=L_hand, 8=R_hand
                    static constexpr int kPairs[][2] = {
                        {0, 1},  // head  → neck
                        {1, 2},  // neck  → spine2
                        {2, 3},  // spine2→ spine1
                        {3, 4},  // spine1→ pelvis
                        {1, 5},  // neck  → L_upperarm
                        {1, 6},  // neck  → R_upperarm
                        {5, 7},  // L_upperarm → L_hand
                        {6, 8},  // R_upperarm → R_hand
                    };
                    for (auto& pr : kPairs) {
                        if (e.bones[pr[0]].on_screen &&
                            e.bones[pr[1]].on_screen) {
                            dl->AddLine(
                                ImVec2(e.bones[pr[0]].screen_x,
                                       e.bones[pr[0]].screen_y),
                                ImVec2(e.bones[pr[1]].screen_x,
                                       e.bones[pr[1]].screen_y),
                                skc, 1.2f);
                        }
                    }
                }
            }
        }

        ImGui::Render();
        const float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
        m_context->OMSetRenderTargets(1, &m_render_target, NULL);
        m_context->ClearRenderTargetView(m_render_target, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        m_swap_chain->Present(1, 0);
    }
}

void Overlay::Shutdown() {
    CleanupRenderTarget();

    if (m_swap_chain) {
        m_swap_chain->Release();
        m_swap_chain = nullptr;
    }
    if (m_context) {
        m_context->Release();
        m_context = nullptr;
    }
    if (m_device) {
        m_device->Release();
        m_device = nullptr;
    }
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
    UnregisterClassA("dx_overlay_wndclass", GetModuleHandleA(NULL));
}
