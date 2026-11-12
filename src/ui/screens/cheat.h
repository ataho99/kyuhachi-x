#pragma once
/* screens/cheat.h -- S_CHEAT: 게임 중 메뉴의 「치트」 (kx-cheat 1단계). 화면에 보이는 값을 넣어 찾고(8·16·32비트 동시), 값이 바뀌면 다시 찾아 후보를 줄이고,
 * 후보가 적어지면 골라서 쓰기·고정·저장한다. 패드만으로 된다(숫자 칸 + ○). 늘었나 줄었나 같은 비교 검색은 일부러 없다. */
inline int Ui::ch_op(int op, int idx, long long v, std::string & io) { return env.cheat ? env.cheat(op, idx, v, io) : 0; }
inline long long Ui::ch_num(const std::string & s) const { if (s.empty() || s.size() > 10) return -1; long long v = 0;
    for (size_t i = 0; i < s.size(); i++) v = v * 10 + (s[i] - '0'); return v > 4294967295LL ? -1 : v; }
inline void Ui::ch_lines_of(const std::string & io) { ch_lines.clear(); std::string t; for (size_t i = 0; i < io.size(); i++) {
    if (io[i] == '\n') { ch_lines.push_back(t); t.clear(); } else t += io[i]; } if (!t.empty()) ch_lines.push_back(t); }
inline void Ui::open_cheat() { std::string io; ch_lvl = 0; ch_msg.clear(); ch_n = ch_op(12, 0, 0, io); go(S_CHEAT, 0); }
inline void Ui::d_cheat(Canvas & c) {
    c.rect(0, 0, c.w, c.h, K); titlebar(c, "");
    rbox(c, 48, 20, 544, 344, WH, phr_("ch_title"));
    std::string io;
    if (ch_lvl == 0) {
        std::string st = ch_n < 0 ? std::string(phr_("ch_nosearch")) : std::string(phr_("ch_count")) + " " + std::to_string(ch_n) + (ch_n
            >= 50000 ? std::string(" ") + phr_("ch_trunc") : std::string());
        text(c, 64, 42, fit(st, 510), CY, K);
        int nf = ch_op(11, 0, 0, io);
        std::vector<std::string> a = {
            std::string(phr_("ch_value")) + " " + (ch_val.empty() ? std::string("-") : ch_val),
            ch_n < 0 ? std::string(phr_("ch_find")) : std::string(phr_("ch_refind")),
            std::string(phr_("ch_list")), std::string(phr_("ch_reset")), std::string(phr_("ch_saved")),
                std::string(phr_("ch_unfreeze")) + " (" + std::to_string(nf) + ")" };
        items(c, 80, 74, 480, a, ch_sel, 26);
        text(c, 64, 244, fit(phr_("ch_how1"), 510), GY, K); text(c, 64, 266, fit(phr_("ch_how2"), 510), GY, K);
            text(c, 64, 288, fit(phr_("ch_how3"), 510), GY, K);
        if (!ch_msg.empty()) text(c, 64, 330, fit(ch_msg, 510), YL, K);
        helpline(c, phr_("ch_help0"));
    } else if (ch_lvl == 1) {
        text(c, 64, 42, fit(std::string(phr_(ch_tgt == 0 ? "ch_pad_find" : "ch_pad_write")), 510), CY, K);
        text(c, 64, 70, fit(ch_buf + "_", 400), YL, K);
        static const char * lb[12] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "<", "0", "OK" };
        for (int i = 0; i < 12; i++) { int x = 150 + (i % 3) * 110, y = 108 + (i / 3) * 52; bool s = i == ch_pad;
            c.rect(x, y, 96, 40, s ? WH : GY); c.rect(x + 1, y + 1, 94, 38, s ? WH : K);
                text(c, x + (96 - tw(lb[i])) / 2, y + 11, lb[i], s ? K : WH, s ? WH : K); }
        helpline(c, phr_("ch_help_pad"));
    } else if (ch_lvl == 2) {
        text(c, 64, 42,
            fit(std::string(phr_("ch_count")) + " " + std::to_string(ch_n) + (ch_n > 8 ? std::string("  ") + phr_("ch_many") :
            std::string()), 510), CY, K);
        if (ch_lines.empty()) text(c, 80, 74, phr_("ch_none"), GY, K); else items(c, 80, 74, 480, ch_lines, ch_pick, 26);
        helpline(c, phr_("ch_help_list"));
    } else if (ch_lvl == 3) {
        if (ch_lines.empty()) text(c, 80, 74, phr_("ch_saved_empty"), GY, K); else items(c, 80, 74, 480, ch_lines, ch_pick, 26);
        if (!ch_msg.empty()) text(c, 64, 330, fit(ch_msg, 510), YL, K);
        helpline(c, phr_("ch_help_saved"));
    } else {
        text(c, 64, 42,
            fit(std::string(phr_("ch_target")) + " " + (ch_pick < (int)ch_lines.size() ? ch_lines[ch_pick] : std::string()) + "   -> " +
            ch_wval, 510), CY, K);
        std::vector<std::string> a = {
            std::string(phr_("ch_act_write")), std::string(phr_("ch_act_freeze")), std::string(phr_("ch_act_savef")),
            std::string(phr_("ch_act_savew")), std::string(phr_("ch_act_val")), std::string(phr_("ch_act_back")) };
        items(c, 80, 74, 480, a, ch_sel, 26);
        if (!ch_msg.empty()) text(c, 64, 330, fit(ch_msg, 510), YL, K);
        helpline(c, phr_("ch_help_act"));
    }
}
inline void Ui::k_cheat(UKey k, Act & a) {
    (void)a; std::string io;
    if (ch_lvl == 0) {
        if (k == UK_UP) ch_sel = (ch_sel + 5) % 6; else if (k == UK_DOWN) ch_sel = (ch_sel + 1) % 6;
        else if (k == UK_CANCEL) go(S_GAME, 6);
        else if (k == UK_OK) {
            ch_msg.clear();
            if (ch_sel == 0) { ch_buf = ch_val; ch_tgt = 0; ch_pad = 0; ch_lvl = 1; }
            else if (ch_sel == 1) { long long v = ch_num(ch_val); if (v < 0) ch_msg = phr_("ch_noval"); else {
                ch_n = ch_op(ch_n < 0 ? 1 : 2, 0, v, io); ch_msg = ch_n == 0 ? std::string(phr_("ch_none")) : std::string(); } }
            else if (ch_sel == 2) { if (ch_n < 0) ch_msg = phr_("ch_nosearch"); else { ch_op(3, 0, 0, io); ch_lines_of(io); ch_pick = 0;
                ch_lvl = 2; } }
            else if (ch_sel == 3) { ch_op(0, 0, 0, io); ch_n = -1; ch_val.clear(); ch_msg = phr_("ch_reset_done"); }
            else if (ch_sel == 4) { ch_op(7, 0, 0, io); ch_lines_of(io); ch_pick = 0; ch_lvl = 3; }
            else if (ch_sel == 5) { ch_op(6, 0, 0, io); ch_msg = phr_("ch_unfroze"); }
        }
    } else if (ch_lvl == 1) {
        if (k == UK_UP) ch_pad = (ch_pad + 9) % 12; else if (k == UK_DOWN) ch_pad = (ch_pad + 3) % 12;
        else if (k == UK_LEFT) ch_pad = (ch_pad / 3) * 3 + (ch_pad % 3 + 2) % 3;
            else if (k == UK_RIGHT) ch_pad = (ch_pad / 3) * 3 + (ch_pad % 3 + 1) % 3;
        else if (k == UK_SQ) { if (!ch_buf.empty()) ch_buf.pop_back(); }
        else if (k == UK_CANCEL) ch_lvl = ch_tgt == 0 ? 0 : 4;
        else if (k == UK_SELECT || (k == UK_OK && ch_pad == 11)) { if (ch_tgt == 0) { ch_val = ch_buf; ch_lvl = 0; } else {
            ch_wval = ch_buf; ch_lvl = 4; ch_sel = 0; } }
        else if (k == UK_OK) {
            if (ch_pad == 9) { if (!ch_buf.empty()) ch_buf.pop_back(); }
            else { static const char dg[12] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', 0, '0', 0 };
                std::string t = ch_buf + dg[ch_pad]; if (ch_num(t) >= 0) ch_buf = t; }
        }
    } else if (ch_lvl == 2) {
        int n = (int)ch_lines.size();
        if (k == UK_UP && n > 0) ch_pick = (ch_pick + n - 1) % n; else if (k == UK_DOWN && n > 0) ch_pick = (ch_pick + 1) % n;
        else if (k == UK_CANCEL) ch_lvl = 0;
        else if (k == UK_OK && n > 0) { ch_buf = ch_wval; ch_tgt = 1; ch_pad = 0; ch_lvl = 1; ch_msg.clear(); }
    } else if (ch_lvl == 3) {
        int n = (int)ch_lines.size();
        if (k == UK_UP && n > 0) ch_pick = (ch_pick + n - 1) % n; else if (k == UK_DOWN && n > 0) ch_pick = (ch_pick + 1) % n;
        else if (k == UK_CANCEL) { ch_lvl = 0; ch_msg.clear(); }
        else if (k == UK_OK && n > 0) { int r = ch_op(8, ch_pick, 0, io);
            ch_msg = r == 1 ? std::string(phr_("ch_applied")) : r == 0 ? std::string(phr_("ch_lost")) : std::string(phr_("ch_fail"));
            ch_op(7, 0, 0, io); ch_lines_of(io); if (ch_pick >= (int)ch_lines.size()) ch_pick = 0; }
        else if (k == UK_TRI && n > 0) { ch_op(10, ch_pick, 0, io); ch_op(7, 0, 0, io); ch_lines_of(io);
            if (ch_pick >= (int)ch_lines.size()) ch_pick = 0; ch_msg = phr_("ch_deleted"); }
    } else {
        if (k == UK_UP) ch_sel = (ch_sel + 5) % 6; else if (k == UK_DOWN) ch_sel = (ch_sel + 1) % 6;
        else if (k == UK_CANCEL) ch_lvl = 2;
        else if (k == UK_OK) {
            long long v = ch_num(ch_wval); std::string f = "f", w = "w";
            if (ch_sel == 5) ch_lvl = 2;
            else if (ch_sel == 4) { ch_buf = ch_wval; ch_tgt = 1; ch_pad = 0; ch_lvl = 1; }
            else if (v < 0) ch_msg = phr_("ch_noval");
            else if (ch_sel == 0) ch_msg = ch_op(4, ch_pick, v, io) ? std::string(phr_("ch_done_w")) : std::string(phr_("ch_fail"));
            else if (ch_sel == 1) ch_msg = ch_op(5, ch_pick, v, io) ? std::string(phr_("ch_done_f")) : std::string(phr_("ch_fail"));
            else if (ch_sel == 2) ch_msg = ch_op(9, ch_pick, v, f) ? std::string(phr_("ch_done_s")) : std::string(phr_("ch_fail"));
            else if (ch_sel == 3) ch_msg = ch_op(9, ch_pick, v, w) ? std::string(phr_("ch_done_s")) : std::string(phr_("ch_fail"));
        }
    }
}
