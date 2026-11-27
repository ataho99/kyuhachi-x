#pragma once
/* screens/screen.h -- S8b: S_SCREEN screen. Code moved unchanged from kh_ui.h / screen_table.h (members of struct Ui, defined out of line). */
inline void Ui::d_screen(Canvas & c) {   /* t48c (#624·#625): 필터 1줄 + 효과 4줄 + 곡면. #587: 범위 둘(전역/이 작품) · 이 작품에서는 전역 전용 줄을 숨기고 맨 아래 「개별 설정 초기화」 · 개별 값 줄 앞 ● */
    if (phos_pg) { d_phos(c); return; }
    if (bgfx_pg) { d_bgfx(c); return; }   /* t662 (#635) */
    titlebar(c, phr_("화면 설정"));
    Settings & s = eff();
    rbox(c, 16, 24, 288, 220, WH, phr_("화면 설정"));
    const std::vector<int> ri = rowids(S_SCREEN);
    const int PY = menu_tall() ? 18 : (ri.size() > 12 ? 15 : 16);   /* kx-bg: one more row on the global page */   /* t858: 14·15px급 글꼴은 목록·값 열 피치 18 */
    const bool own = scope == 1 && cur >= 0 && cur < MAXG; const bool ind = own && ovm[cur] != 0u;
    static const unsigned rb[17] = { 0u, 1u << 0, 1u << 1, 1u << 5, 1u << 6, 1u << 3, 1u << 4, 1u << 2, 0u, 0u, 1u << 16, 0u, 127u, 1u << 22, 0u, 0u, 0u };
    std::vector<std::string> n;
    { const std::string nm[17] = { phr_("적용 범위"), phr_("비율"), phr_("필터"), phr_("주사선"), phr_("마스크"), phr_("RGB 번짐"), phr_("빛 번짐"), phr_("곡면"), phr_("창 배율"), phr_("전체화면"), "FPS / Speed", phr_("indiv_reset"), phr_("preset"), phr_("ph_title"), "", std::string(phr_("메뉴 꾸밈")) + "  >", T("화면 위치 (16:10)") };
      for (int id : ri) n.push_back(id == 14 ? std::string(phr_("bg_row")) : (own && (ovm[cur] & rb[id])) ? std::string("● ") + nm[id] : nm[id]); }
    const int rs = (sel >= 0 && sel < (int)ri.size()) ? ri[sel] : 0;
    const int NR = (int)ri.size(), CAP = 184 / PY; const int top = lst_win(NR, sel, CAP);   /* t848: 마지막 줄이 상자 아래 테두리에 닿지 않게 한 줄 덜 */   /* kx-nova: 스크롤 */
    { std::vector<bool> dm(n.size(), false); for (size_t q = 0; q < ri.size(); q++) if (ri[q] == 13 && s.ratio == 2) dm[q] = true;
      const size_t e2 = (size_t)std::min(NR, top + CAP); std::vector<std::string> n2(n.begin() + top, n.begin() + e2); std::vector<bool> d2(dm.begin() + top, dm.begin() + e2); items(c, 40, 44, 256, n2, sel - top, PY, &d2); }
    lst_arrows(c, top, NR, CAP, 296, 29, 228);
    rbox(c, 320, 24, 304, 220, WH, phr_("선택지"));
    auto row = [&](int i) { return rs == i; };
    auto Y = [&](int id) { int k = 0; for (size_t i = 0; i < ri.size(); i++) if (ri[i] == id) k = (int)i; if (k < top || k >= top + CAP) return -20000; return 44 + (k - top) * PY; };
    const char * sc[] = { T("전역"), phr_("scope_this") }; if (scope_lock) text(c, 344, Y(0), scope == 1 ? T("이 작품 고정") : T("전역 기본값 고정"), scope == 1 ? YL : WH, K); else choices(c, 344, Y(0), sc, 2, scope, WH, row(0));
    { for (int q_ = top; q_ < std::min(NR, top + CAP); q_++) { const int id_ = ri[q_]; if (id_ == 0 || id_ == 12 || id_ == 15) continue;   /* 정리안 A안: 값 출처 꼬리표 */
        const bool ov_ = own && (ovm[cur] & rb[id_]) != 0u; const bool sl_ = q_ == sel;
        int ex_ = 0; if (scope == 0 && rb[id_]) for (int gi_ = 0; gi_ < (int)games.size() && gi_ < MAXG; gi_++) if (ovm[gi_] & rb[id_]) ex_++;   /* t822: 예외 N작품 */
        const bool fa_ = !ov_ && ex_ == 0 && kh_factory_row(true, id_, s);   /* t822: 출하값 */
        char tg_[24]; snprintf(tg_, sizeof tg_, "%s%d", T("예외"), ex_ > 9 ? 9 : ex_);
        { const char * tag_ = ov_ ? T("이 작품") : ex_ > 0 ? tg_ : fa_ ? T("출하값") : T("전역"); text(c, 296 - tw(tag_), 44 + (q_ - top) * PY, tag_, ov_ ? YL : ex_ > 0 ? OR : fa_ ? CY : GY, sl_ ? WH : K); } } }   /* #1478 B3: 꼬리표 오른쪽 정렬 */
    row_name_now.clear(); if (sel >= 0 && sel < (int)n.size()) { row_name_now = n[sel]; if (row_name_now.compare(0, 4, "● ") == 0) row_name_now.erase(0, 4); }
    { const int pp = pre_pos(); const int nb = PRE_NB + (int)presets.size(); const std::string pn = pp >= 0 ? pre_name(pp) : std::string(phr_("preset_custom")); if (row(12)) g_row_val = pn; (void)nb; text(c, 344, Y(12), std::string("< ") + pn + " >", row(12) ? CY : WH, K); }   /* 빌드 6: 프리셋 줄 */
    const char * rt[] = { "4:3", "16:10", phr_("정수배") }; choices(c, 344, Y(1), rt, 3, s.ratio, WH, row(1));
    if (scope == 0) { const char * vp_[] = { phr_("가운데"), phr_("위쪽"), phr_("아래쪽") }; choices(c, 344, Y(16), vp_, 3, s.vpos, s.ratio == 1 ? WH : GY, row(16)); }   /* t848 */
    const char * sh[] = { "sharp", "nearest" }; choices(c, 344, Y(2), sh, 2, s.shader == 2 ? 1 : 0, WH, row(2));   /* t856 (슨상 07:47 「샤퍼는 삭제 해도 되고」): sharper 삭제 - 저장된 3 은 sharp 로 읽음 */
    char b[16]; const int fxv[4] = { s.scan, s.mask, s.bleed, s.glow };
    for (int i = 0; i < 4; i++) { int y = Y(3 + i); if (fxv[i] <= 0) snprintf(b, sizeof b, "%s", phr_("끔")); else snprintf(b, sizeof b, "%d%%", fxv[i] * 10); barv(c, 340, y, 216, fxv[i] / 10.0, phr_("끔"), phr_("최대"), WH, row(3 + i)); text(c, 566, y, b, WH, K); }
    snprintf(b, sizeof b, "%.2f", s.curve_f()); barv(c, 340, Y(7), 216, s.curve / 10.0, phr_("평면"), phr_("최대"), WH, row(7)); text(c, 566, Y(7), b, WH, K);
#if !defined(__ANDROID__)
    if (scope == 0) { const char * wm[] = { "x1", "x2", "x3", "x4", "x5", phr_("사용자 지정") }; unsigned dmk = 0; for (int i = 0; i < WMN; i++) if (i + 1 > wmax) dmk |= 1u << i;
      choicesd(c, 344, Y(8), wm, 6, wmult ? wmult - 1 : 5, WH, dmk);
      const char * fsc[] = { phr_("창 모드"), phr_("전체화면") }; choices(c, 344, Y(9), fsc, 2, fs_now ? 1 : 0, WH, row(9)); }   /* t52i (B3) */
#endif
    { char pb[32]; if (s.phos == 0) snprintf(pb, sizeof pb, "%s", phr_("ph_lv0")); else snprintf(pb, sizeof pb, "%d %d %d %d", s.ph_beam(), s.ph_pers(), s.ph_breath(), s.ph_glow());
      text(c, 344, Y(13), std::string(pb) + "  >", s.ratio == 2 ? GY : WH, K); }
    if (scope == 0) text(c, 344, Y(14), std::string(phr_(bgx_fxkey(g.bgfx))) + "  >", WH, K);   /* kx-bg */   /* t662 (#635): phosphor page summary */
    if (scope == 0) { const char * shm[] = { phr_("shim_m0"), phr_("shim_m1"), phr_("shim_m2") }; text(c, 344, Y(15), std::string(shm[clampi(g.shim, 0, 2)]) + "  >", WH, K); }   /* #1478: 메뉴 꾸밈 줄 */
    char wb[192]; if (wmult) snprintf(wb, sizeof wb, phr_("창 x%d (%dx%d) · 회색 = 모니터에 안 들어감 · Alt+[ ]"), wmult, 640 * wmult, 480 * wmult); else snprintf(wb, sizeof wb, phr_("창 사용자 지정 %dx%d · 테두리로 끈 크기 · Alt+[ ]"), winw, winh);
    static std::string wd; wd = wb;
    static const char * dd[] = { "desc_scope", "desc_ratio", "desc_filter", "desc_scan", "desc_mask", "desc_bleed", "desc_glow", "desc_curve" };
    int exn_ = 0; if (scope == 0 && rs > 0 && rs < 14 && rb[rs]) for (int gi_ = 0; gi_ < (int)games.size() && gi_ < MAXG; gi_++) if (ovm[gi_] & rb[rs]) exn_++;   /* t822: 예외 N작품 */
    char exb_[96]; snprintf(exb_, sizeof exb_, "%s %d%s", T("예외"), exn_, T("작품: 이 줄 값을 따로 씀"));
    /* #1478 B1·B2: 미리보기 상자 제거 → 주황 안내 상자를 두 줄로 접어 고정 표시(흐르는 한 줄 대신). 아래에 한 줄 상태·저장함 */
    { const std::string dtx = rs == 16 ? dkey(T("화면 위치: 비율이 16:10 일 때 게임 화면을 화면 위·가운데·아래 중 어디에 붙일지 정합니다. 남는 검은 띠는 반대쪽으로 모이고, 오버레이 버튼은 4:3 기준 자리에 그대로 있습니다.")) : rs < 8 ? dkey(phr_(dd[rs])) : rs == 15 ? dkey(phr_("set_d4")) : rs == 14 ? dkey(phr_("desc_bg")) : rs == 13 ? dkey(phr_("desc_phos")) : rs == 12 ? dkey(phr_("desc_preset")) : rs == 11 ? dkey(phr_("desc_indiv_reset")) : rs == 9 ? dkey(phr_("desc_fullscr")) : wd;
      std::vector<std::string> dl = wrap_words(dtx, 560); if (dl.size() > 3) dl.resize(3);
      const int dh = dl.size() > 2 ? 72 : 52; desc(c, 252, dh, dl);
      const int ny = 252 + dh + 8;
      if (save_note_pend) { save_note = std::string(T("저장함: ")) + row_name_now + (g_row_val.empty() ? std::string() : " = " + g_row_val) + (scope == 1 ? T(" (이 작품)") : T(" (전역)")); save_note_pend = 0; }
      if (!save_note.empty() && ts_ms < save_note_until) { c.rect(16, ny - 2, 608, 20, K); text(c, 24, ny, fit(save_note, 592), YL, K); }   /* t822: 잔상 지움 */
      else text(c, 24, ny, fit(exn_ > 0 ? std::string(exb_) : rs == 0 && scope == 0 ? dkey(phr_("scr_allglobal_off")) : (ind ? std::string(phr_("indiv_has")) : std::string(T("값을 바꾸면 이 화면에도 바로 적용됩니다."))), 592), exn_ > 0 ? OR : WH, K); }
    const bool ibk = own && rs < 15 && (ovm[cur] & indiv_bit(S_SCREEN, rs));
    helpline(c, ibk ? phr_("indiv_help") : phr_("↑↓:선택, ←→:값, ○:결정, ×:취소(되돌림), △:출하 시 설정"));
}
inline Act Ui::k_screen_conf(UKey k, Act & a) {
    if (scr == S_CONF && tmini) { if (k == UK_CANCEL) { tmini = false; a.t = A_RESUME; return a; }   /* t862f: 작은 창 — × = 게임으로 돌아감 */
        if (k == UK_OK || k == UK_LEFT || k == UK_RIGHT) { const std::vector<int> ri_ = rowids(S_CONF); const int id_ = (sel >= 0 && sel < (int)ri_.size()) ? ri_[sel] : 0; const int d_ = k == UK_LEFT ? -1 : 1; Settings & s_ = g;
            if (id_ == 55) { if (k != UK_LEFT) { a.t = A_EXTRA; a.arg = 7; a.game = cur; } return a; }
            if (id_ == 52) s_.touchbar = s_.touchbar ? 0 : 1;
            else if (id_ == 53) s_.touchmode = s_.touchmode ? 0 : 1;
            else if (id_ == 54) { if (s_.touchmode) s_.touchdir2 = s_.touchdir2 ? 0 : 1; else s_.touchdir = s_.touchdir ? 0 : 1; }
            else if (id_ == 56) s_.tstick = s_.tstick ? 0 : 1;
            else if (id_ == 57) s_.tmmode = s_.tmmode ? 0 : 1;
            else if (id_ == 58) s_.taccel = (s_.taccel + d_ + 3) % 3;
            else if (id_ == 59) s_.tsens = (s_.tsens + d_ + 3) % 3;
            else if (id_ == 60) s_.trclick = s_.trclick ? 0 : 1;
            else return a;
            save_now = true; dirty = true; a.t = A_APPLY; return a; } }   /* 누르기·좌우 = 값 바꿈(바로 저장) */
    if (scr == S_SCREEN && phos_pg) return k_phos(k, a);
    if (scr == S_SCREEN && bgfx_pg) return k_bgfx(k, a);   /* t662 (#635) */
    const Scr scr0 = scr; const std::vector<int> ri = rowids(scr0); const int sidx = sel; const int sid = (sidx >= 0 && sidx < (int)ri.size()) ? ri[sidx] : 0;
    if (k == UK_TRI && scope == 1 && cur >= 0 && cur < MAXG) {   /* build 9: triangle on a marked line = that line back to the global value */
        const unsigned ib = (sid < (scr0 == S_SCREEN ? 15 : 38) || (scr0 == S_CONF && sid >= 62 && sid <= 66)) ? indiv_bit(scr0, sid) : 0u;
        if (ib && (ovm[cur] & ib)) { ov_set(cur, ovm[cur] & ~ib); sync_t(cur); dirty = true; a.t = A_APPLY; return a; }
    }
    struct Gd { Ui & u; Scr s0; int sidx, sid, gi; bool have; Settings pre;   /* #587: 줄 위치 ↔ 실제 줄 번호 · 이 작품 범위에서 바뀐 줄만 개별 값으로 */
        ~Gd() { if (u.scr != s0) return; if (u.nav_skip) { u.nav_skip = false; return; } if (u.sel == sid) u.sel = sidx;
                if (have && u.scope == 1 && u.cur == gi && gi >= 0 && gi < Ui::MAXG) { unsigned d = Ui::diff_bits(pre, u.t[gi]); if (d) u.ov_set(gi, u.ovm[gi] | d); u.sync_t(gi); } } } gd{ *this, scr0, sidx, sid, cur, false, Settings() };
    if (scope == 1 && cur >= 0 && cur < MAXG) { sync_t(cur); gd.pre = t[cur]; gd.have = true; }
    sel = sid;
    do { {
        if (k == UK_CANCEL && (((scr == S_SCREEN) && sel >= 11 && sel <= 15) || ((scr == S_CONF) && (sel == 14 || sel == 61 || (sel >= 38 && sel <= 47))))) { if (!(scr == S_SCREEN)) { cb_has = false; a.t = A_APPLY; dirty = true; }   /* t52i: × = 뒤로. 값은 바뀐 순간부터 저장 대상 */ if ((scr == S_SCREEN) && backup_has) { eff() = backup; if (bk_cur >= 0 && bk_cur < MAXG) { ov_set(bk_cur, bk_ovm); sync_t(bk_cur); } backup_has = false; a.t = A_APPLY; if (wmult != bk_wmult || winw != bk_winw || winh != bk_winh) { wmult = bk_wmult; winw = bk_winw; winh = bk_winh; a.t = A_WINMULT; a.arg = wmult; } } leave_cfg(); break; }   /* t853 (#1646): 뒤로(×)를 삼키던 줄들 - 개별 초기화 · 폴더 줄 · 화면 설정 11~15 - 186줄과 같은 동작 */
        if ((scr == S_SCREEN && sel == 11) || (scr == S_CONF && sel == 14)) { if (k == UK_OK) { sel = sidx; yn_open(scr0, YN_INDIVRESET, phr_("indiv_q1"), phr_("indiv_q2")); } break; }   /* 개별 설정 초기화 */
        if (scr == S_CONF && sel == 13 && (k == UK_OK || k == UK_LEFT || k == UK_RIGHT)) { int dl = (k == UK_LEFT) ? -1 : 1; lang = (lang + dl + 3) % 3; phr_set_lang(lang); lang_chg = true; dirty = true; break; }   /* t52g (#133): 언어 - 문구 칸이 제자리에서 바뀌어 바로 보인다 */
        if (scr == S_CONF && sel == 51 && k == UK_OK) { open_font(3, S_CONF); break; }
        if (scr == S_CONF && sel >= 11 && sel <= 12 && (k == UK_OK || k == UK_LEFT || k == UK_RIGHT || k == UK_TRI)) { if (k == UK_OK) open_font(sel - 11, S_CONF); break; }   /* t52f: 글꼴 행이 11·12 로 한 칸씩 내려감 */
        if (scr == S_CONF && sel == 37 && (k == UK_OK || k == UK_RIGHT)) { go(S_LINES, 0); break; }   /* t844: 줄 음량 */
        if (scr == S_CONF && ((sel >= 38 && sel <= 47) || sel == 61)) {   /* #1478: 폴더 줄 · 맵핑(41) · 진단(46) · 부팅 시퀀스 다시 보기(47) */
            if (k == UK_OK || k == UK_RIGHT) {
                const int ch = conf_child(sel);
                if (ch) { conf_grp = ch; go(S_CONF, 0); nav_skip = true; }
                else if (sel == 41) go(S_PAD, 0);
                else if (sel == 46) { diag_wait = 0; diag_ans.clear(); go(S_DIAG, 0); }
                else if (sel == 47 && k == UK_OK) { a.t = A_EXTRA; a.arg = 2; a.game = cur; } }
            break; }
        if (scr == S_CONF && sel == 4 && conf_grp != 0 && (k == UK_OK || k == UK_LEFT || k == UK_RIGHT)) { Settings & s4 = edit(); s4.pad = s4.pad == 1 ? 0 : 1; dirty = true; a.t = A_APPLY; save_note_pend = 1; save_note_until = ts_ms + 2500; return a; }   /* #1478: 결정 버튼 = 컨트롤러 페이지 줄 */
        if (scr == S_CONF && sel == 4 && conf_grp == 0 && (k == UK_OK || k == UK_LEFT || k == UK_RIGHT)) { if (k == UK_OK) go(S_PAD, 0); break; }   /* t48e (#646) */
#if defined(__ANDROID__)
        if (scr == S_CONF && sel == 3 && (k == UK_OK || k == UK_LEFT || k == UK_RIGHT)) { g_sndbuf = (g_sndbuf + (k == UK_LEFT ? 3 : 1)) % 4; if (g_kha_save) g_kha_save(); dirty = true; break; }   /* t137: 소리 버퍼 */
#endif
#if defined(__ANDROID__)
        if (scr == S_CONF && sel == 28 && (k == UK_OK || k == UK_LEFT || k == UK_RIGHT)) { g_cpucore = g_cpucore ? 0 : 1; if (g_kha_save) g_kha_save(); dirty = true; break; }   /* t733: CPU 코어 빠름/호환 (다음 시작부터) */
        if (scr == S_CONF && sel == 27 && (k == UK_OK || k == UK_LEFT || k == UK_RIGHT)) {   /* E3R: 글꼴 폴더(27) */
            if (k == UK_LEFT || k == UK_RIGHT) {
                if (ov::g_kha_fontdir.empty()) { if (!kha_fontdir_pick()) break; }
                else kha_fontdir_set("");
            } else if (ov::g_kha_fontdir.empty()) kha_font_rescan();
            else if (!kha_fontdir_pick()) break;
            dirty = true; break; }
#endif   /* E3R */
        bool scr_s = scr == S_SCREEN; int nrow = scr_s ? 11 : 14;
        if (scr_s && sel == 9 && (k == UK_LEFT || k == UK_RIGHT || k == UK_OK)) { a.t = A_FULLSCR; return a; }   /* t52i (B3): 전체화면 줄 - 좌우·○ 로 바로 오간다 */
        if (scr_s && sel == 12 && (k == UK_LEFT || k == UK_RIGHT || k == UK_OK)) {   /* 빌드 6: 프리셋 - ←→ 고르면 바로 적용(즉시 미리보기), ○ = 저장 칸에서 저장 · 내 프리셋에서 삭제 확인 */
            const int np = (int)presets.size(), tot = pre_total(); int p = pre_pos();
            if (k == UK_OK) {
                if (p >= PRE_NB && p < PRE_NB + np) { sel = sidx; yn_open(scr0, YN_PREDEL, phr_("preset_q1"), phr_("preset_q2"), std::string(), p); }
                else if (p == PRE_NB + np && np < PRE_MAX) { pre_add(eff()); }
                break; }
            if (p < 0) p = (k == UK_RIGHT) ? tot - 1 : 0;
            p = (p + (k == UK_RIGHT ? 1 : tot - 1)) % tot; pre_i = p;
            if (p < PRE_NB + np) { pre_copy(eff(), pre_get(p)); a.t = A_APPLY; dirty = true; save_note_pend = 1; save_note_until = ts_ms + 2500; }
            return a; }
        if (scr_s && sel == 13) { if (k == UK_OK || k == UK_RIGHT) { phos_pg = true; sel = 0; } return a; }
        if (scr_s && sel == 14) { if (k == UK_OK || k == UK_RIGHT) { bgfx_pg = true; sel = 0; } return a; }   /* kx-bg */   /* t662 (#635): open the phosphor page */
        if (scr_s && sel == 15) { if (k == UK_OK || k == UK_RIGHT) { go(S_SHIM, 0); } return a; }   /* #1478: 메뉴 꾸밈 (전역 범위) */
        if (scr == S_CONF && sel == 15 && (k == UK_LEFT || k == UK_RIGHT || k == UK_OK)) { g.fpsd = g.fpsd ? 0 : 1; a.t = A_APPLY; dirty = true; return a; }   /* #639: FPS display is global only */
        if (sel == 0 && k == UK_OK && scope == 0 && !scope_lock) { sel = sidx; yn_open(scr0, YN_INDIVALL, phr_("indiv_allq1"), phr_("indiv_q2")); break; }   /* t147d: 전역 범위에서 ○ = 전역 일괄 적용 켬/끔 */
        if (scr == S_CONF && sel == 55 && in_game) { if (k == UK_OK || k == UK_RIGHT) { a.t = A_EXTRA; a.arg = 7; a.game = cur; } return a; }   /* t862c: 터치 배치 편집 */
        Settings & s = (sel == 0) ? eff() : edit();
        UKey kk = k;   /* kx-touch: 선택지 없는 값 줄을 탭(○)하면 오른쪽 절반 = 오른쪽 키, 왼쪽 절반 = 왼쪽 키 */
        if (k == UK_OK && ov::g_tap_half != 0) { const int th = ov::g_tap_half; const bool fresh = ov::tap_now_ms() - ov::g_tap_ms < 400; ov::g_tap_half = 0;
            bool sl = false; if (fresh && sel != 0) { if (scr_s) sl = (sel >= 1 && sel <= 8) || sel == 16; else sl = (sel >= 1 && sel <= 2) || (sel >= 48 && sel <= 50) || sel == 52 || sel == 53 || sel == 54 || (sel >= 5 && sel <= 10) || (sel >= 16 && sel <= 26) || sel == 34 || sel == 35 || (sel >= 29 && sel <= 32) || (sel >= 56 && sel <= 60) || (sel >= 62 && sel <= 66); }
            if (sl) kk = th > 0 ? UK_RIGHT : UK_LEFT; }
        if (kk == UK_LEFT || kk == UK_RIGHT) {
            int d = (kk == UK_RIGHT) ? 1 : -1;
            if (scr_s && sel == 8) {   /* 창 배율(전역): 모니터에 들어가는 다음/이전 배율 */
                int m = wm_next(d); if (m > 0) { wmult = m; a.t = A_WINMULT; a.arg = m; dirty = true; }
                return a;
            }
            if (sel == 0) { if (!scope_lock) scope_step(d); }
            else if (scr_s) {
                if (sel == 1) s.ratio = clampi(s.ratio + d, 0, 2);
                if (sel == 2) { int fi_ = s.shader == 2 ? 1 : 0; fi_ += d > 0 ? 1 : -1; if (fi_ < 0) fi_ = 0; if (fi_ > 1) fi_ = 1; s.shader = fi_ == 1 ? 2 : 0; }   /* t48c: 필터 = sharp / nearest */
                if (sel == 16) { s.vpos = clampi(s.vpos + d, 0, 2); if (s.ratio == 1) { if (s.vpos == 1) s.ovpos_edge = 0; else if (s.vpos == 2) s.ovpos_edge = 1; } }   /* t848: 화면 위치 · t849: 위 → 오버레이 변 아래, 아래 → 변 위, 가운데 → 사용자 값 유지(16:10 일 때만) */
                { if (sel == 3) s.scan = clampi(s.scan + d, 0, 10); if (sel == 4) s.mask = clampi(s.mask + d, 0, 10); if (sel == 5) s.bleed = clampi(s.bleed + d, 0, 10); if (sel == 6) s.glow = clampi(s.glow + d, 0, 10); if (sel == 7) s.curve = clampi(s.curve + d, 0, 10); }   /* t48c: 필터와 상관없이 효과를 켠다. 바꾸면 A_APPLY 로 곧바로 화면에 반영 */
                a.t = A_APPLY;
            } else {
                if (sel == 1) s.bgm = clampi(s.bgm + d, 0, 8);
                if (sel == 2) s.se = clampi(s.se + d, 0, 8);
                if (sel == 36) s.vo = clampi(s.vo + d, 0, 8);   /* t831: 음성·PCM */
                if (sel == 5) s.bmult = clampi(s.bmult + d, 0, 4);
                if (sel == 18) s.cpuspd = clampi(s.cpuspd + d, 0, 3);
                if (sel == 6) s.bmode = clampi(s.bmode + d, 0, 2);
                if (sel == 7) s.bsound = clampi(s.bsound + d, 0, 2);
                if (sel == 16) s.dsnd = clampi(s.dsnd + d, 0, 2);   /* #641 */
                if (sel == 17) s.dvol = clampi(s.dvol + d, 0, 8);
                if (sel == 19) s.pwrseq = clampi(s.pwrseq + d, 0, 2);
                if (sel == 20) s.hddhum = clampi(s.hddhum + d, 0, 2);
                if (sel == 21) s.hvol = clampi(s.hvol + d, 0, 8);
                if (sel == 26) s.mclick = clampi(s.mclick + d, 0, 1); /*E1R*/
                if (sel == 34) s.vibe = clampi(s.vibe + d, 0, 1);   /* t794 */
                if (sel == 35) s.vibe_pct = clampi(s.vibe_pct + d * 10, 20, 300);   /* t822: 진동 세기 */
                if (sel == 29) s.touchfx = clampi(s.touchfx + d, 0, 3);   /* kx-touch */
                if (sel == 30) s.tfxsz = clampi(s.tfxsz + d, 0, 1);
                if (sel == 31) s.tfxmouse = clampi(s.tfxmouse + d, 0, 1);
                if (sel == 32) s.tmouse = clampi(s.tmouse + d, 0, 1);
                if (sel == 33) s.backpos = clampi(s.backpos + d, 0, 4);   /* kx-touch */
                if (sel == 8) s.boot = clampi(s.boot + d, 0, 1); if (sel == 9) s.bosd = clampi(s.bosd + d, 0, 1);
                if (sel == 10) s.ovbtn = clampi(s.ovbtn + d, 0, 1);   /* E2R: split */
                if (sel == 22) s.ovpos_mode = clampi(s.ovpos_mode + d, 0, 1);   /* E2R */
                if (sel == 23) s.ovpos_edge = clampi(s.ovpos_edge + d, 0, 3);   /* E2R */
                if (sel == 24) s.ovpos_align = clampi(s.ovpos_align + d, 0, 2);   /* E2R */
                if (sel == 25) s.ovpos_free = clampi(s.ovpos_free + d * 5, 0, 100);   /* E2R */
                if (sel == 48) s.ovsize = clampi(s.ovsize + d, 0, 3);   /* t848 */
                if (sel == 49) s.ovalpha = clampi(s.ovalpha + d, 0, 3);
                if (sel == 50) s.ovauto = s.ovauto ? 0 : 1;
                if (sel == 52) { s.touchbar = s.touchbar ? 0 : 1; save_now = true; }
                if (sel == 53) { s.touchmode = s.touchmode ? 0 : 1; save_now = true; }   /* t862a */
                if (sel == 54) { if (s.touchmode) s.touchdir2 = s.touchdir2 ? 0 : 1; else s.touchdir = s.touchdir ? 0 : 1; save_now = true; }   /* t860: 터치 키 표시 - 고르는 즉시 저장 */
                if (sel == 56) { s.tstick = s.tstick ? 0 : 1; save_now = true; }   /* t869: 마우스 페이지 */
                if (sel == 57) { s.tmmode = s.tmmode ? 0 : 1; save_now = true; }
                if (sel == 58) { s.taccel = (s.taccel + d + 3) % 3; save_now = true; }
                if (sel == 59) { s.tsens = (s.tsens + d + 3) % 3; save_now = true; }
                if (sel == 60) { s.trclick = s.trclick ? 0 : 1; save_now = true; }
                if (sel == 62) s.stk_l = clampi(s.stk_l + d, 0, 2);   /* t869: 패드 스틱 (옛 맵핑 화면 줄) */
                if (sel == 63) s.stk_r = clampi(s.stk_r + d, 0, 2);
                if (sel == 64) s.stk_sp = clampi(s.stk_sp + d, 1, 3);
                if (sel == 65) s.dz_l = clampi(s.dz_l + d, 0, 3);
                if (sel == 66) s.dz_r = clampi(s.dz_r + d, 0, 3);
                a.t = A_APPLY;
            }
            dirty = true; (void)nrow;
            if (sel != 0) { save_note_pend = 1; save_note_until = ts_ms + 2500; }   /* 정리안 A안: 문구는 다음 그림에서(새 값이 그려진 뒤) 만든다 */
        } else if (k == UK_TRI) { Settings def; Settings & e = eff(); if (scr_s) {   /* E2R */
            e.vpos = def.vpos; e.ratio = def.ratio; e.shader = def.shader; e.curve = def.curve; e.bleed = def.bleed;
            e.glow = def.glow; e.scan = def.scan; e.mask = def.mask; e.phos = def.phos; }
        else { e.bgm = def.bgm; e.se = def.se; e.vo = def.vo; e.bmult = def.bmult; e.cpuspd = def.cpuspd;
            e.bmode = def.bmode; e.boot = def.boot; e.bosd = def.bosd; e.bsound = def.bsound;
            if (conf_grp == 1) { e.dsnd = def.dsnd; e.dvol = def.dvol; e.pwrseq = def.pwrseq;
                e.hddhum = def.hddhum; e.hvol = def.hvol; e.mclick = def.mclick; e.vibe = def.vibe; e.vibe_pct = def.vibe_pct; /*E1R*/
                e.touchfx = def.touchfx; e.tfxsz = def.tfxsz; e.tfxmouse = def.tfxmouse; e.tmouse = def.tmouse; e.backpos = def.backpos;   /* kx-touch */ }
            e.ovbtn = def.ovbtn; e.ovpos_mode = def.ovpos_mode; e.ovpos_edge = def.ovpos_edge;
            e.ovpos_align = def.ovpos_align; e.ovpos_free = def.ovpos_free; e.ovsize = def.ovsize; e.ovalpha = def.ovalpha; e.ovauto = def.ovauto; e.touchbar = def.touchbar; e.touchmode = def.touchmode; e.touchdir = def.touchdir; e.touchdir2 = def.touchdir2;
            if (conf_grp == 11) { e.stk_l = def.stk_l; e.stk_r = def.stk_r; e.dz_l = def.dz_l; e.dz_r = def.dz_r; e.stk_sp = def.stk_sp;   /* t869: 마우스 페이지 출하 값 */
                if (scope == 0) { e.tmouse = def.tmouse; e.tfxmouse = def.tfxmouse; e.tmmode = def.tmmode; e.taccel = def.taccel; e.tsens = def.tsens; e.trclick = def.trclick; e.tstick = def.tstick; } }
            if (scope == 0) g.fpsd = def.fpsd; } dirty = true; a.t = A_APPLY; }
        else if (k == UK_CANCEL) { if (!scr_s) { cb_has = false; a.t = A_APPLY; dirty = true; }   /* t52i: × = 뒤로. 값은 바뀐 순간부터 저장 대상 */ if (scr_s && backup_has) { eff() = backup; if (bk_cur >= 0 && bk_cur < MAXG) { ov_set(bk_cur, bk_ovm); sync_t(bk_cur); } backup_has = false; a.t = A_APPLY; if (wmult != bk_wmult || winw != bk_winw || winh != bk_winh) { wmult = bk_wmult; winw = bk_winw; winh = bk_winh; a.t = A_WINMULT; a.arg = wmult; } } leave_cfg(); }
        else if (k == UK_OK) { backup_has = false; cb_has = false; dirty = true; a.t = A_APPLY; leave_cfg(); }
        break; }
    } while (0);
    return a;
}
