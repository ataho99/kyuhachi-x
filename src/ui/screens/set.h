#pragma once
/* screens/set.h -- build 5b-1: settings hub (screen / sound / control / system) and the in-game keyboard submenu. */
/* 메뉴 정리안 1안: 설정 = 전역 기본값 허브 (화면·소리·조작·시스템·셰이더 + 진단·조작 안내·정보). 「이 작품 설정」(범위 고정)에서는 앞 4줄만 */
inline void Ui::d_set(Canvas & c) {
    titlebar(c, "");
    const bool wk = scope_lock && scope == 1;
    rbox(c, 16, 24, 288, 224, WH, wk ? phr_("이 작품 설정") : phr_("set_title"));
    std::vector<std::string> m = { phr_("set_screen"), phr_("set_sound"), phr_("set_control"), phr_("set_system") };
    if (!wk) { m.push_back(phr_("set_shim")); m.push_back(phr_("진단")); m.push_back(phr_("조작 안내")); m.push_back(phr_("정보·라이선스")); }
    items(c, 40, 42, 256, m, sel, wk ? 28 : 22);
    info_box(c, 320, 24, 304, 224);
    static const char * md[] = { "set_d0", "set_d1", "set_d2", "set_d3", "set_d4" };
    static const char * md2[] = { "진동 시험·컨트롤러 정보·진단 로그를 만듭니다. (진동이 되네 안되네 할 때)", "조작 방법 안내를 봅니다.", "버전·라이선스를 봅니다." };
    const int si = sel >= 0 && sel < (int)m.size() ? sel : 0;
    desc(c, 264, 64, { si >= 5 ? std::string(phr_(md2[si - 5])) : std::string(phr_(md[si])) });
    if (wk) text(c, 344, 200, phr_("이 작품만 따로 정합니다. 바꾼 줄은 노랑 ● 입니다."), YL, K);
    helpline(c, phr_("set_help"));
}
inline void Ui::k_set(UKey k, Act & a) {
    (void)a;
    const bool wk = scope_lock && scope == 1; const int n = wk ? 4 : 8;
    if (k == UK_UP) sel = (sel + n - 1) % n;
    else if (k == UK_DOWN) sel = (sel + 1) % n;
    else if (k == UK_OK) {
        if (sel == 0) { scr_from = S_SET; enter_screen_cfg(); go(S_SCREEN, 0); }
        else if (sel == 4) go(S_SHIM, 0);
        else if (sel == 5) { diag_wait = 0; diag_ans.clear(); go(S_DIAG, 0); }
        else if (sel == 6) go(S_HELP, 0);
        else if (sel == 7) go(S_ABOUT, 0);
        else { conf_grp = sel; enter_conf_cfg(); go(S_CONF, 0); }
    } else if (k == UK_CANCEL) {
        conf_grp = 0;
        if (wk) { scope_lock = 0; if (in_game) { scope = 0; go(S_GAME, 5); } else go(S_EXTRA, 1); }   /* 이 작품 설정에서 나오면 「이 작품」으로 */
        else { scope_lock = 0; if (in_game) go(S_GAME, 4); else go(S_MAIN, 0); }
    }
}
inline void Ui::d_gkb(Canvas & c) {
    titlebar(c, "");
    rbox(c, 16, 24, 368, 224, WH, phr_("pg_kb"));
    const std::string sp = "  ";
    std::vector<std::string> a = { phr_("gkb_keys"),
        std::string(phr_("gkb_kbd")) + sp + (kb_vis ? phr_("gkb_on") : phr_("gkb_off")),
        std::string(phr_("gkb_btn")) + sp + (kb_btn ? phr_("gkb_show") : phr_("gkb_hide")),
        std::string(phr_("gkb_sh")) + sp + (kb_sh ? phr_("gkb_apply") : phr_("gkb_no")), phr_("gkb_shot") };
    items(c, 40, 46, 336, a, sel, 28);
    helpline(c, phr_("gkb_help"));
}
inline void Ui::k_gkb(UKey k, Act & a) {
    if (k == UK_UP) sel = (sel + 4) % 5;
    else if (k == UK_DOWN) sel = (sel + 1) % 5;
    else if (k == UK_OK) {
        if (sel == 0) go(S_KEYS, 0);
        else if (sel == 1) a.t = A_KBD;
        else if (sel == 2) a.t = A_KBBTN;
        else if (sel == 3) a.t = A_KBSH;
        else a.t = A_SHOT;
    } else if (k == UK_CANCEL) go(S_GAME, 6);
}
