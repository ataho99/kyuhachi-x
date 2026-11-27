#pragma once
/* screens/font.h -- S8b: S_FONT screen. Code moved unchanged from kh_ui.h / screen_table.h (members of struct Ui, defined out of line). */
inline void Ui::d_font(Canvas & c) {
    if (fnt_mode == 4) { d_fontmk(c); return; }   /* t865 */
    if (fnt_mode == 3) {   /* t855: 메뉴 글꼴 - M갈무리 계열(계열 3 × 크기 10·12·15 × 보통/Bold) + quiple 갈무리 + 옛 글꼴. 모두 ○ = 적용하고 머무름, ←→ = 크기, × = 뒤로 */
        titlebar(c, ""); rbox(c, 16, 24, 288, 296, WH, phr_("메뉴 글꼴"));
        std::string cf_; int cs_ = 12; bool cb_ = false; const bool cm_ = mf_split(font_ui, cf_, cs_, cb_); if (!cm_) { cs_ = 12; cb_ = false; }
        std::vector<std::string> r; std::vector<uint32_t> col;
        for (auto & o : fnt_opts) {
            uint32_t cc_ = 0;
            if (o == "#sz") { r.push_back(std::string("  ") + fit(std::string(phr_("크기")) + ": " + std::to_string(cs_) + (cm_ ? " < >" : ""), 220)); if (!cm_) cc_ = GY; }
            else if (o == "#bd") { r.push_back(std::string("  ") + fit(std::string(phr_("굵기")) + ": " + (cb_ ? phr_("굵게 (Bold)") : phr_("보통")) + (cm_ ? " < >" : ""), 220)); if (!cm_) cc_ = GY; }
            else if (o == "#mk") { r.push_back(std::string("  ") + phr_("내 글꼴 가져오기")); cc_ = CY; }
            else if (o.size() == 3 && o[0] == '#') r.push_back(std::string(cm_ && cf_ == o.substr(1) ? "● " : "  ") + (o == "#mg" ? phr_("M갈무리") : o == "#dg" ? phr_("둥근갈무리") : phr_("굴린갈무리")));
            else r.push_back(std::string(o == font_ui ? "● " : "  ") + fit(mfont_label(o), 220));
            col.push_back(cc_);
        }
        rowlist(c, 40, 48, 256, 9, r, sel, 24, &col);
        rbox(c, 320, 24, 304, 296, WH, phr_("미리보기"));
        std::string pv = sel < (int)fnt_opts.size() ? fnt_opts[sel] : font_ui;
        if (pv.size() == 3 && pv[0] == '#' && pv != "#sz" && pv != "#bd" && pv != "#mk") pv = mf_make(pv.substr(1), cs_, cb_); else if (pv == "#sz" || pv == "#bd" || pv == "#mk") pv = font_ui;
        const ov::BmpFont * mb = pv == "builtin" ? NULL : mfont_get(pv);
        const ov::BmpFont * sv = ov::g_menu_bmp; const int * sa_ = ov::g_menu_advp; const int sh_ = ov::g_menu_half, sf_ = ov::g_menu_full; ov::g_menu_bmp = mb; ov::g_menu_advp = mb ? mf_adv(pv, mb) : NULL; ov::g_menu_half = mb ? mf_half(pv) : 8; ov::g_menu_full = mb ? mf_full(pv) : 16;   /* 미리보기만 고른 글꼴(칸 폭 포함)로 */
        text(c, 340, 44, fit(pv == "builtin" ? std::string(phr_("옛 메뉴 글꼴")) : mfont_label(pv), 270), YL, K);
        text(c, 340, 80, phr_("한글 가나다라마바사"), WH, K); text(c, 340, 104, "Settings Sound BGM 0123", WH, K); text(c, 340, 128, phr_("화면 효과"), WH, K); text(c, 340, 152, "あいうえお アイウエオ", WH, K);
        ov::g_menu_bmp = sv; ov::g_menu_advp = sa_; ov::g_menu_half = sh_; ov::g_menu_full = sf_;
        desc(c, 324, 58, { phr_("메뉴 글자 모양입니다. 게임 화면의 글꼴은 따로 정합니다(게임 글꼴)."), phr_("M갈무리 (C) MonadABXY · SIL OFL 1.1 (res\\fonts\\menu\\OFL.txt)") }, -1, phr_("↑↓:선택, ○:적용, ←→:크기, ×:뒤로"));
        helpline(c, !mk_del.empty() ? phr_("△:한 번 더 누르면 지웁니다, 다른 키:취소") : (sel < (int)fnt_opts.size() && fnt_opts[sel].compare(0, 2, "u_") == 0) ? phr_("↑↓:선택, ○:적용, △:지우기, ×:뒤로") : phr_("↑↓:선택, ○:적용, ←→:크기, ×:뒤로")); return;
    }
    titlebar(c, ""); std::string ttl = fnt_mode == 0 ? phr_("한글 글꼴") : fnt_mode == 1 ? phr_("일본어 글꼴") : phr_("실행 방법 글꼴");
    rbox(c, 16, 24, 288, 296, WH, ttl.c_str());
    std::vector<std::string> r; std::vector<uint32_t> col; std::string curv = fnt_cur();
    for (auto & o : fnt_opts) { r.push_back(std::string(o == curv ? "● " : "  ") + fit(o.empty() ? std::string(fnt_mode == 2 ? phr_("자동 (목록 기본)") : phr_("기본")) : (o == "#mk" ? std::string(phr_("내 글꼴 가져오기")) : o.compare(0, 2, "u_") == 0 ? std::string(phr_("내 글꼴")) + ": " + fontbake::label(o) : o), 220)); col.push_back(o == "#mk" ? (uint32_t)CY : 0u); }
    if (fnt_opts.empty()) { r.push_back(phr_("(고를 수 있는 글꼴 없음)")); col.push_back(GY); }
    rowlist(c, 40, 48, 256, 9, r, sel, 24, &col);
    rbox(c, 320, 24, 304, 296, WH, phr_("미리보기"));
    std::string pv = sel < (int)fnt_opts.size() ? fnt_opts[sel] : std::string(); if (pv.empty()) pv = fnt_default_name();
    if (pv == "#mk") pv = std::string();   /* t865 */
    const ov::BmpFont * bf = fnt_font(pv);
    text(c, 340, 44, fit(pv.empty() ? std::string("-") : pv, 270), YL, K);
    { std::string grp; for (auto & f : fonts) if (f.name == pv) grp = f.group; text(c, 340, 64, fit(grp.empty() ? std::string(phr_("묶음 -")) : std::string(fontreg::group_label(grp)), 270), CY, K); }
    { std::string pp; for (auto & f : fonts) if (f.name == pv) pp = f.path;   /* E3R: 고른 글꼴의 폴더 */
      if (!pp.empty()) { size_t s1 = pp.find_last_of("\\/");
        std::string dr = s1 == std::string::npos ? pp : pp.substr(0, s1);
        size_t s2 = dr.find_last_of("\\/");
        size_t s0 = (s2 == std::string::npos || s2 == 0) ? std::string::npos : dr.find_last_of("\\/", s2 - 1);
        text(c, 340, 260, fit(s0 == std::string::npos ? dr : dr.substr(s0 + 1), 270), CY, K); } }
    if (bf) { text_bmp(c, *bf, 340, 100, phr_("한글 가나다라마바사"), WH, K); text_bmp(c, *bf, 340, 124, phr_("열락의 학원 신비한 바다"), WH, K); text_bmp(c, *bf, 340, 148, "あいうえお アイウエオ", WH, K); } else text(c, 340, 100, phr_("미리볼 수 없는 글꼴입니다."), GY, K);
    { int rej = 0; for (auto & f : fonts) if (!f.ok) rej++; if (rej) text(c, 340, 280, fit(phr_("거른 파일 ") + std::to_string(rej) + phr_("개 (형식·크기 불일치)"), 270), OR, K); }
    text(c, 340, 196, fit(phr_("같은 묶음의 글꼴만 바꿀 수 있습니다."), 270), WH, K); text(c, 340, 216, fit(phr_("다른 묶음은 한글이 깨져 막아 둡니다."), 270), WH, K);
    if (!fnt_need.empty()) text(c, 340, 240, fit(phr_("이 작품의 묶음: ") + std::string(fontreg::group_label(fnt_need)), 270), CY, K);
    desc(c, 324, 58, { fnt_mode == 2 ? phr_("이 실행 방법의 글꼴입니다. 자동 = 목록 기본(한글판=한글, 원판=일본어).") : phr_("전역 기본 글꼴입니다. 실행 방법이 글꼴을 따로 정하면 그것이 우선입니다."), phr_("글꼴 폴더에 bmp 를 넣으면 자동으로 목록에 나옵니다.") }, -1, phr_("↑↓:선택, ○:정함, ×:취소"));
    helpline(c, !mk_del.empty() ? phr_("△:한 번 더 누르면 지웁니다, 다른 키:취소") : (sel < (int)fnt_opts.size() && fnt_opts[sel].compare(0, 2, "u_") == 0) ? phr_("↑↓:선택, ○:정함, △:지우기, ×:취소") : phr_("↑↓:선택, ○:정함, ×:취소"));
}
inline void Ui::k_font(UKey k, Act & a) {
    if (fnt_mode == 4) { k_fontmk(k, a); return; }   /* t865 */
    if (k == UK_TRI) {   /* t867: 내 글꼴(u_…) 위에서 △ 두 번 = 지우기 */
        const std::string o_ = sel < (int)fnt_opts.size() ? fnt_opts[sel] : std::string();
        if (o_.compare(0, 2, "u_") == 0) { if (mk_del == o_) { mk_delete(o_, a); mk_del.clear(); } else mk_del = o_; }
        return;
    }
    mk_del.clear();
    auto mv = [&](int & s, int cnt) { if (cnt < 1) cnt = 1; if (k == UK_UP) s = (s + cnt - 1) % cnt; else if (k == UK_DOWN) s = (s + 1) % cnt; };
    do {
            mv(sel, (int)fnt_opts.size());
            if (fnt_mode == 3) {   /* t855: 메뉴 글꼴 - ○ = 적용하고 머무름 · ←→ = 크기(굵기 줄이면 보통/Bold) · × = 뒤로 */
                std::string cf_ = "mg"; int cs_ = 12; bool cb_ = false; const bool cm_ = mf_split(font_ui, cf_, cs_, cb_); if (!cm_) { cf_ = "mg"; cs_ = 12; cb_ = false; }
                const std::string o = sel < (int)fnt_opts.size() ? fnt_opts[sel] : std::string(); std::string nn = font_ui;
                auto stepsz = [&](int d) { static const int S3[3] = { 10, 12, 15 }; int i = 1; for (int q = 0; q < 3; q++) if (S3[q] == cs_) i = q; i = (i + d + 3) % 3; return S3[i]; };
                if (k == UK_LEFT || k == UK_RIGHT) { if (cm_) { if (o == "#bd") nn = mf_make(cf_, cs_, !cb_); else nn = mf_make(cf_, stepsz(k == UK_RIGHT ? 1 : -1), cb_); } }
                else if (k == UK_OK) {
                    if (o == "#sz") { if (cm_) nn = mf_make(cf_, stepsz(1), cb_); }
                    else if (o == "#bd") { if (cm_) nn = mf_make(cf_, cs_, !cb_); }
                    else if (o == "#mk") { mk_open(3); break; }
                    else if (o.size() == 3 && o[0] == '#') nn = mf_make(o.substr(1), cs_, cb_);
                    else if (!o.empty()) nn = o;
                }
                else if (k == UK_CANCEL) { scr = fnt_ret; sel = fnt_prev_sel; break; }
                if (nn != font_ui && (nn == "builtin" || mf_have(nn))) { font_ui = nn; menu_font_apply(); dirty = true; save_now = true; a.t = A_APPLY; }
                break;
            }
            if (k == UK_OK && sel < (int)fnt_opts.size()) {
                const std::string o = fnt_opts[sel];
                if (o == "#mk") { mk_open(fnt_mode); break; }   /* t865 */
                if (fnt_mode == 0) font_kr = (o == "kyuhachi_font" || o == "pc98player_font") ? std::string() : o; else if (fnt_mode == 1) font_jp = (o == "jp_default") ? std::string() : o;
                else { if (o.empty()) { ed.erase("font"); ed.erase("font_kind"); } else { ed["font"] = o; std::string g; for (auto & f : fonts) if (f.name == o) g = f.group; if (g == fnt_need) ed["font_kind"] = g == "jis" ? "jp" : "kr"; /* t52c: 다른 묶음 글꼴을 고르면 작품 종류(font_kind)는 그대로 둔다 */ } }
                dirty = true; save_now = true; scr = fnt_ret; sel = fnt_prev_sel; a.t = A_APPLY;
            } else if (k == UK_CANCEL) { scr = fnt_ret; sel = fnt_prev_sel; }
            break;
    } while (0);
}
