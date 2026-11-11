#pragma once
/* screens/infopane.h -- #M4 (슨상 18:58 확정): 정보 칸 그리기 공용 함수. 작품 선택(A2, 인포 408 폭)과 메인메뉴(시안 A, 오른쪽 상자 304 폭)가 같이 쓴다.
   제목 = info_title · 캡처 칸 = info_cap(비율 유지 맞춤, #M3) · 오른쪽 열 = info_cols. Members of struct Ui, defined out of line. */

/* 제목 = 항상 한 줄(#M5, 슨상 19:12). big = 1.5배(작품 선택, 24px) / 아니면 1.0배 굵게(노랑) + 일본어 제목은 아래 작은 줄(메인메뉴, #M2 ①).
   big: 16px 글꼴 점마다 2x2 를 3/2 간격으로 찍어 획이 고르게(2px). 칸 폭을 넘으면 옆으로 흐른다(마키): 1초 쉼 → 왼쪽으로 → 끝에서 0.8초 → 처음. 넘치지 않으면 정지.
   같은 제목이면 이어서, 제목이 바뀌면 처음부터. 흐르기 코드는 kh_ui.h dline/mq_off 와 같은 규칙(hold0 1000 · hold1 800) — 속도만 상수 KH_MQ_MSPP(1px 당 ms)로 뺐다.
   big 의 일본어 제목(처리안, #M5 3번): 한 줄에 다 들어가면 한글 제목 바로 뒤. 아니면 칸 오른쪽 끝에 고정하고 한글 제목이 그 앞에서 잘려 흐른다(겹침 없음). 일본어 제목이 칸 폭의 1/3 보다 길면 생략. */
static const int KH_MQ_MSPP = 20;   /* 1px 당 ms (20 → 초당 50px, #M5 40~60) */
static const int KH_MQ_HOLD0 = 1000, KH_MQ_HOLD1 = 800;
inline void Ui::info_title(Canvas & c, const UiGame & gb, bool blk, int X, int W, bool big) {
    const uint32_t tc = blk ? WH : YL;
    const std::string & ti = gb.title;
    const int w1 = tw(ti), sw = big ? w1 * 3 / 2 : w1 + 1, ty = big ? 36 : 34;   /* sw = 그려지는 가로 길이 */
    const int jw = (big && !gb.title_jp.empty()) ? tw(gb.title_jp) : 0;
    int tavail = W; bool jp_right = false;   /* tavail = 제목이 쓸 수 있는 폭 */
    if (jw > 0 && sw + 20 + jw > W && jw * 3 <= W) { tavail = W - jw - 16; jp_right = true; }
    static std::string lk; static int since = 0;
    { const std::string key = ti + (big ? "|B" : "|S"); if (key != lk) { lk = key; since = ts_ms; } }
    int off = 0;
    if (sw > tavail) {
        const int maxoff = sw - tavail + 8, span = maxoff * KH_MQ_MSPP, cyc = KH_MQ_HOLD0 + span + KH_MQ_HOLD1;
        int e = ts_ms - since; if (e < 0) e = 0; const int tcy = e % cyc;
        off = tcy < KH_MQ_HOLD0 ? 0 : tcy < KH_MQ_HOLD0 + span ? (tcy - KH_MQ_HOLD0) / KH_MQ_MSPP : maxoff;
    }
    if (w1 > 0) {
        std::vector<uint32_t> buf((size_t)w1 * 16, 0u); Canvas t; t.p = buf.data(); t.w = w1; t.h = 16; text(t, 0, 0, ti, 1u, NOBG);
        const int pw = big ? 2 : 2, ph = big ? 2 : 1;   /* 한 점 = big 2x2 / 굵게 2x1(한 점 옆에 한 번 더) */
        for (int j = 0; j < 16; j++) for (int i = 0; i < w1; i++) if (buf[(size_t)j * w1 + i]) {
            const int px = X + (big ? i * 3 / 2 : i) - off;
            if (px < X || px + pw > X + tavail) continue;   /* 칸 밖은 그리지 않는다(바깥 clip 을 건드리지 않으려고 점마다 거른다) */
            c.rect(px, ty + (big ? j * 3 / 2 : j), pw, ph, tc); }
    }
    if (!big) { if (!gb.title_jp.empty()) text(c, X, ty + 18, fit(gb.title_jp, W), CY, K); return; }
    if (jw > 0) { if (jp_right) text(c, X + W - jw, 44, gb.title_jp, CY, K); else if (sw + 20 + jw <= W) text(c, X + sw + 20, 44, gb.title_jp, CY, K); }
}

/* 타이틀 캡처 칸(w x h). 마지막 저장 그림이 있으면 비율 유지로 맞춰 그리고(남는 곳은 검정), 없으면 안내 글. g 가 NULL 이면 빈 칸. */
inline void Ui::info_cap(Canvas & c, const UiGame * g, int x, int y, int w, int h) {
    c.rect(x - 1, y - 1, w + 2, h + 2, GY); c.rect(x, y, w, h, K);
    if (g && !g->thmb.empty()) blit_fit(c, g->thmb.data(), g->tw_, g->th_, x, y, w, h);
    else { const char * m = phr_("타이틀 캡처 없음"); text(c, x + (w - tw(m)) / 2, y + h / 2 - 8, m, GY, K); }
}

/* 오른쪽 열: 발매사 · 연도 · 장르 · (디스크 · 제작 — rows == 6 일 때만) · 판정/사유/안내. RX = 라벨 x, RV = 값 x, RW = 값 폭, ry = 첫 줄 y. dim = 작품 없음(값 회색). */
inline void Ui::info_cols(Canvas & c, const UiGame & gb, const UiGame & g, bool blk, const std::string & bk, int RX, int RV, int RW, int ry, int rows, bool dim) {
    const uint32_t vc = dim ? GY : WH;
    auto jw = [](const std::string & s) { size_t p = s.find(" ("), q = s.find(" · "), m = p < q ? p : q; return m == std::string::npos ? s : s.substr(0, m); };   /* 괄호·점 뒤 설명은 아래 안내 상자로 — 칸에는 짧은 낱말만 */
    auto rr = [&](const char * lab, const std::string & v, uint32_t col) { text(c, RX, ry, lab, CY, K); text(c, RV, ry, fit(v.empty() ? std::string("-") : v, RW), col, K); ry += 20; };
    rr(phr_("발매사"), gb.company, vc); rr(phr_("연도"), gb.year, vc); rr(phr_("장르"), gb.genre, vc);
    if (rows >= 6) {
        { std::string dk = g.disknote.empty() ? std::string("-") : g.disknote;   /* 칸이 좁으면 「FD 2장 + HDD」→「FD 2장+HDD」→「FD 2+HDD」 */
          if (tw(dk) > RW) { size_t p; while ((p = dk.find(" + ")) != std::string::npos) dk.replace(p, 3, "+"); }
          if (tw(dk) > RW) { size_t p = dk.find("장"); if (p != std::string::npos) dk.erase(p, std::string("장").size()); }
          rr(phr_("디스크"), dk, vc); }
        rr(phr_("제작"), gb.patch, vc);
    }
    if (blk) rr(phr_("사유"), bk, OR); else if (!dn_cur().empty()) rr(phr_("안내"), jw(dn_cur()), OR); else rr(phr_("판정"), g.jshort.empty() ? std::string("-") : jw(g.jshort), vc);
    if (rows >= 6 && !blk) {   /* #L5/#L6: 기동 줄 — 작품 선택 화면에서만(메인 정보 칸은 4줄). 실행 불가 사유가 뜨는 작품은 같은 자리 겹침 방지로 안 그린다. 7줄×20 = 캡처 칸 높이 140 */
        const std::string & st = gb.bootst; std::string v; uint32_t col = GY;
        if (st == "NA") return;   /* extras·ROM 은 줄 없음 */
        if (st == "OK") { v = gb.bootconf == "눈" ? phr_("바로 됨") : phr_("바로됨(자동)"); col = WH; }
        else if (st == "SYS") { v = phr_("시스템 필요"); col = OR; }
        else if (st == "HDI") { v = phr_("HD 설치 필요"); col = OR; }
        else if (st == "NOSYS") { v = phr_("시스템 없음"); col = OR; }
        else if (st == "FAIL") { v = phr_("안 됨"); col = GY; }
        else { v = phr_("미확인"); col = GY; }
        rr(phr_("기동"), v, col);
    }
}

/* 메인메뉴 오른쪽 상자 = 「지금 고른 작품」 (#M1 시안 A). 상자 (320,24) 304x224, 안쪽 x=344 폭 276. 작품 선택 화면과 같은 제목·캡처·열 함수를 쓴다. */
inline void Ui::d_main_info(Canvas & c) {
    rbox(c, 320, 24, 304, 224, WH, phr_("인포메이션"));
    const UiGame * gm = game(); const int X = 344, W = 276, CAPY = 70, CW = 144, CH = 90;
    static const UiGame none = UiGame();
    const UiGame & g = gm ? *gm : none; const bool blk = gm && !blk_cur().empty(); const std::string bk = blk_cur();
    int LB = 0; { const char * const lb[6] = { phr_("발매사"), phr_("연도"), phr_("장르"), phr_("판정"), phr_("사유"), phr_("안내") }; for (int q = 0; q < 6; q++) { int t = tw(lb[q]) + 4; if (t > LB) LB = t; } }
    const int RX = X + CW + 8, RV = RX + LB, RW = X + W - RV;
    if (!gm) text(c, X, 34, phr_("(고른 작품 없음)"), GY, K); else info_title(c, g, blk, X, W, false);
    info_cap(c, gm, X, CAPY, CW, CH);
    info_cols(c, g, g, blk, bk, RX, RV, RW, CAPY + 2, 4, !gm);
    if (!gm) { text(c, X, 178, phr_("작품 선택에서 작품을 고르면"), GY, K); text(c, X, 198, phr_("여기에 정보가 나옵니다."), GY, K); return; }
    int L2 = 0; { const char * const lb[3] = { phr_("플레이"), phr_("저장"), phr_("실행") }; for (int q = 0; q < 3; q++) { int t = tw(lb[q]) + 4; if (t > L2) L2 = t; } }
    int y = CAPY + CH + 10;
    text(c, X, y, phr_("플레이"), CY, K); text(c, X + L2, y, fit(play_str(g.play_s), W - L2), WH, K); y += 20;
    { std::string st = phr_("없음"); if (g.lastslot >= 0 && g.slot[g.lastslot].used) st = phr_("슬롯 ") + slot_lbl(g.lastslot) + "  " + g.slot[g.lastslot].stamp;
      text(c, X, y, phr_("저장"), CY, K); text(c, X + L2, y, fit(st, W - L2), WH, K); y += 20; }
    { const int gi = cur; int ei = row_ent(gi); std::string ev; uint32_t col = WH;
      if (ei < 0) ev = phr_("자동 판정"); else { std::vector<int> v = tvents(gi); int idx = 0; for (size_t q = 0; q < v.size(); q++) if (v[q] == ei) idx = (int)q; const UiEnt & e = g.ents[ei];
        ev = e.label + " (" + std::to_string(idx + 1) + "/" + std::to_string((int)v.size()) + ")"; if (ent_warn(gi, ei)) col = OR; }
      text(c, X, y, phr_("실행"), CY, K); text(c, X + L2, y, fit(ev, W - L2), col, K); }
}
