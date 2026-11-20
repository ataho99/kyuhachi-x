#pragma once
/* screens/lines.h -- t845(안내문 user.ini, KH-P10) · t844 (KH-J25): S_LINES 줄 음량. 이 작품 linekey.tsv 의 줄 이름별 슬라이더(0~100퍼센트, 10씩). 설정 > 소리 > 「줄 음량 설정」에서 들어온다(게임 중 메뉴·줄 표가 있는 작품만).
   * 값이 바뀔 때마다 Act A_LINEGAIN(entry = 줄 이름, arg = 퍼센트) — 적용·저장(user.ini linegain)은 호스트(linekey.h kh_lk_set). */
inline void Ui::d_lines(Canvas & c) {
    titlebar(c, "");
    rbox(c, 16, 24, 608, 296, WH, T("줄 음량"));
    const int n = (int)ln_name.size(), CAP = 9; const int top = lst_win(n, sel, CAP);
    for (int i = 0; i < CAP && top + i < n; i++) {
        const int q = top + i, y = 50 + i * 24; const bool s = q == sel; char b[16];
        text(c, 40, y, fit(ln_name[q], 150), s ? K : WH, s ? WH : K);
        snprintf(b, sizeof b, "%d%%", ln_pct[q]);
        barv(c, 220, y, 300, ln_pct[q] / 100.0, "0", "100", WH, s); text(c, 540, y, b, WH, K);
    }
    if (n == 0) text(c, 40, 50, T("이 작품에는 줄 표가 없습니다."), GY, K);
    lst_arrows(c, top, n, CAP, 600, 34, 296);
    text(c, 40, 274, T("같은 줄의 소리는 한꺼번에 바뀝니다. 100 = 그대로, 0 = 끔."), GY, K);
    text(c, 40, 294, T("바꾼 값은 이 작품(user.ini linegain)에 저장됩니다."), GY, K);
    helpline(c, T("↑↓:줄 선택, ←→:음량(10씩), △:100 으로, ×:뒤로"));
}
inline void Ui::k_lines(UKey k, Act & a) {
    const int n = (int)ln_name.size();
    if (k == UK_CANCEL) { go(S_CONF, rowidx(S_CONF, 37)); return; }
    if (sel < 0 || sel >= n) return;
    if (k == UK_LEFT || k == UK_RIGHT || k == UK_TRI) {
        const int v0 = ln_pct[sel]; const int v = (k == UK_TRI) ? 100 : clampi(v0 + (k == UK_RIGHT ? 10 : -10), 0, 100);
        if (v != v0) { ln_pct[sel] = v; a.t = A_LINEGAIN; a.entry = ln_name[sel]; a.arg = v; }
    }
}
