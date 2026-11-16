#pragma once
/* screens/extra.h -- S_EXTRA = 「이 작품」 (메뉴 정리안 1안, 슨상 결정 2026-11-16 14:35 「네 추천 안대로」).
   옛 「부가」를 해체: 조작 안내·정보·라이선스 → 설정(6·7번째 줄), 적용값 보기 → 각 설정 줄의 출처 꼬리표로 대체.
   줄은 번호가 아니라 id 로 찾는다 (Ui::extra_ids): 10 실행 방법 · 11 이 작품 설정 · 8 언어 · 5 즐겨찾기 · 2 부팅 시퀀스 다시 보기 · 12 슬롯 규칙 · 6 원본으로 초기화 */
inline std::string ex_rule_label(const Ui & u, const std::string & r) {
    if (!u.ex_rule_cmd) return phr_("해당 없음 (.cmd 없음)");
    return r == "ini" ? phr_("game.ini 규칙 (.cmd 무시)") : r == "cmd" ? phr_(".cmd 규칙 (고정)") : phr_(".cmd 규칙 (자동: .cmd 가 있으면 우선)");
}
inline void Ui::d_extra(Canvas & c) {
    titlebar(c, phr_("이 작품"));
    const UiGame * gm = game();
    rbox(c, 16, 24, 288, 244, WH, gm ? ttl_suf(*gm, phr_("이 작품"), 256).c_str() : phr_("이 작품"));
    const std::string fv = std::string(phr_("즐겨찾기: ")) + (games.empty() || !games[base_of(cur)].fav ? phr_("☆ 끔") : phr_("★ 켬"));
    ex_rule_cmd = false;   /* .cmd 가 이 작품 폴더에 있나 (있어야 슬롯 규칙을 고를 수 있다) */
    if (gm && ::g_has_cmd_fn) {   /* t822: .cmd 찾는 곳 = 이미지 폴더 → 게임 폴더 → srcdir (실행 쪽 gh_apply_cmd 와 같게) · 작품 바뀔 때만 다시 */
        static int c_cur = -2; static std::string c_dir; static bool c_res = false; static std::string c_rule; static int c_ep = -1; static decltype(ts_ms) c_t = 0;
        if (c_cur != cur || c_ep != ex_rule_epoch || c_dir != imgdir_of(cur) || ts_ms - c_t > 1500 || ts_ms < c_t) { c_ep = ex_rule_epoch; c_cur = cur; c_dir = imgdir_of(cur); c_t = ts_ms; c_rule.clear(); bool r = ::g_has_cmd_fn(c_dir) || ::g_has_cmd_fn(games[cur].dir);
            if (env.resolved) { ent::Kv kv; if (env.resolved(cur, kv)) { { auto ir = kv.find("slot_rule"); if (ir != kv.end()) c_rule = ir->second; } auto ic = kv.find("cmdfile"); if (ic != kv.end() && !ic->second.empty() && ic->second != "-") r = true; auto is = kv.find("srcdir"); if (!r && is != kv.end() && !is->second.empty() && is->second != "-" && ::g_has_cmd_fn(is->second)) r = true; } }   /* 실행 쪽 해석이 기록한 cmdfile·srcdir */
            if (!r && env.values) { std::vector<ValRow> vr; if (env.values(cur, vr)) for (auto & q : vr) { if (q.val.empty() || q.val[0] == '(') continue; if ((q.key == "srcdir" && ::g_has_cmd_fn(q.val)) || q.key == "cmdfile") r = true; } }
            c_res = r; }
        ex_rule_cmd = c_res; ex_rule_str = c_rule; }
    std::vector<std::string> a = { phr_("실행 방법  >"), phr_("이 작품 설정  >"), phr_("lang_set"), fv, phr_("부팅 시퀀스 다시 보기"), std::string(phr_("슬롯 규칙")) + ": " + ex_rule_label(*this, ex_rule_str), phr_("원본으로 초기화") };
    std::vector<bool> dm(a.size(), false); dm[5] = !ex_rule_cmd; items(c, 40, 44, 256, a, sel, 24, &dm);
    rbox(c, 320, 24, 304, 244, WH, phr_("인포메이션"));
    if (gm) { std::string l[4] = { gm->disk[0], gm->disk[1], "", gm->boot_note }; for (int i = 0; i < 4; i++) if (!l[i].empty()) text(c, 344, 44 + i * 22, l[i], i == 3 ? CY : WH, K); }
    if (!save_note.empty() && ts_ms < save_note_until) { c.rect(336, 244, 280, 18, K); text(c, 344, 246, fit(save_note, 270), YL, K); }   /* t825: 저장함 한 줄 */
    static const char * dsc[] = { "디스크·CD·자동 입력·글꼴 등 이 작품이 실행되는 방법을 보고 고칩니다.", "화면·소리·조작·시스템을 이 작품만 따로 정합니다. (바꾼 줄은 노랑 ●)", "이 작품의 언어(글꼴 자동 선택)를 정합니다.", "작품 목록에서 먼저 보이게 합니다.", "부팅 연출을 다시 봅니다.", ".cmd 파일이 있을 때 디스크 배치를 .cmd 로 할지 game.ini 로 할지 고릅니다.", "게임 안 세이브가 지워지고 디스크가 원본 상태로 돌아갑니다." };
    desc(c, 276, 56, { phr_(dsc[sel >= 0 && sel < 7 ? sel : 0]) });
    helpline(c, phr_("↑↓:선택, ○:결정, ×:취소"));
}
inline void Ui::k_extra(UKey k, Act & a) {
    const std::vector<int> ids = extra_ids(); const int id = (sel >= 0 && sel < (int)ids.size()) ? ids[sel] : -1;
    if (k == UK_OK) {
        if (id == 10) { const int gi = base_of(cur); if (gi >= 0 && gi < (int)games.size()) { entg = gi; msg.clear(); std::vector<int> v = vents(gi); int ei = row_ent(gi), idx = 0; for (size_t q = 0; q < v.size(); q++) if (v[q] == ei) idx = (int)q; ent_from_extra = true; scr = S_ENT; sel = idx; } }   /* 실행 방법 = 작품 선택 □ 와 같은 목록 (× = 이 작품으로 돌아옴) */
        else if (id == 11) { scope = 1; scope_lock = 1; conf_grp = 0; enter_conf_cfg(); go(S_SET, 0); }   /* 이 작품 설정 = 설정 허브, 범위 「이 작품」 고정 */
        else if (id == 8) { int b = base_of(cur); if (b >= 0 && b < (int)games.size()) open_lang(std::vector<std::string>(1, games[b].id), std::vector<std::string>(), S_EXTRA); }   /* build 3: language per work */
        else if (id == 5) { int keep = cur, b = base_of(cur); if (b >= 0 && b < (int)games.size()) { games[b].fav = !games[b].fav; dirty = true; rebuild_vis(keep); } }
        else if (id == 12) {   /* 슬롯 규칙: "" → cmd → ini 순환, 작품 단위 (user.ini [game] slot_rule) */
            if (ex_rule_cmd && env.setkey && cur >= 0) {
                ent::Kv kv; std::string r; if (env.resolved && env.resolved(cur, kv)) { auto it = kv.find("slot_rule"); if (it != kv.end()) r = it->second; }
                const std::string nx = r.empty() ? "cmd" : r == "cmd" ? "ini" : ""; env.setkey(cur, "slot_rule", nx); if (env.reload) env.reload(cur); ex_rule_epoch++;
                save_note = std::string(phr_("저장함: 슬롯 규칙 = ")) + (nx == "ini" ? "game.ini" : nx == "cmd" ? ".cmd(고정)" : ".cmd(자동)") + phr_(" (이 작품)"); save_note_until = ts_ms + 2500; } }
        else if (id == 6) yn_open(S_EXTRA, YN_DISKRESET, phr_("게임 안 세이브가 지워지고 디스크가"), phr_("원본 상태로 돌아갑니다 (이전 사본은 .bak)."), phr_("계속할까요?"));   /* t52e (#131 2) */
        else if (id == 2) { a.t = A_EXTRA; a.arg = 2; a.game = cur; }   /* 부팅 시퀀스 다시 보기 (호스트) */
    } else if (k == UK_CANCEL) go(back == S_PICK ? S_PICK : S_MAIN, 0);
}
