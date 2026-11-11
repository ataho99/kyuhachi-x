#pragma once
/* screens/pick.h -- S8a: S_PICK screen (v0.6.8 A2 레이아웃: 목록 192 폭 · 인포 408 폭 · 타이틀 캡처 224x140 y66 고정(#M3·#M5, 비율 유지) · 제목 한 줄+흐르기(#M5) · 좌우 상자 세로 길이 동일). Members of struct Ui, defined out of line. */
inline void Ui::d_pick(Canvas & c) {
    titlebar(c, phr_("작품 선택"));
    std::string bt = std::string(phr_("작품 선택  ")) + (sortfav ? "★" : "") + sort_name(sortm) + (reorder ? phr_("  [순서 이동]") : "");
    rbox(c, 16, 24, 192, 284, WH, bt.c_str());
    { int x = 26, nun = 0; for (size_t q = 0; q < games.size(); q++) if (games[q].variant_of.empty() && games[q].lang.empty()) nun++; std::string nms[4]; int tot = 0; for (int t = 0; t < 4; t++) { nms[t] = tab_name(t); if (t == TAB_UN && nun > 0) nms[t] += "(" + std::to_string(nun) + ")"; tot += tw(nms[t]) + 10; }   /* t672 (#656): 탭 줄이 넘치면 짧은 이름 (목록 폭 192 → 한계 172) */
      if (tot > 172) { static const char * sk[4] = { "tab_s_kp", "tab_s_orig", "tab_s_un", "tab_s_all" }; for (int t = 0; t < 4; t++) { const char * sn = phr_(sk[t]); if (strcmp(sn, sk[t]) != 0) { nms[t] = sn; if (t == TAB_UN && nun > 0) nms[t] += "(" + std::to_string(nun) + ")"; } } }
      { auto cut2 = [](const std::string & s) { size_t p = s.find('('); std::string b = s.substr(0, p), suf = p == std::string::npos ? std::string() : s.substr(p); size_t i = 0; int n = 0; while (i < b.size() && n < 2) { i++; while (i < b.size() && ((unsigned char)b[i] & 0xC0) == 0x80) i++; n++; } return b.substr(0, i) + suf; };   /* 그래도 넘치면 앞 두 글자만 (ko 는 짧은 이름 문구가 없다) */
        int t2 = 0; for (int t = 0; t < 4; t++) t2 += tw(nms[t]) + 10; if (t2 > 172) for (int t = 0; t < 4; t++) nms[t] = cut2(nms[t]); }
      for (int t = 0; t < 4; t++) { const std::string & nm = nms[t]; int w = tw(nm) + 8; if (t == tab) { c.rect(x - 2, 42, w, 21, WH); text(c, x + 2, 45, nm, K, WH); } else text(c, x + 2, 45, nm, CY, K); x += w + 2; } }
    int nv = (int)vis.size(), n = nv + 1;
    if (pick != pick_seen) { pick_seen = pick; pick_since = ts_ms; }
    const int VIS = 10, PT = 24, LX = 32, LWD = 168, AVAIL = 148, Y0 = 70; int top = pick - 4; if (top > n - VIS) top = n - VIS; if (top < 0) top = 0;
    for (int k = 0; k < VIS && top + k < n; k++) {
        int i = top + k, yy = Y0 + k * PT; bool add = i >= nv; int gi = add ? -1 : vis[i];
        bool d = !add && !blk_row(gi).empty(), fv = !add && games[gi].fav; int wn = add ? 0 : row_warn(gi);
        int s0 = fv ? 16 : 0, ww = wn ? 16 : 0, av = AVAIL - s0 - ww; std::string full = add ? std::string(phr_("+ 작품 추가")) : games[gi].title; bool sl = i == pick;
        { Hit ht = { LX - 8, yy - 3, LWD + 2, 22, i, pick }; hits.push_back(ht); }   /* t55a */
        if (sl) c.rect(LX - 8, yy - 3, LWD + 2, 22, d ? GY : WH);
        if (fv) text(c, LX, yy, "★", sl ? K : YL, sl ? (d ? GY : WH) : K);
        if (wn) text(c, LX + AVAIL - 16, yy, "※", sl ? K : OR, sl ? (d ? GY : WH) : K);
        c.clip(LX + s0, yy - 3, av, 22);
        { std::string shown = add ? full : ttl_fit(games[gi], av);   /* 글자 단위로 자르고 …  (선택 줄도 같은 방식 — 반쪽 글자가 보이지 않게) */
          if (sl) text(c, LX + s0, yy, shown, K, d ? GY : WH); else text(c, LX + s0, yy, shown, d ? GY : CY, K); }
        c.unclip();
    }
    if (top > 0) text(c, 188, 30, "↑", CY, K); if (top + VIS < n) text(c, 188, 288, "↓", CY, K);
    rbox(c, 216, 24, 408, 284, WH, phr_("인포메이션"));
    if (pick >= nv) {
        text(c, 240, 48, phr_("새 작품을 등록합니다."), YL, K);
        text(c, 240, 78, phr_("이미지 파일(zip 포함)이나 폴더를"), WH, K); text(c, 240, 98, phr_("고르면 형식과 구조를 판정해"), WH, K); text(c, 240, 118, phr_("자동으로 등록합니다."), WH, K);
        text(c, 240, 158, phr_("게임 폴더를 지정해 두면 켤 때마다"), CY, K); text(c, 240, 178, phr_("새 작품을 찾아 자동 등록합니다."), CY, K);
        desc(c, 316, 50, { phr_("+ 작품 추가: 이미지 파일 · 폴더 · 게임 폴더 지정"), phr_("회색 작품은 아직 실행할 수 없는 작품입니다 (사유는 인포메이션).") });
        helpline(c, phr_("↑↓:선택, ○:작품 추가 화면, ×:취소")); return;
    }
    int gi = vis[pick]; const UiGame & gb = games[gi]; const UiGame & g = games[(cur >= 0 && cur < (int)games.size()) ? cur : gi]; const std::string & bk = blk_cur(); bool blk = !bk.empty();
    const int IX = 232, IW = 376; int LB = 48;   /* t52h (#134): 라벨 칸 폭 = 가장 긴 라벨 + 8 */
    { const char * const lb[8] = { phr_("발매사"), phr_("연도"), phr_("장르"), phr_("디스크"), phr_("제작"), phr_("판정"), phr_("사유"), phr_("안내") }; for (int q = 0; q < 8; q++) { int t = tw(lb[q]) + 8; if (t > LB) LB = t; } }
    const int RX = IX + 232, RV = RX + LB, RW = IX + IW + 8 - RV;   /* 오른쪽 열 (상자 안쪽 끝까지) */
    c.clip(IX - 4, 28, IW + 8, 276);
    info_title(c, gb, blk, IX, IW, true);   /* #M4: 제목·캡처·오른쪽 열 = 메인메뉴와 같이 쓰는 함수(screens/infopane.h). 제목 1.5배 2줄 */
    const int CAPY = 66, CW = 224, CH = 140;   /* #M3·#M5: 캡처 칸 = 한 줄 제목 아래 한 자리(y66)·큰 쪽 224x140(1.6:1) 고정, 그림은 비율 유지(blit_fit) */
    info_cap(c, &g, IX, CAPY, CW, CH);
    info_cols(c, gb, g, blk, bk, RX, RV, RW, CAPY + 4, 6, false);
    { int y = CAPY + CH + 8; for (int q = 0, nl = 0; q < 3 && nl < 2; q++) if (!gb.intro[q].empty()) { text(c, IX, y + nl * 18, fit(gb.intro[q], IW), WH, K); nl++; } }
    prof_w = 0; { int ei = row_ent(gi); std::string ev; uint32_t col = WH; bool multi = false; const int RY = CAPY + CH + 46;
      if (ei < 0) ev = phr_("자동 판정"); else { std::vector<int> v = tvents(gi); int idx = 0; for (size_t q = 0; q < v.size(); q++) if (v[q] == ei) idx = (int)q; const UiEnt & e = gb.ents[ei];
        ev = e.label + " (" + std::to_string(idx + 1) + "/" + std::to_string((int)v.size()) + ")"; if (ent_warn(gi, ei)) col = OR; multi = v.size() > 1; }
      text(c, IX, RY, phr_("실행"), CY, K); text(c, IX + LB, RY, fit(ev, IW - LB - 24), col, K);
      if (multi) { text(c, IX + IW - 16, RY, "▶", CY, K); prof_x = IX; prof_y = RY - 4; prof_w = IW; prof_h = 24; }   /* 방법 2개 이상: 줄 끝 ▶ · 이 줄을 누르면 다음 방법 */
      {   std::string st = phr_("없음"); if (g.lastslot >= 0 && g.slot[g.lastslot].used) st = phr_("슬롯 ") + slot_lbl(g.lastslot) + "  " + g.slot[g.lastslot].stamp;
          text(c, IX, RY + 18, phr_("저장"), CY, K); text(c, IX + LB, RY + 18, fit(st, IW - LB), WH, K); } }
    c.unclip();
    std::vector<std::string> d;
    if (blk) d = { fit_title(gb.title, 560 - tw(phr_("   [실행 불가]"))) + phr_("   [실행 불가]"), fit(phr_("사유: ") + bk, 560) };
    else { std::string suf = g.lastslot >= 0 && g.slot[g.lastslot].used ? phr_("   마지막 저장 ") + g.slot[g.lastslot].stamp : std::string(phr_("   처음부터 시작합니다")); d = { fit_title(gb.title, 560 - tw(suf)) + suf, fit(!dn_cur().empty() ? dn_cur() : g.judge.empty() ? (g.disk[0] + (g.disk[1].empty() ? std::string() : "    " + g.disk[1])) : g.judge, 560) }; }
    desc(c, 316, 50, d, blk ? 1 : -1);
    g_help_two = true;   /* 안내줄 높이 고정(두 줄 자리) — 작품마다 한 줄/두 줄이 달라 어긋나 보이던 것 */
    helpline(c, reorder ? phr_("↑↓:순서 이동, ○ 또는 ×:끝") : tvents(gi).size() > 1 ? phr_("↑↓:작품, ←→:페이지, L R:탭, ○:정함, □:편집, △:정렬, R3:실행 방법, SEL:부가, ×") : phr_("↑↓:작품, ←→:페이지, L R:탭, ○:정함, □:편집, △:정렬, SEL:부가, ×"));
}
inline void Ui::k_pick(UKey k, Act & a) {
    do { {
            int nv = (int)vis.size();
            if (reorder) { if (k == UK_OK || k == UK_CANCEL) { reorder = false; dirty = true; a.t = A_APPLY; } break; }
            if (k == UK_L || k == UK_R) { int keep = cur; tab = (tab + (k == UK_R ? 1 : 3)) % 4; rebuild_vis(keep); dirty = true; break; }
            if (k == UK_LEFT || k == UK_RIGHT) { int p2 = pick + (k == UK_RIGHT ? 10 : -10); if (p2 > nv) p2 = nv; if (p2 < 0) p2 = 0; if (p2 != pick) { pick = p2; if (pick < nv) sync_cur(); } break; }   /* v0.6.8: ←→ = 한 화면(10줄)씩 페이지 넘김, 끝에서는 처음/마지막 항목에서 멈춤 */
            if (pick >= nv) { if (k == UK_OK) go(S_ADD, 0); else if (k == UK_CANCEL) go(S_MAIN, 0); break; }
            int gi = vis[pick];
            if (k == UK_PROF) { std::vector<int> v = tvents(gi); if (v.size() > 1) { int ei = row_ent(gi), idx = 0; for (size_t q = 0; q < v.size(); q++) if (v[q] == ei) idx = (int)q; idx = (idx + (k == UK_RIGHT ? 1 : (int)v.size() - 1)) % (int)v.size(); if ((int)rent.size() <= gi) rent.resize(games.size(), -1); rent[gi] = v[idx]; sync_cur(); } break; }
            if (k == UK_OK && games[gi].nosrc) { open_tidy_for(a, games[gi].id); }   /* #144-2: 원본 없음 = 실행 대신 목록 정리(다시 찾기 / 빼기) */
            else if (k == UK_OK) { sync_cur(); if (!game() || game()->id.empty()) { mainsel = 0; go(S_MAIN, 0); } else { start_ret = S_PICK; if (has_save(game()) || !blk_cur().empty()) { startsel = has_save(game()) ? 0 : 1; go(S_START, startsel); } else { startsel = 1; start_ok(a); } } }   /* t52i (B1): 홈 = 작품 목록, ○ = 시작 */
            else if (k == UK_CANCEL) go(S_MAIN, 0);
            else if (k == UK_SELECT) { sync_cur(); go(S_EXTRA, 0); back = S_PICK; }
            else if (k == UK_SQ) { entg = gi; msg.clear(); std::vector<int> v = vents(gi); int ei = row_ent(gi), idx = 0; for (size_t q = 0; q < v.size(); q++) if (v[q] == ei) idx = (int)q; scr = S_ENT; sel = idx; }
            else if (k == UK_TRI) { sel = sortm; scr = S_SORT; }
            break; }
    } while (0);
}
