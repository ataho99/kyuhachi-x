#pragma once
/* screens/touch.h -- S_TOUCH: 터치 8칸 키 재배정 (t859-K). S_PAD 맨 끝 줄에서 진입, scope 계승, 뒤로가기는 S_PAD.
 * 줄 구성 = 적용 범위 + 재배정 8줄(←→↑↓ Enter Esc Space Tab) + 고정 4줄(MENU/KBD/L/R, 표시만).
 * ○=키 고르기(S_PADKEY·S_KBPICK 재사용, 터치용 목록) · △=초기화(삭제=기본값) · ×=뒤로(S_PAD). */
inline void Ui::d_touch(Canvas & c) {
    titlebar(c, phr_("조작 설정"));
    std::map<std::string, std::string> wm; if (scope == 1 && env.touch_get && cur >= 0 && cur < (int)games.size()) env.touch_get(cur, wm);   /* 한 번만 읽는다 */
    auto val = [&](int i, bool & fw, bool & df) -> std::string {
        std::string k = touch_key(i); fw = false; df = false;
        auto it = wm.find(k); if (it != wm.end() && !it->second.empty()) { fw = true; return it->second; }
        auto g2 = gpad.find(k); if (g2 != gpad.end() && !g2->second.empty()) return g2->second;
        df = true; return touch_default(i); };
    rbox(c, 16, 24, 288, 296, WH, phr_("조작 설정"));
    std::vector<std::string> r; r.push_back(phr_("적용 범위"));
    for (int i = 0; i < TOUCH_N; i++) { bool fw, df; val(i, fw, df); r.push_back(std::string(fw ? "* " : "  ") + fit(touch_row_name(i), 220)); }
    static const char * const FX[4] = { "MENU", "KBD", "L", "R" };
    for (int i = 0; i < 4; i++) r.push_back(std::string("  ") + fit(std::string(FX[i]) + " (" + phr_("touch_fixed") + ")", 220));
    std::vector<uint32_t> col(r.size(), 0u);
    rowlist(c, 40, 44, 256, 11, r, sel, 24, &col);
    rbox(c, 320, 24, 304, 296, WH, phr_("인포메이션"));
    const char * sc[] = { phr_("전역"), phr_("scope_this") }; choices(c, 344, 44, sc, 2, scope, WH, sel == 0);
    std::string ln;
    if (sel == 0) ln = phr_("전역=모든 작품, 이 작품=이 작품만. 작품 값이 우선.");
    else if (sel >= 1 && sel <= 8) { bool fw, df; std::string v = val(sel - 1, fw, df); ln = touch_row_name(sel - 1) + " → " + pad_show(v) + ((scope == 1 && !fw) ? (df ? phr_(" (기본)") : phr_(" (전역)")) : (scope == 0 && df ? phr_(" (기본)") : "")); }
    else ln = std::string(FX[sel - 9]) + " (" + phr_("touch_fixed") + ")";
    desc(c, 324, 44, { fit(dkey(ln), 560) });
    helpline(c, phr_("↑↓:선택, ←→:값, ○:키 고르기, ×:뒤로, △:출하 시 설정"));
}
inline void Ui::k_touch(UKey k, Act & a) {
    auto mv = [&](int & s, int cnt) { if (cnt < 1) cnt = 1; if (k == UK_UP) s = (s + cnt - 1) % cnt; else if (k == UK_DOWN) s = (s + 1) % cnt; };
    do { {
            mv(sel, 13);
            if (k == UK_LEFT || k == UK_RIGHT) {
                int d = (k == UK_RIGHT) ? 1 : -1;
                if (sel == 0) { scope_step(d); }
            } else if (k == UK_OK) {
                if (sel >= 1 && sel <= 8) open_touchkey(sel - 1);
            } else if (k == UK_TRI) {   /* 출하 시 설정: 이 범위의 터치 8줄을 모두 지움(삭제=기본값) */
                if (scope == 0) { for (int i = 0; i < TOUCH_N; i++) gpad.erase(touch_key(i)); }
                else if (cur >= 0 && cur < (int)games.size() && env.touch_set) { for (int i = 0; i < TOUCH_N; i++) env.touch_set(cur, touch_key(i), std::string()); }
                dirty = true; save_now = true; a.t = A_APPLY;
            } else if (k == UK_CANCEL) { go(S_PAD, PAD_N + 7); }   /* S_PAD 맨 끝(터치 진입 줄)로 */
            break; }
    } while (0);
}
