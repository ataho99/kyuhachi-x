#pragma once
/* screens/bgfx.h -- kx-bg: settings > screen > background effect page (sub-page of S_SCREEN, like the phosphor page).
 * Global only (g.bgfx / g.bgspd / g.bgbri), saved with the global settings; the effect itself is drawn by bgx::draw().
 * Rows 0..12 = the effects: the cursor row is shown live behind the menu (preview), OK picks it (marked with *).
 * Rows 13 / 14 = speed / brightness (left/right or OK to change). */
static inline const char * bgx_fxkey(int i) {
    static const char * const K[bgx::NFX] = { "bg_fx0", "bg_fx1", "bg_fx2", "bg_fx3", "bg_fx4", "bg_fx5", "bg_fx6", "bg_fx7", "bg_fx8",
        "bg_fx9", "bg_fx10", "bg_fx11", "bg_fx12" };
    return K[i < 0 ? 0 : i >= bgx::NFX ? bgx::NFX - 1 : i];
}
inline void Ui::d_bgfx(Canvas & c) {
    const int N = bgx::NFX; const bool off = g.bgfx == 0;
    titlebar(c, phr_("bg_title"));
    rbox(c, 16, 24, 232, 36 + N * 16, WH, phr_("bg_fx"));
    std::vector<std::string> n;
    for (int i = 0; i < N; i++) n.push_back(std::string(g.bgfx == i ? "* " : "  ") + phr_(bgx_fxkey(i)));
    items(c, 40, 44, 200, n, sel, 16);
    rbox(c, 256, 24, 368, 72, WH, phr_("bg_choices"));
    std::vector<std::string> v = { phr_("bg_spd"), phr_("bg_bri") }; std::vector<bool> dm = { off, off };
    items(c, 280, 44, 130, v, sel - N, 16, &dm);
    const std::string sv[2] = { phr_(g.bgspd == 0 ? "bg_sp0" : g.bgspd == 1 ? "bg_sp1" : "bg_sp2"),
        phr_(g.bgbri == 0 ? "bg_br0" : g.bgbri == 1 ? "bg_br1" : "bg_br2") };
    for (int i = 0; i < 2; i++) text(c, 424, 44 + i * 16, std::string("< ") + sv[i] + " >", off ? GY : (sel == N + i ? CY : WH), K);
    text(c, 264, 112, std::string(phr_("bg_prev")) + ": " + phr_(bgx_fxkey(sel < N ? sel : g.bgfx)), CY, K);
    const char * dk = sel < N ? "desc_bg_fx" : sel == N ? "desc_bg_spd" : "desc_bg_bri";
    desc_mq(c, 324, 100, std::string(dkey(phr_(dk))) + "  " + phr_("bg_note"));
    helpline(c, phr_("bg_help"));
}
inline Act Ui::k_bgfx(UKey k, Act & a) {
    const int N = bgx::NFX;
    if ((k == UK_LEFT || k == UK_RIGHT) && sel >= N) {
        const int d = k == UK_RIGHT ? 1 : -1;
        if (g.bgfx > 0 && sel == N) g.bgspd = clampi(g.bgspd + d, 0, 2);
        else if (g.bgfx > 0 && sel == N + 1) g.bgbri = clampi(g.bgbri + d, 0, 2);
        dirty = true;
    } else if (k == UK_TRI) { Settings def; g.bgfx = def.bgfx; g.bgspd = def.bgspd; g.bgbri = def.bgbri; dirty = true; }
    else if (k == UK_OK && sel < N) { g.bgfx = sel; dirty = true; }
    else if (k == UK_OK) {
        if (g.bgfx > 0) { if (sel == N) g.bgspd = (g.bgspd + 1) % 3; else g.bgbri = (g.bgbri + 1) % 3; dirty = true; }
    } else if (k == UK_CANCEL) { bgfx_pg = false; sel = rowidx(S_SCREEN, 14); }
    return a;
}
