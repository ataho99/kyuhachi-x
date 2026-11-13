#pragma once
/* screens/quit.h -- S8a: S_QUIT screen. Code moved unchanged from kh_ui.h (members of struct Ui, defined out of line). */
inline void Ui::d_quit(Canvas & c) {
    const char * q[] = { phr_("설정 저장하고 끝냄"), phr_("저장 안 함"), phr_("취소") }; int cw = choices_w(q, 3); int bw = cw + 64 > 360 ? cw + 64 : 360, bx, by;
    confirm_box(c, bw, 112, phr_("확인"), bx, by);
    const char * msg = phr_("설정을 저장하고 끝냅니까?"); text(c, bx + ((bw - tw(msg)) >> 1), by + 30, msg, WH, K);
    choices(c, bx + ((bw - cw) >> 1) + 4, by + 70, q, 3, quitsel, WH, false);
    hits.clear(); ov::g_chit.clear();   /* kx-touch: 세 칸 탭 = 위/아래 키(칸 이동) + 결정 */
    for (int i = 0, hx = bx + ((bw - cw) >> 1) + 4; i < 3; i++) {
        Hit ht = { hx - 2, by + 67, tw(q[i]) + 12, 24, i, quitsel }; hits.push_back(ht); hx += tw(q[i]) + 16; }
    helpline(c, phr_("←→:선택, ○:결정, ×:취소"));
}
inline void Ui::k_quit(UKey k, Act & a) {
        if (k == UK_LEFT && quitsel > 0) quitsel--; if (k == UK_RIGHT && quitsel < 2) quitsel++;
        if (k == UK_UP && quitsel > 0) quitsel--; if (k == UK_DOWN && quitsel < 2) quitsel++;   /* kx-touch: 탭이 보내는 키 */
        if (k == UK_CANCEL) { if (in_game) go(S_GAME, 7); else scr = S_MAIN; }
        if (k == UK_OK) { if (quitsel == 2) { if (in_game) go(S_GAME, 7); else scr = S_MAIN; } else { a.t = A_QUIT; a.arg = quitsel == 0 ? 1 : 0; } }
}
