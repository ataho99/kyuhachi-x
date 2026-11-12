#pragma once
/* screens/bgfx.h -- kx-bg: settings > screen > background effect page (sub-page of S_SCREEN, like the phosphor page).
 * Global only (g.bgfx / g.bgspd / g.bgbri), saved with the global settings; the effect itself is drawn by bgx::draw(). */
static inline const char * bgx_fxkey(int i) {
    static const char * const K5[5] = { "bg_fx0", "bg_fx1", "bg_fx2", "bg_fx3", "bg_fx4" }; return K5[i < 0 ? 0 : i > 4 ? 4 : i];
}
inline void Ui::d_bgfx(Canvas & c) {
    titlebar(c, phr_("bg_title"));
    rbox(c, 16, 24, 288, 220, WH, phr_("bg_title"));
    const bool off = g.bgfx == 0;
    std::vector<std::string> n = { phr_("bg_fx"), phr_("bg_spd"), phr_("bg_bri") }; std::vector<bool> dm = { false, off, off };
    items(c, 40, 44, 256, n, sel, 16, &dm);
    rbox(c, 320, 24, 304, 220, WH, phr_("bg_choices"));
    const std::string sv[3] = { phr_(bgx_fxkey(g.bgfx)), phr_(g.bgspd == 0 ? "bg_sp0" : g.bgspd == 1 ? "bg_sp1" : "bg_sp2"),
        phr_(g.bgbri == 0 ? "bg_br0" : g.bgbri == 1 ? "bg_br1" : "bg_br2") };
    for (int i = 0; i < 3; i++) text(c, 344, 44 + i * 16, std::string("< ") + sv[i] + " >", (i > 0 && off) ? GY : (sel == i ? CY : WH), K);
    rbox(c, 16, 248, 608, 82, WH, phr_("bg_prev"));
    text(c, 40, 270, phr_("bg_note"), WH, K);
    static const char * const dk[3] = { "desc_bg_fx", "desc_bg_spd", "desc_bg_bri" };
    desc_mq(c, 324, 100, dkey(phr_(dk[sel >= 0 && sel < 3 ? sel : 0])));
    helpline(c, phr_("bg_help"));
}
inline Act Ui::k_bgfx(UKey k, Act & a) {
    if (k == UK_LEFT || k == UK_RIGHT) {
        const int d = k == UK_RIGHT ? 1 : -1;
        if (sel == 0) g.bgfx = clampi(g.bgfx + d, 0, 4);
        else if (g.bgfx > 0 && sel == 1) g.bgspd = clampi(g.bgspd + d, 0, 2);
        else if (g.bgfx > 0 && sel == 2) g.bgbri = clampi(g.bgbri + d, 0, 2);
        dirty = true;
    } else if (k == UK_TRI) { Settings def; g.bgfx = def.bgfx; g.bgspd = def.bgspd; g.bgbri = def.bgbri; dirty = true; }
    else if (k == UK_OK || k == UK_CANCEL) { bgfx_pg = false; sel = rowidx(S_SCREEN, 14); }
    return a;
}
