#pragma once
/* screens/pick.h -- S8a: S_PICK screen (v0.6.8 A2 레이아웃: 목록 192 폭 · 인포 408 폭 · 타이틀 캡처 224x140 y66 고정(#M3·#M5, 비율 유지) · 제목 한 줄+흐르기(#M5) · 좌우 상자 세로 길이 동일). Members of struct Ui, defined out of line. */
inline void Ui::d_pick(Canvas & c) {
    struct Fx_ { Fx_() { g_help_fix2 = true; } ~Fx_() { g_help_fix2 = false; } } fx_;   /* t855 틀 고정: 키 도움말 두 줄 자리 고정 */
    titlebar(c, phr_("작품 선택"));
    std::string bt = std::string(sortfav ? "★" : "") + sort_name(sortm) + (reorder ? phr_("  [순서 이동]") : "");   /* t851: 「작품 선택」 머리말은 위 제목줄에 이미 있어 뺀다 */
    rbox(c, 16, 24, 232, 272, WH, bt.c_str());
    { int x = 26, nun = 0; for (size_t q = 0; q < games.size(); q++) if (games[q].variant_of.empty() && games[q].lang.empty()) nun++; std::string nms[4];
      const int LIM = 212;   /* 목록 상자 192 폭 안쪽 (x 26 ~ 198). t851: 탭 폭 = 글자폭+8, 간격 2 를 정확히 더해 넘치면 단계별로 줄인다 */
      auto setn = [&](int mode) { for (int t = 0; t < 4; t++) { std::string b = tab_name(t); if (mode >= 1) { static const char * sk[4] = { "tab_s_kp", "tab_s_orig", "tab_s_un", "tab_s_all" }; const char * sn = phr_(sk[t]); if (strcmp(sn, sk[t]) != 0) b = sn; }
              std::string suf = (t == TAB_UN && nun > 0) ? "(" + std::to_string(nun) + ")" : std::string();
              if (mode >= 2) { size_t i = 0; int n = 0; const int keep = mode >= 3 ? 1 : 2; while (i < b.size() && n < keep) { i++; while (i < b.size() && ((unsigned char)b[i] & 0xC0) == 0x80) i++; n++; } b = b.substr(0, i); }
              if (mode >= 4) suf.clear();
              nms[t] = b + suf; } };
      auto tot = [&]() { int s = 0; for (int t = 0; t < 4; t++) s += tw(nms[t]) + 8 + 2; return s - 2; };
      for (int mode = 0; mode <= 4; mode++) { setn(mode); if (tot() <= LIM) break; }
      if (g_ovf_on) ovf_box(16, 42, 232, 24);
      for (int t = 0; t < 4; t++) { const std::string & nm = nms[t]; int w = tw(nm) + 8; if (t == tab) { c.rect(x - 2, 42, w, 21, WH); text(c, x + 2, 45, nm, K, WH); } else text(c, x + 2, 45, nm, CY, K); x += w + 2; } ovf_nobox(); }
    int nv = (int)vis.size(), n = nv + 2;   /* #1478: nv = 작품 추가 · nv+1 = 이 작품 ▶ */
    if (pick != pick_seen) { pick_seen = pick; pick_since = ts_ms; }
    const int VIS = 9, PT = 24, LX = 32, LWD = 214, AVAIL = 194, Y0 = 70; int top = pick - 4; if (top > n - VIS) top = n - VIS; if (top < 0) top = 0;
    for (int k = 0; k < VIS && top + k < n; k++) {
        int i = top + k, yy = Y0 + k * PT; bool add = i >= nv; int gi = add ? -1 : vis[i]; const bool ownr = i == nv + 1;
        bool d = !add && !blk_row(gi).empty(), fv = !add && games[gi].fav; int wn = add ? 0 : row_warn(gi);
        int s0 = fv ? 16 : 0, ww = wn ? 16 : 0, av = AVAIL - s0 - ww; std::string full = ownr ? std::string(phr_("이 작품  >")) : add ? std::string(phr_("+ 작품 추가")) : games[gi].title; bool sl = i == pick;
        { Hit ht = { LX - 8, yy - 3, LWD + 2, 22, i, pick }; hits.push_back(ht); }   /* t55a */
        if (sl) c.rect(LX - 8, yy - 3, LWD + 2, 22, d ? GY : WH);
        if (fv) text(c, LX, yy, "★", sl ? K : YL, sl ? (d ? GY : WH) : K);
        if (wn) text(c, LX + AVAIL - 16, yy, "※", sl ? K : OR, sl ? (d ? GY : WH) : K);
        c.clip(LX + s0, yy - 3, av, 22);
        { std::string shown = add ? full : ttl_fit(games[gi], av);   /* 글자 단위로 자르고 …  (선택 줄도 같은 방식 - 반쪽 글자가 보이지 않게) */
          if (sl) { const std::string ft = add ? full : games[gi].title; const int o_ = mq_calc(tw(ft), av, pick_since); text(c, LX + s0 - o_, yy, o_ ? ft : shown, K, d ? GY : WH); } else text(c, LX + s0, yy, shown, d ? GY : CY, K); }   /* t851: 선택 줄은 길면 흐른다 (인포 제목과 같은 규칙) */
        c.unclip();
    }
    if (top > 0) text(c, 232, 30, "↑", CY, K); if (top + VIS < n) text(c, 232, 278, "↓", CY, K);
    if (pick == nv + 1) {   /* #1478: 이 작품 ▶ */
        rbox(c, 256, 24, 368, 272, WH, phr_("인포메이션"));
        const UiGame * og = game(); text(c, 272, 48, og ? ttl_fit(*og, 336) : std::string(), CY, K);
        { const std::vector<std::string> w_ = wrap_text(phr_("이 작품의 실행 방법·설정·언어·즐겨찾기·") + std::string(" ") + phr_("슬롯 규칙·원본으로 초기화를 엽니다."), 336, 3); for (size_t q_ = 0; q_ < w_.size(); q_++) text(c, 272, 78 + (int)q_ * 20, w_[q_], WH, K); }
        const std::string kk = phr_("↑↓:선택, ○:이 작품 화면, ×:취소");
        desc_box(c, NULL, phr_("이 작품  >: 현재 작품의 실행 방법 · 이 작품 설정 · 언어 · 즐겨찾기"), kk); helpline(c, kk); return;
    }
    if (pick >= nv) {
        rbox(c, 256, 24, 368, 272, WH, phr_("인포메이션"));
        text(c, 272, 48, phr_("새 작품을 등록합니다."), YL, K);
        text(c, 272, 78, phr_("이미지 파일(zip 포함)이나 폴더를"), WH, K); text(c, 272, 98, phr_("고르면 형식과 구조를 판정해"), WH, K); text(c, 272, 118, phr_("자동으로 등록합니다."), WH, K);
        text(c, 272, 158, phr_("게임 폴더를 지정해 두면 켤 때마다"), CY, K); text(c, 272, 178, phr_("새 작품을 찾아 자동 등록합니다."), CY, K);
        const std::string kk = phr_("↑↓:선택, ○:작품 추가 화면, ×:취소");
        desc_box(c, NULL, std::string(phr_("+ 작품 추가: 이미지 파일 · 폴더 · 게임 폴더 지정. ")) + phr_("회색 작품은 아직 실행할 수 없는 작품입니다 (사유는 인포메이션)."), kk); helpline(c, kk); return;
    }
    int gi = vis[pick]; const UiGame & gb = games[gi]; const UiGame & g = games[(cur >= 0 && cur < (int)games.size()) ? cur : gi];
    info_pane(c, &gb, gi);   /* t855 시안 v3: 제목 · 캡처 200x125 · 2열 4행 (screens/infopane.h) - 메인과 같은 함수 */
    const std::string kk = reorder ? phr_("↑↓:순서 이동, ○ 또는 ×:끝") : tvents(gi).size() > 1 ? phr_("↑↓:작품, ←→:페이지, L R:탭, ○:정함, □:편집, △:정렬, R3:실행 방법, SEL:부가, ×") : phr_("↑↓:작품, ←→:페이지, L R:탭, ○:정함, □:편집, △:정렬, SEL:부가, ×");
    const bool shown = desc_box(c, &gb, phr_("목록에서 작품을 고르면 위 정보가 바뀝니다. ○ 로 시작하고, 마지막 저장이 있으면 이어하기를 고를 수 있습니다."), kk);
    helpline(c, kk);
    if (shown && !reorder) help_right(c, kk, (g.lastslot >= 0 && g.slot[g.lastslot].used ? std::string(phr_("마지막 저장 ")) + g.slot[g.lastslot].stamp : std::string(phr_("처음부터 시작합니다"))) + phr_(" · 이 작품의 설정은 맨 아래"));
}
inline void Ui::k_pick(UKey k, Act & a) {
    do { {
            int nv = (int)vis.size();
            if (reorder) { if (k == UK_OK || k == UK_CANCEL) { reorder = false; dirty = true; a.t = A_APPLY; } break; }
            if (k == UK_L || k == UK_R) { int keep = cur; tab = (tab + (k == UK_R ? 1 : 3)) % 4; rebuild_vis(keep); dirty = true; break; }
            if (k == UK_LEFT || k == UK_RIGHT) { int p2 = pick + (k == UK_RIGHT ? 9 : -9); if (p2 > nv + 1) p2 = nv + 1; if (p2 < 0) p2 = 0; if (p2 != pick) { pick = p2; if (pick < nv) sync_cur(); } break; }   /* v0.6.8: ←→ = 한 화면(10줄)씩 페이지 넘김, 끝에서는 처음/마지막 항목에서 멈춤 */
            if (pick >= nv) { if (k == UK_OK) { if (pick == nv) go(S_ADD, 0); else if (game() && !game()->id.empty()) { go(S_EXTRA, 0); back = S_PICK; } } else if (k == UK_CANCEL) go(S_MAIN, 0); break; }
            int gi = vis[pick];
            if (k == UK_PROF) { std::vector<int> v = tvents(gi); if (v.size() > 1) { int ei = row_ent(gi), idx = 0; for (size_t q = 0; q < v.size(); q++) if (v[q] == ei) idx = (int)q; idx = (idx + (k == UK_RIGHT ? 1 : (int)v.size() - 1)) % (int)v.size(); if ((int)rent.size() <= gi) rent.resize(games.size(), -1); rent[gi] = v[idx]; sync_cur(); } break; }
            if (k == UK_OK && games[gi].nosrc) { open_tidy_for(a, games[gi].id); }   /* #144-2: 원본 없음 = 실행 대신 목록 정리(다시 찾기 / 빼기) */
            else if (k == UK_OK) { sync_cur(); if (!game() || game()->id.empty()) { mainsel = 0; go(S_MAIN, 0); } else { start_ret = S_PICK; if (has_save(game()) || !blk_cur().empty()) { startsel = has_save(game()) ? 0 : 1; go(S_START, startsel); } else { startsel = 1; start_ok(a); } } }   /* t52i (B1): 홈 = 작품 목록, ○ = 시작 */
            else if (k == UK_CANCEL) go(S_MAIN, 0);
            else if (k == UK_SELECT) { sync_cur(); go(S_EXTRA, 0); back = S_PICK; }
            else if (k == UK_SQ) { entg = gi; ent_from_extra = false; msg.clear(); std::vector<int> v = vents(gi); int ei = row_ent(gi), idx = 0; for (size_t q = 0; q < v.size(); q++) if (v[q] == ei) idx = (int)q; scr = S_ENT; sel = idx; }
            else if (k == UK_TRI) { sel = sortm; scr = S_SORT; }
            break; }
    } while (0);
}
