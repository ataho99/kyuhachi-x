#pragma once
/* screens/main.h -- S8a: S_MAIN screen. #1478 메뉴 개편 v1: 메인 4줄 (게임 시작 · 작품 선택 · 설정 · 끝내기). 세이브 관리는 시작 팝업 4번째 줄, 이 작품은 작품 선택 맨 아래 줄로 옮김. */
inline void Ui::d_main(Canvas & c, int mode) {
    struct Fx_ { Fx_(bool on) { g_help_fix2 = on; } ~Fx_() { g_help_fix2 = false; } } fx_(mode != 3);   /* t855 틀 고정 */
    if (mode != 3) {
    titlebar(c, "");
    rbox(c, 16, 24, 232, 272, WH, phr_("메인메뉴"));   /* t855 시안 v3: 왼쪽 상자 = 메뉴 4 + 최근 실행 5 */
    const int msel = mode == 2 ? 3 : mainsel;
    std::vector<std::string> m; menu_names(m); items(c, 30, 41, 216, m, msel, 26);
    std::vector<int> rc; recent_runs(rc);
    c.rect(30, 148, 202, 1, GY); text(c, 30, 156, phr_("최근 실행"), GY, K);
    if (rc.empty()) text(c, 30, 182, phr_("(아직 없음)"), GY, K);
    else { std::vector<std::string> rn; for (int gi_ : rc) rn.push_back(games[gi_].title); items(c, 30, 180, 216, rn, msel, 22, NULL, 4); }
    const int sv_cur = cur; int vg = cur; if (msel >= 4 && msel - 4 < (int)rc.size()) vg = rc[msel - 4];
    if (vg >= 0 && vg < (int)games.size()) cur = vg;   /* 인포·설명은 고른 줄의 작품(최근 실행이면 그 작품)으로 그린다 */
    info_pane(c, game(), cur);
    static const char * md[] = { T("현재 작품을 이어하기(마지막 저장) 또는 처음부터 시작합니다. 세이브 관리도 여기서 합니다."), T("작품을 고릅니다. 고른 작품이 현재 작품이 됩니다. 이 작품의 설정도 목록 맨 아래에 있습니다."), T("전역 기본 설정(디스플레이·사운드·조작·실행환경)과 조작 안내·정보입니다."), T("설정을 저장하고 끝냅니다.") };
    { const std::string kk = T("↑↓:선택, ○:결정, ×:취소, SELECT:이 작품"); const std::string hp = md[msel < 4 ? msel : 0];
      const bool shown = desc_box(c, game(), hp, kk);
      if (mode == 0) { helpline(c, kk); if (shown) help_right(c, kk, hp); } }
    cur = sv_cur;
    }
    if (mode == 1 || mode == 3) {
        const UiGame * gm = game(); bool hs = has_save(gm); int bx, by; confirm_box(c, 400, 224, phr_("게임 시작"), bx, by);
        bool blk = gm && !blk_cur().empty();
        text(c, bx + 24, by + 24, gm ? ttl_fit(*gm, 352) : std::string(), blk ? GY : CY, K);
        std::vector<std::string> a = { hs ? phr_("이어하기   슬롯 ") + slot_lbl(gm->lastslot) + "  " + gm->slot[gm->lastslot].stamp : std::string(phr_("이어하기   (저장 없음)")), phr_("처음부터"), phr_("시스템 디스크만 부팅"), phr_("세이브 관리") };
        std::vector<bool> dm = { !hs || blk, blk, blk, false }; items(c, bx + 30, by + 60, 340, a, startsel, 26, &dm);
        if (blk) { text(c, bx + 24, by + 184, fit(phr_("실행 불가: ") + blk_cur(), 352), OR, K); }
        helpline(c, phr_("↑↓:선택, ○:결정, ×:취소"));
    }
    if (mode == 2) d_quit(c);
}
inline void Ui::k_main(UKey k, Act & a) {
    do {
            if (k == UK_OK && mainsel >= 4) { std::vector<int> rc; recent_runs(rc); const int ri = mainsel - 4; if (ri < (int)rc.size()) { cur = rc[ri]; back = S_MAIN; startsel = has_save(game()) ? 0 : 1; start_ret = S_MAIN; go(S_START, 0); } }   /* t855: 최근 실행 = 그 작품 시작 팝업 */
            else if (k == UK_OK) {
                static const Scr to[4] = { S_START, S_PICK, S_SET, S_QUIT };
                back = S_MAIN; if (to[mainsel] == S_SET) { conf_grp = 0; scope = 0; scope_lock = 1; enter_conf_cfg(); } if (to[mainsel] == S_PICK) { pick = pos_of(cur); reorder = false; } if (to[mainsel] == S_QUIT) quitsel = 0;
                if (to[mainsel] == S_START) { startsel = has_save(game()) ? 0 : 1; start_ret = S_MAIN; }
                go(to[mainsel], 0);
            } else if (k == UK_CANCEL) { go(S_QUIT, 0); quitsel = 0; }
            else if (k == UK_SELECT) go(S_EXTRA, 0);
            break;
    } while (0);
}
