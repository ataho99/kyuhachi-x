#pragma once
/* screens/tidy.h -- kx-clean: S_TIDY 「목록 정리」. 작품 목록을 사용자가 직접 정리한다.
 * 줄마다 제목 · 상태(정상 / 원본 없음 / 중복 의심). ○ 로 여러 줄 표시 → △ 메뉴: 목록에서 빼기(원본·큐하치 폴더는 그대로, 숨김 표시만 — 세이브는 남기거나 _old 로)
 * · 합치기(두 줄 중 남길 쪽 고름: 세이브·사용자 제목을 옮기고 다른 줄은 숨김) · 다시 찾기(원본 폴더 다시 지정) · 보기 바꾸기(전체 / 원본 없음만 / 숨긴 항목) · 되살리기.
 * 데이터는 host(app/tidy.h)가 tidy_rows 에 채우고, 동작은 A_TIDY 로 넘긴다(윈도·안드 같은 코드). Members of struct Ui, defined out of line. */
inline std::vector<int> Ui::tidy_vis() const {
    std::vector<int> v;
    for (size_t i = 0; i < tidy_rows.size(); i++) {
        const TidyRow & r = tidy_rows[i];
        if (tidy_filt == 2) { if (r.hidden) v.push_back((int)i); }
        else if (!r.hidden && (tidy_filt == 0 || r.st == 1)) v.push_back((int)i);
    }
    return v;
}
inline int Ui::tidy_nmark() const { int n = 0; for (size_t i = 0; i < tidy_mark.size(); i++) if (tidy_mark[i]) n++; return n; }
inline std::vector<int> Ui::tidy_picked(int cursor_row) const {   /* 표시한 줄들, 없으면 커서 줄 하나 */
    std::vector<int> r; for (size_t i = 0; i < tidy_mark.size() && i < tidy_rows.size(); i++) if (tidy_mark[i]) r.push_back((int)i);
    if (r.empty() && cursor_row >= 0 && cursor_row < (int)tidy_rows.size()) r.push_back(cursor_row);
    return r;
}
inline void Ui::d_tidy(Canvas & c) {
    titlebar(c, phr_("목록 정리"));
    std::vector<int> v = tidy_vis(); const int n = (int)v.size();
    if (sel >= n) sel = n > 0 ? n - 1 : 0;
    const char * const fnm = tidy_filt == 0 ? phr_("전체") : tidy_filt == 1 ? phr_("원본 없음만") : phr_("숨긴 항목");
    rbox(c, 16, 24, 408, 284, WH, phr_("목록 정리"));
    text(c, 32, 44, std::string(phr_("보기")) + ": " + fnm + "  (" + std::to_string(n) + ")   " + phr_("표시") + ": " + std::to_string(tidy_nmark()), CY, K);
    const int VIS = 9, PT = 24, LX = 32, LWD = 384, Y0 = 68;
    int top = sel - 4; if (top > n - VIS) top = n - VIS; if (top < 0) top = 0;
    if (n == 0) text(c, LX, Y0, phr_("(해당하는 작품이 없습니다)"), GY, K);
    const int tagw = std::max(tw(phr_("원본 없음")), tw(phr_("중복"))) + 8;
    for (int k = 0; k < VIS && top + k < n; k++) {
        int i = top + k, yy = Y0 + k * PT, ri = v[i]; const TidyRow & r = tidy_rows[ri]; bool sl = (i == sel && tidy_lvl == 0);
        if (tidy_lvl == 0) { Hit ht = { LX - 8, yy - 3, LWD + 2, 22, i, sel }; hits.push_back(ht); }
        if (sl) c.rect(LX - 8, yy - 3, LWD + 2, 22, WH);
        const bool mk = ri < (int)tidy_mark.size() && tidy_mark[ri];
        const char * tag = r.hidden ? "" : r.st == 1 ? phr_("원본 없음") : r.st == 2 ? phr_("중복") : "";
        c.clip(LX - 6, yy - 3, LWD, 22);
        text(c, LX, yy, mk ? "[*]" : "[ ]", sl ? K : (mk ? YL : GY), sl ? WH : K);
        const int mw = tw("[*] ") + 2;
        text(c, LX + mw, yy, fit(r.title, LWD - mw - tagw - 12), sl ? K : (r.st == 1 && !r.hidden ? GY : CY), sl ? WH : K);
        if (tag[0]) text(c, LX + LWD - tagw - 4, yy, tag, sl ? K : (r.st == 1 ? OR : YL), sl ? WH : K);
        c.unclip();
    }
    if (top > 0) text(c, 408, 30, "↑", CY, K); if (top + VIS < n) text(c, 408, 288, "↓", CY, K);
    if (!tidy_msg.empty()) text(c, 32, 288, fit(tidy_msg, 360), YL, K);
    rbox(c, 432, 24, 192, 284, WH, phr_("인포메이션"));
    if (n > 0) {
        const TidyRow & r = tidy_rows[v[sel]]; int y = 44;
        text(c, 444, y, fit(r.title, 168), WH, K); y += 22;
        text(c, 444, y, fit(r.src.empty() ? std::string(phr_("(없음)")) : r.src, 168), GY, K); y += 22;
        const char * st = r.hidden ? phr_("숨김") : r.st == 1 ? phr_("원본 없음") : r.st == 2 ? phr_("중복 의심") : phr_("정상");
        text(c, 444, y, std::string(phr_("상태")) + ": " + st, r.st == 1 && !r.hidden ? OR : WH, K); y += 22;
        text(c, 444, y, std::string(phr_("세이브")) + ": " + (r.saves ? phr_("있음") : phr_("없음")), WH, K);
    }
    const std::vector<std::string> m = { phr_("목록에서 빼기 (세이브 남김)"), phr_("빼고 세이브는 _old 로"), phr_("합치기"), phr_("다시 찾기 (원본 폴더)"), phr_("보기 바꾸기"), phr_("숨긴 항목 되살리기"), phr_("표시 모두 해제") };
    const std::vector<bool> dm = { tidy_filt == 2, tidy_filt == 2, tidy_filt == 2, tidy_filt == 2, false, tidy_filt != 2, tidy_nmark() == 0 };
    if (tidy_lvl == 1) items(c, 444, 150, 176, m, tidy_ms, 21, &dm);
    else for (size_t i = 0; i < m.size(); i++) text(c, 444, 150 + (int)i * 21, fit(m[i], 168), dm[i] ? GY : CY, K);
    if (tidy_lvl == 2 && tidy_ia >= 0 && tidy_ib >= 0) {
        int bw = 520, bx, by; confirm_box(c, bw, 150, phr_("남길 쪽을 고르세요"), bx, by);
        std::vector<std::string> k2 = { fit(tidy_rows[tidy_ia].title, bw - 60), fit(tidy_rows[tidy_ib].title, bw - 60) };
        text(c, bx + 24, by + 22, fit(phr_("다른 쪽의 세이브·사용자 제목을 남길 쪽으로 옮기고, 그 줄은 목록에서 뺍니다."), bw - 48), WH, K);
        hits.clear(); ov::g_chit.clear();
        items(c, bx + 24, by + 60, bw - 48, k2, tidy_keep, 26);
    }
    helpline(c, tidy_lvl == 2 ? phr_("↑↓:남길 쪽, ○:정함, ×:취소") : tidy_lvl == 1 ? phr_("↑↓:고르기, ○:결정, ×:목록으로") : phr_("↑↓:이동, ○:표시, △:메뉴, L R:보기, ×:돌아가기"));
}
inline void Ui::tidy_menu_do(int mi, Act & a) {
    std::vector<int> v = tidy_vis(); const int n = (int)v.size();
    const int cr = (n > 0 && sel >= 0 && sel < n) ? v[sel] : -1;
    tidy_msg.clear();
    if (mi == 4) { tidy_filt = (tidy_filt + 1) % 3; sel = 0; tidy_lvl = 0; return; }
    if (mi == 6) { for (size_t i = 0; i < tidy_mark.size(); i++) tidy_mark[i] = 0; tidy_lvl = 0; return; }
    std::vector<int> pk = tidy_picked(cr);
    if (pk.empty()) { tidy_msg = phr_("표시한 작품이 없습니다."); tidy_lvl = 0; return; }
    if (mi == 5) {   /* 되살리기 */
        tidy_ids.clear(); for (int i : pk) if (tidy_rows[i].hidden) tidy_ids.push_back(tidy_rows[i].id);
        if (tidy_ids.empty()) { tidy_msg = phr_("숨긴 항목 보기에서 고르세요."); tidy_lvl = 0; return; }
        a.t = A_TIDY; a.arg = 3; tidy_lvl = 0; return;
    }
    tidy_ids.clear(); for (int i : pk) if (!tidy_rows[i].hidden) tidy_ids.push_back(tidy_rows[i].id);
    if (tidy_ids.empty()) { tidy_msg = phr_("이미 목록에서 뺀 작품입니다."); tidy_lvl = 0; return; }
    if (mi == 0 || mi == 1) {
        yn_open(S_TIDY, YN_TIDYHIDE, phr_("표시한 작품을 목록에서 뺍니까?"), mi == 0 ? phr_("원본 파일과 세이브는 그대로 둡니다.") : phr_("세이브는 보관 폴더(_old)로 옮깁니다. 원본 파일은 그대로입니다."), std::string(phr_("작품 수")) + ": " + std::to_string((int)tidy_ids.size()), mi);
        return;
    }
    if (mi == 2) {
        if (pk.size() != 2 || tidy_ids.size() != 2) { tidy_msg = phr_("합칠 두 작품을 ○로 표시하세요."); tidy_lvl = 0; return; }
        tidy_ia = pk[0]; tidy_ib = pk[1]; tidy_keep = 0; tidy_lvl = 2; return;
    }
    if (mi == 3) {
        if (pk.size() != 1) { tidy_msg = phr_("다시 찾을 작품 한 개만 고르세요."); tidy_lvl = 0; return; }
        a.t = A_TIDY; a.arg = 4; tidy_lvl = 0; return;
    }
}
inline void Ui::k_tidy(UKey k, Act & a) {
    std::vector<int> v = tidy_vis(); const int n = (int)v.size();
    if (tidy_lvl == 2) {
        if (k == UK_UP || k == UK_DOWN) tidy_keep ^= 1;
        else if (k == UK_CANCEL) { tidy_lvl = 0; }
        else if (k == UK_OK && tidy_ia >= 0 && tidy_ib >= 0) {
            const int kr = tidy_keep == 0 ? tidy_ia : tidy_ib, dr = tidy_keep == 0 ? tidy_ib : tidy_ia;
            tidy_keep_id = tidy_rows[kr].id; tidy_drop_id = tidy_rows[dr].id; tidy_ids.clear(); tidy_ids.push_back(tidy_drop_id);
            tidy_lvl = 0;
            yn_open(S_TIDY, YN_TIDYMERGE, std::string(phr_("남길 쪽")) + ": " + fit(tidy_rows[kr].title, 380), std::string(phr_("합치는 쪽")) + ": " + fit(tidy_rows[dr].title, 370), phr_("세이브·사용자 제목을 옮기고 이 줄은 목록에서 뺍니다."));
        }
        return;
    }
    if (tidy_lvl == 1) {
        if (k == UK_UP) tidy_ms = (tidy_ms + 6) % 7; else if (k == UK_DOWN) tidy_ms = (tidy_ms + 1) % 7;
        else if (k == UK_OK) { const bool dim[7] = { tidy_filt == 2, tidy_filt == 2, tidy_filt == 2, tidy_filt == 2, false, tidy_filt != 2, tidy_nmark() == 0 }; if (!dim[tidy_ms]) tidy_menu_do(tidy_ms, a); }
        else if (k == UK_CANCEL || k == UK_TRI) tidy_lvl = 0;
        return;
    }
    if (k == UK_UP) { if (n) sel = (sel + n - 1) % n; }
    else if (k == UK_DOWN) { if (n) sel = (sel + 1) % n; }
    else if (k == UK_LEFT || k == UK_RIGHT) { if (n) { int p = sel + (k == UK_RIGHT ? 9 : -9); if (p > n - 1) p = n - 1; if (p < 0) p = 0; sel = p; } }
    else if (k == UK_OK) { if (n && sel < n) { int ri = v[sel]; if (ri < (int)tidy_mark.size()) tidy_mark[ri] = tidy_mark[ri] ? 0 : 1; tidy_msg.clear(); } }
    else if (k == UK_TRI) { tidy_lvl = 1; }
    else if (k == UK_L || k == UK_R) { tidy_filt = (tidy_filt + (k == UK_R ? 1 : 2)) % 3; sel = 0; tidy_msg.clear(); }
    else if (k == UK_CANCEL) { go(S_ADD, 7); }
}
