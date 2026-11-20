#pragma once
/* screens/conf.h -- S8b: S_CONF screen. #587: 범위 둘(전역/이 작품) · 이 작품에서는 전역 전용 줄(소리 버퍼·글꼴·언어)을 숨기고 맨 아래 「개별 설정 초기화」 · 개별 값 줄 앞 ● */
inline void Ui::d_conf(Canvas & c) {
    const std::string pgt = conf_grp == 1 ? std::string(phr_("사운드")) : conf_grp == 2 ? std::string(phr_("조작")) : conf_grp == 3 ? std::string(phr_("실행환경")) : conf_grp == 4 ? std::string(phr_("컨트롤러")) : conf_grp == 5 ? std::string(phr_("터치 조작")) : conf_grp == 6 ? std::string(phr_("오버레이")) : conf_grp == 7 ? std::string(phr_("성능 설정")) : conf_grp == 8 ? std::string(phr_("글꼴 설정")) : conf_grp == 9 ? std::string(phr_("배속 설정")) : conf_grp == 10 ? std::string(phr_("표시")) : std::string(phr_("소리/패드/배속 설정"));   /* #1478: 페이지 이름 */
    if (!tmini) titlebar(c, pgt.c_str());
    Settings & s = eff();
    if (!tmini) rbox(c, 16, 24, 288, 296, WH, conf_grp >= 2 ? pgt.c_str() : phr_("동작 환경 설정"));
    const std::vector<int> ri = rowids(S_CONF);
    const bool own = scope == 1 && cur >= 0 && cur < MAXG; const bool ind = own && ovm[cur] != 0u;
    static const unsigned rb[56] = { /*E1R*/ /* E3R */ 0u, 1u << 7, 1u << 8, 0u, (1u << 9) | (31u << 17),
        1u << 10, 1u << 11,   /* E3R */ 1u << 14, 1u << 12, 1u << 13, 1u << 15,
        0u, 0u, 0u, 0u, 0u, 1u << 23,   /* E3R */ 1u << 24, 1u << 25, 1u << 26, 1u << 27, 1u << 28, 0u, 0u, 0u, 0u,
        0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u << 29, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u }; /* t831: 36 = 음성·PCM(vo) */ /*E1R: 23-25 reserved, 26 mclick global */ /* E3R: 27 fontdir */ /* t733: 28 cpucore */
    if (tmini) {   /* t862f: 오버레이 TK 길게 누름 → 터치 설정 작은 창(게임 일시정지 위) */
        const int H_ = 20 * (int)ri.size() + 74;
        c.rect(120, 16, 400, H_ + 24, K); rbox(c, 128, 24, 384, H_ + 8, WH, T("터치 설정"));
        for (int q_ = 0; q_ < (int)ri.size(); q_++) { const int id_ = ri[q_]; const int y_ = 44 + 20 * q_; const bool sl_ = q_ == sel;
            const char * nm_ = id_ == 52 ? T("터치 키 표시") : id_ == 53 ? T("터치 모드") : id_ == 54 ? T("방향 입력") : id_ == 55 ? T("터치 배치 편집") : id_ == 56 ? T("스틱 조종") : id_ == 57 ? T("마우스 방식") : id_ == 58 ? T("마우스 가속") : id_ == 59 ? T("마우스 감도") : T("두 손가락 우클릭");
            { std::vector<std::string> one_(1, nm_); items(c, 144, y_, 140, one_, sl_ ? 0 : -1, 20); }
            if (id_ == 53) { const char * a_[] = { T("키보드"), T("키보드+마우스") }; choices(c, 296, y_, a_, 2, s.touchmode ? 1 : 0, WH, sl_); }
            else if (id_ == 54) { const char * a_[] = { T("십자"), T("아날로그") }; choices(c, 296, y_, a_, 2, (s.touchmode ? s.touchdir2 : s.touchdir) ? 1 : 0, WH, sl_); }
            else if (id_ == 55) { const char * a_[] = { T("편집") }; choices(c, 296, y_, a_, 1, 0, WH, sl_); }
            else if (id_ == 52) { const char * a_[] = { phr_("끔"), phr_("켬") }; choices(c, 296, y_, a_, 2, s.touchbar ? 1 : 0, WH, sl_); }
            else if (id_ == 56) { const char * a_[] = { T("방향키"), T("마우스") }; choices(c, 296, y_, a_, 2, s.tstick, WH, sl_); }
            else if (id_ == 57) { const char * a_[] = { T("트랙패드"), T("길게 누름") }; choices(c, 296, y_, a_, 2, s.tmmode, WH, sl_); }
            else if (id_ == 58) { const char * a_[] = { phr_("끔"), T("약"), T("강") }; choices(c, 296, y_, a_, 3, s.taccel, WH, sl_); }
            else if (id_ == 59) { const char * a_[] = { T("낮음"), T("보통"), T("높음") }; choices(c, 296, y_, a_, 3, s.tsens, WH, sl_); }
            else { const char * a_[] = { phr_("끔"), phr_("켬") }; choices(c, 296, y_, a_, 2, s.trclick, WH, sl_); } }
        { const std::string h0_ = phr_("↑↓:선택, ←→:값, ○:결정, ×:뒤로, △:출하 시 설정"), hc_ = help_common(h0_);
          const std::string hs_ = g_helpdev == 2 ? help_touch(h0_, false) : g_helpdev == 1 ? help_kbd(hc_) : kh_lbl(hc_);
          { size_t m_ = hs_.find(' ', hs_.size() / 2); const int y0_ = 44 + 20 * (int)ri.size() + 6;   /* t862g: 도움말 두 줄로(잘림 방지) */
            if (m_ == std::string::npos) text(c, 138, y0_, fit(hs_, 364), YL, K);
            else { text(c, 138, y0_, fit(hs_.substr(0, m_), 364), YL, K); text(c, 138, y0_ + 18, fit(hs_.substr(m_ + 1), 364), YL, K); } } }
        return;
    }
    std::vector<std::string> n;
    { const std::string nm[56] = { /*E1R*/ /* E3R */ T("적용 범위"), T("BGM 음량"), T("효과음 음량"),   /* E2R */
      KH_SNDLABEL,   /* E3R */
        (conf_grp ? phr_("pad_decide") : phr_("조작 설정")), phr_("배속 배율"), phr_("배속 방식"), phr_("배속 중 소리"),
      phr_("부팅 시퀀스"), phr_("배속 표시"), phr_("오버레이 버튼 표시"),
        phr_("게임 글꼴 (한글)"),   /* E3R */
      phr_("게임 글꼴 (일본어)"), phr_("언어 / Language / 言語"), phr_("indiv_reset"), "",
        phr_("snd_disk"),   /* E3R */ phr_("snd_dvol"), phr_("CPU 속도"), phr_("기동 시퀀스"),
      phr_("HDD 회전음"), phr_("회전음 음량"), phr_("오버레이 위치 모드"), phr_("오버레이 변"),
      phr_("오버레이 정렬"), phr_("오버레이 직접 위치"), phr_("메뉴 클릭음"),
        phr_("글꼴 폴더"), phr_("cpu_core"),
        phr_("touchfx"), phr_("tfxsz"), phr_("tfxmouse"), phr_("tmouse"), phr_("backpos"), phr_("set_vibe"), T("진동 세기"), phr_("snd_voice"), T("줄 음량 설정"),
      std::string(phr_("컨트롤러")) + "  >", std::string(phr_("터치 조작")) + "  >", std::string(phr_("오버레이")) + "  >", std::string(phr_("맵핑")) + "  >", std::string(phr_("표시")) + "  >", std::string(phr_("성능 설정")) + "  >", std::string(phr_("글꼴 설정")) + "  >", std::string(phr_("배속 설정")) + "  >", std::string(phr_("진단")) + "  >", phr_("부팅 시퀀스 다시 보기"), T("오버레이 크기"), T("오버레이 평소 진하기"), T("패드 연결 시 숨김"), phr_("메뉴 글꼴"), T("터치 키 표시"), T("터치 모드"), T("방향 입력"), T("터치 배치 편집") };   /* E3R / t733 */ /*E1R: 26 mclick */ /* E2R: 22-25 overlay */
      for (int id : ri) n.push_back(id == 15 ? std::string("FPS / Speed") : ((own && (ovm[cur] & rb[id])) ? std::string("● ") + nm[id] : nm[id])); }
    const int rs = (sel >= 0 && sel < (int)ri.size()) ? ri[sel] : 0;
    const int NR = (int)ri.size(), CAP = 14; const int top = lst_win(NR, sel, CAP);   /* kx-nova: 상자에 안 들어가면 스크롤 */
    { std::vector<std::string> n2(n.begin() + top, n.begin() + std::min(NR, top + CAP)); items(c, 40, 44, 256, n2, sel - top, 19); }
    { const int e2_ = std::min(NR, top + CAP); for (int q_ = top; q_ < e2_; q_++) { const int id_ = ri[q_]; if (id_ == 0 || id_ == 37 || id_ == 51 || (id_ >= 38 && id_ <= 47)) continue;   /* 정리안 A안: 줄마다 값 출처 꼬리표 (전역 = 회색 · 이 작품이 따로 가진 값 = 노랑) */
        const bool ov_ = own && (ovm[cur] & rb[id_]) != 0u; const bool sl_ = q_ == sel;
        int ex_ = 0; if (scope == 0 && rb[id_]) for (int gi_ = 0; gi_ < (int)games.size() && gi_ < MAXG; gi_++) if (ovm[gi_] & rb[id_]) ex_++;   /* t822: 예외 N작품 */
        const bool fa_ = !ov_ && ex_ == 0 && kh_factory_row(false, id_, s);   /* t822: 출하값 */
        char tg_[24]; snprintf(tg_, sizeof tg_, "%s%d", T("예외"), ex_ > 9 ? 9 : ex_);
        { const char * tag_ = ov_ ? T("이 작품") : ex_ > 0 ? tg_ : fa_ ? T("출하값") : T("전역"); text(c, 296 - tw(tag_), 44 + 19 * (q_ - top), tag_, ov_ ? YL : ex_ > 0 ? OR : fa_ ? CY : GY, sl_ ? WH : K); } } }   /* #1478 B3: 꼬리표 오른쪽 정렬(여백 8) */
    row_name_now.clear(); if (sel >= 0 && sel < (int)n.size()) { row_name_now = n[sel]; if (row_name_now.compare(0, 4, "● ") == 0) row_name_now.erase(0, 4); }
    lst_arrows(c, top, NR, CAP, 296, 29, 304);
    rbox(c, 320, 24, 304, 296, WH, phr_("인포메이션"));
    auto row = [&](int i) { return rs == i; };
    auto ry = [&](int id) { int k = -1000; for (size_t i = 0; i < ri.size(); i++) if (ri[i] == id) k = (int)i; if (k < top || k >= top + CAP) return -20000; return 44 + 19 * (k - top); };
    const char * sc[] = { T("전역"), phr_("scope_this") }; if (scope_lock) text(c, 344, ry(0), scope == 1 ? T("이 작품 고정") : T("전역 기본값 고정"), scope == 1 ? YL : WH, K); else choices(c, 344, ry(0), sc, 2, scope, WH, row(0));
    char b[8];
    snprintf(b, sizeof b, "%d", s.bgm); barv(c, 340, ry(1), 216, s.bgm / 8.0, "OFF", phr_("최대"), WH, row(1)); text(c, 574, ry(1), b, WH, K);
    snprintf(b, sizeof b, "%d", s.se); barv(c, 340, ry(2), 216, s.se / 8.0, "OFF", phr_("최대"), WH, row(2)); text(c, 574, ry(2), b, WH, K);
    snprintf(b, sizeof b, "%d", s.vo); barv(c, 340, ry(36), 216, s.vo / 8.0, "OFF", T("최대"), WH, row(36)); text(c, 574, ry(36), b, WH, K);   /* t831: 음성·PCM */
    if (in_game && !ln_name.empty()) text(c, 344, ry(37), T("줄 음량 설정  ▶"), row(37) ? K : WH, row(37) ? WH : K);   /* t844 */
    if (scope == 0) {
#if defined(__ANDROID__)
    { const char * sb[] = { "512", "1024", "2048", "4096" }; choices(c, 344, ry(3), sb, 4, g_sndbuf, WH, row(3)); }
#else
    text(c, 344, ry(3), phr_("기본 장치"), row(3) ? K : WH, row(3) ? WH : K);
#endif
    }
    if (conf_grp == 0) text(c, 344, ry(4), s.pad == 1 ? phr_("조작 설정 (서양식)") : phr_("조작 설정 (일본식)"), row(4) ? K : WH, row(4) ? WH : K);
    else { static std::string pdj, pdw; { auto cut_ = [](const char * q) { std::string t = q; size_t k = t.find(" ("); if (k != std::string::npos) t.resize(k); return t; }; pdj = cut_(phr_("일본식")); pdw = cut_(phr_("서양식")); } const char * pdv[] = { pdj.c_str(), pdw.c_str() }; choices(c, 344, ry(4), pdv, 2, s.pad == 1 ? 1 : 0, WH, row(4)); }   /* #1478: 결정 버튼 = 컨트롤러 페이지 줄 (맵핑은 41) */   /* t48e (#646): 고정 글자 → 조작 설정 화면 입구 */
    const char * bm[] = { "x2", "x3", "x4", "x6", "x8" };
    const char * cs[] = { phr_("표준"), phr_("빠름"), phr_("매우 빠름"), phr_("최대") }; choices(c, 344, ry(5), bm, 5, s.bmult, WH, row(5));
    const char * bk[] = { phr_("홀드"), phr_("토글"), phr_("둘 다") }; choices(c, 344, ry(6), bk, 3, s.bmode, WH, row(6));
    const char * bs[] = { phr_("끔"), phr_("빠르게"), phr_("음높이 유지") }; choices(c, 344, ry(7), bs, 3, s.bsound, WH, row(7));
    { const char * ds[] = { phr_("dsnd_off"), phr_("dsnd_seek"), phr_("dsnd_on") }; choices(c, 344, ry(16), ds, 3, s.dsnd, WH, row(16)); }   /* #641 */
    choices(c, 344, ry(18), cs, 4, s.cpuspd, WH, row(18));
    { char b2[8]; snprintf(b2, sizeof b2, "%d", s.dvol); barv(c, 340, ry(17), 216, s.dvol / 8.0, "OFF", "MAX", WH, row(17)); text(c, 574, ry(17), b2, WH, K); }
    { const char * pw[] = { phr_("끔"), phr_("pwr_short"), phr_("pwr_long") }; choices(c, 344, ry(19), pw, 3, s.pwrseq, WH, row(19)); }
    { const char * hh[] = { phr_("끔"), phr_("게임 중만"), phr_("메뉴 포함") }; choices(c, 344, ry(20), hh, 3, s.hddhum, WH, row(20)); }
    { char b3[8]; snprintf(b3, sizeof b3, "%d", s.hvol); barv(c, 340, ry(21), 216, s.hvol / 8.0, "OFF", "MAX", WH, row(21)); text(c, 574, ry(21), b3, WH, K); }
    { const char * mc[] = { phr_("끔"), phr_("켬") }; /*E1R*/
      choices(c, 344, ry(26), mc, 2, s.mclick, WH, row(26)); } /*E1R*/
    { const char * vb[] = { phr_("vibe_off"), phr_("vibe_on") }; choices(c, 344, ry(34), vb, 2, s.vibe, WH, row(34)); }   /* t794 */
    { char pb_[16]; snprintf(pb_, sizeof pb_, "%d%%", s.vibe_pct); barv(c, 340, ry(35), 216, (s.vibe_pct - 20) / 280.0, "20%", "300%", WH, row(35)); text(c, 574, ry(35), pb_, WH, K); }   /* t822: 진동 세기 */
    { const char * tf[] = { phr_("끔"), phr_("tfx_dot"), phr_("tfx_butterfly"), phr_("tfx_user") }; choices(c, 344, ry(29), tf, 4, s.touchfx, WH, row(29)); }   /* kx-touch */
    { const char * tz[] = { phr_("tfx_small"), phr_("tfx_large") }; choices(c, 344, ry(30), tz, 2, s.tfxsz, WH, row(30)); }
    { const char * tm[] = { phr_("끔"), phr_("켬") }; choices(c, 344, ry(31), tm, 2, s.tfxmouse, WH, row(31)); }
    { const char * tq[] = { phr_("tmouse_pad"), phr_("tmouse_abs") }; choices(c, 344, ry(32), tq, 2, s.tmouse, WH, row(32)); }
    { const char * bp[] = { phr_("bp_tl"), phr_("bp_tr"), phr_("bp_bl"), phr_("bp_br"), phr_("끔") }; choices(c, 344, ry(33), bp, 5, s.backpos, WH, row(33)); }
    const char * bt[] = { phr_("켬"), phr_("끔") }; choices(c, 344, ry(8), bt, 2, s.boot, WH, row(8));
    { const char * bo[] = { phr_("끔"), phr_("켬") }; choices(c, 344, ry(9), bo, 2, s.bosd, WH, row(9)); }
    { const char * ob[] = { phr_("끔"), phr_("켬") }; choices(c, 344, ry(10), ob, 2, s.ovbtn, WH, row(10)); }   /* t52f (#132) */
    { const char * om[] = { phr_("정렬 배치"), phr_("직접 지정") };   /* E2R */
      choices(c, 344, ry(22), om, 2, s.ovpos_mode, WH, row(22)); }
    { const char * oe[] = { phr_("하"), phr_("상"), phr_("좌"), phr_("우") };   /* E2R */
      choices(c, 344, ry(23), oe, 4, s.ovpos_edge, WH, row(23)); }
    if (s.ovpos_mode == 0) {   /* E2R */
      if (s.ovpos_edge <= 1) { const char * oa[] = { phr_("왼쪽"), phr_("가운데"), phr_("오른쪽") };
        choices(c, 344, ry(24), oa, 3, s.ovpos_align, WH, row(24)); }
      else { const char * oa[] = { phr_("위쪽"), phr_("가운데"), phr_("아래쪽") };
        choices(c, 344, ry(24), oa, 3, s.ovpos_align, WH, row(24)); } }
    else { char b4[8]; snprintf(b4, sizeof b4, "%d", s.ovpos_free);   /* E2R */
      barv(c, 340, ry(25), 216, s.ovpos_free / 100.0, "0", "100", WH, row(25));
      text(c, 574, ry(25), b4, WH, K); }
    if (scope == 0) { const char * os_[] = { T("작게"), T("보통"), T("크게"), T("아주 크게") }; choices(c, 344, ry(48), os_, 4, s.ovsize, WH, row(48));   /* t848 */
      const char * oa_[] = { T("연하게"), T("보통"), T("진하게"), T("항상 진하게") }; choices(c, 344, ry(49), oa_, 4, s.ovalpha, WH, row(49));
      const char * oh_[] = { "OFF", "ON" }; choices(c, 344, ry(50), oh_, 2, s.ovauto ? 1 : 0, WH, row(50)); }
    if (scope == 0) { const char * tb_[] = { phr_("끔"), phr_("켬") }; choices(c, 344, ry(52), tb_, 2, s.touchbar ? 1 : 0, WH, row(52)); }   /* t860 */
    if (scope == 0) { const char * tm_[] = { T("키보드"), T("키보드+마우스") }; choices(c, 344, ry(53), tm_, 2, s.touchmode ? 1 : 0, WH, row(53));   /* t862a */
      const char * td_[] = { T("십자"), T("아날로그") }; choices(c, 344, ry(54), td_, 2, (s.touchmode ? s.touchdir2 : s.touchdir) ? 1 : 0, WH, row(54)); }
    if (scope == 0 && in_game) { const char * te_[] = { T("편집") }; choices(c, 344, ry(55), te_, 1, 0, WH, row(55)); }   /* t862c */
    if (scope == 0) { const char * fp[] = { "OFF", "ON" }; choices(c, 344, ry(15), fp, 2, g.fpsd ? 1 : 0, WH, row(15)); }   /* #639 */
    if (scope == 0) {
    text(c, 344, ry(51), fit(mfont_label(font_ui), 250), row(51) ? K : WH, row(51) ? WH : K);
    text(c, 344, ry(11), fit(font_kr.empty() ? (std::string(phr_("기본")) + " (" + phr_("M갈무리 15") + ")") : fnt_nice_(font_kr), 250), row(11) ? K : WH, row(11) ? WH : K);
    text(c, 344, ry(12), fit(font_jp.empty() ? (std::string(phr_("기본")) + " (" + phr_("kyuhachi_font") + ")") : fnt_nice_(font_jp), 250), row(12) ? K : WH, row(12) ? WH : K);
    { const char * lg[] = { "한국어", "English", "にほんご" }; choices(c, 344, ry(13), lg, 3, lang, WH, row(13)); }   /* t52g (#133): 언어 */
#if defined(__ANDROID__)
    { std::string fdv = ov::g_kha_fontdir.empty() ? std::string(phr_("글꼴 폴더: 자동")) : ov::g_kha_fontdir;   /* E3R */
      text(c, 344, ry(27), fit(fdv, 250), row(27) ? K : WH, row(27) ? WH : K); }
    { const char * cq[] = { phr_("cpucore_fast"), phr_("cpucore_compat") }; choices(c, 344, ry(28), cq, 2, g_cpucore ? 0 : 1, WH, row(28)); }   /* t733: CPU 코어 */
#endif   /* E3R */
    }
#if defined(__ANDROID__)
    static const char * dd[] = { "desc_scope", "desc_bgm", "desc_se", "desc_sndbuf", "desc_pad", "desc_bmult", "desc_bmode", "desc_bsound", "desc_boot", "desc_bosd", "desc_ovbtn", "desc_fontkr", "desc_fontjp", "desc_lang", "desc_indiv_reset", "desc_fps", "desc_dsnd", "desc_dvol" };
#else
    static const char * dd[] = { "desc_scope", "desc_bgm", "desc_se", "desc_snddev", "desc_pad", "desc_bmult", "desc_bmode", "desc_bsound", "desc_boot", "desc_bosd", "desc_ovbtn", "desc_fontkr", "desc_fontjp", "desc_lang", "desc_indiv_reset", "desc_fps", "desc_dsnd", "desc_dvol" };
#endif
    static const char * const fd[10] = { T("컨트롤러: 결정 버튼(○×)과 패드 버튼 맵핑을 정합니다."), T("터치 조작: 화면 터치 표시와 터치 마우스 방식을 정합니다."), T("오버레이: 화면 위 버튼의 표시·위치·뒤로가기 버튼 위치를 정합니다."), T("맵핑: 패드 버튼마다 키·기능을 배정합니다."), T("표시: 터치할 때 나오는 표시의 종류·크기·마우스 모드 표시를 정합니다."), T("성능 설정: CPU 속도와 (안드로이드) CPU 코어를 정합니다."), T("글꼴 설정: 한글·일본어 글꼴과 글꼴 폴더를 정합니다."), T("배속 설정: 배속 배율·방식·배속 중 소리를 정합니다."), T("진단: 진동 시험·컨트롤러 정보·진단 로그를 만듭니다."), T("이 작품의 부팅 연출을 다시 봅니다.") };
    std::string dt = dkey(phr_((rs >= 38 && rs <= 47) ? fd[rs - 38] : rs == 48 ? T("오버레이 크기: 화면 위 버튼 줄의 크기. 작게 80% · 보통 100% · 크게 125% · 아주 크게 150%.") : rs == 49 ? T("오버레이 평소 진하기: 만지지 않을 때 버튼 줄의 진하기. 연하게·보통·진하게·항상 진하게.") : rs == 50 ? T("패드 연결 시 숨김: 켜면 게임패드가 연결돼 있는 동안 버튼 줄을 숨깁니다.") : rs == 37 ? T("줄 음량: 이 작품의 소리를 줄(음성·효과음·음악 등)별로 따로 조절합니다. ○ 로 들어갑니다.") : rs == 36 ? "desc_vo" : rs == 33 ? "desc_backpos" : rs == 32 ? "desc_tmouse" : rs == 31 ? "desc_tfxmouse" : rs == 30 ? "desc_tfxsz" : rs == 29 ? "desc_touchfx" : rs == 28 ? "desc_cpucore" : rs == 27   /* E3R */
        ? "desc_fontdir" : rs == 22 ? "desc_ovpos_mode" : rs == 23 ?   /* E2R */
      "desc_ovpos_edge" : rs == 24 ? "desc_ovpos_align" : rs == 25 ? "desc_ovpos_free" :
      rs == 19 ? "desc_pwrseq" : rs == 20 ? "desc_hddhum" : rs == 21 ? "desc_hvol" :
      rs == 35 ? T("진동 세기: 기기 진동의 세기 (20~300%, 기본 100). ←→ 로 10씩. 설정 > 실행환경 > 진단 > 진동 시험으로 확인") : rs == 34 ? "desc_vibe" : rs == 26 ? "desc_mclick" /*E1R*/ : rs == 18 ? "desc_cpuspd" :
        rs == 15 ? "desc_fps" : rs == 55 ? T("터치 배치 편집: 키 묶음을 끌어 옮기고, 키를 더하거나 내보내고 가져옵니다. 게임 중 메뉴에서만 열립니다.") : rs == 53 ? T("터치 모드: 키보드 = 방향·확인 키만 화면에 보입니다. 키보드+마우스 = 마우스 조작 영역과 L R 버튼이 더해집니다.") : rs == 54 ? T("방향 입력: 십자 = 방향 키 4개, 아날로그 = 화면 왼쪽을 누르면 그 자리에 스틱이 생깁니다.") : rs == 52 ? T("터치 키 표시: 게임 화면 아래 터치 키 줄을 보이거나 숨깁니다. 패드 연결 여부와 상관없습니다.") : rs == 51 ? T("메뉴 글꼴: 메뉴·설정 화면의 글자 모양. ○ 로 들어가 고릅니다.") : (rs >= 0 && rs < 18) ? dd[rs] : ""));   /* t854: 행 51 설명 추가 + dd[] 범위 막기 (노바 크래시 #1674: dd[51] 배열 밖 읽기) */   /* E3R */
    if (rs == 18) { static const char * const ce[4] = { "cpu_eq0", "cpu_eq1", "cpu_eq2", "cpu_eq3" }; const int cq = s.cpuspd < 0 ? 0 : s.cpuspd > 3 ? 3 : s.cpuspd; dt += std::string("  ·  ") + phr_(ce[cq]); }   /* #S2: 지금 단계의 실재 기종 기준 환산(DOSBox-X 위키 cycles 표 + PC-98 기종 클럭). 한 줄이 길면 desc_mq 가 흐른다 */
    if (ind) dt = std::string(phr_("indiv_has")) + "  ·  " + dt;
    { const char * hp_ = phr_("↑↓:선택, ←→:값, ○:결정, ×:뒤로, △:출하 시 설정"); desc(c, 328, 44, clamp_lines(wrap_words(dt, 560), 2, 560), -1, hp_); }   /* t848: 흐르는 한 줄 대신 두 줄로 접어 고정 표시 */
    if (save_note_pend) { save_note = std::string(T("저장함: ")) + row_name_now + (g_row_val.empty() ? std::string() : " = " + g_row_val) + (scope == 1 ? T(" (이 작품)") : T(" (전역)")); save_note_pend = 0; }
    if (!save_note.empty() && ts_ms < save_note_until) { c.rect(336, 300, 280, 18, K); text(c, 344, 302, fit(save_note, 270), YL, K); }   /* t822: 잔상 지움 */
    else if (scope == 0 && rs > 0 && rs < 36 && rb[rs]) { int n_ = 0; for (int gi_ = 0; gi_ < (int)games.size() && gi_ < MAXG; gi_++) if (ovm[gi_] & rb[rs]) n_++;   /* t822: 예외 N작품 설명줄 */
        if (n_ > 0) { char eb_[96]; snprintf(eb_, sizeof eb_, "%s %d%s", T("예외"), n_, T("작품: 이 줄 값을 따로 씀")); text(c, 344, 302, fit(eb_, 270), OR, K); } }   /* 정리안: 저장함 한 줄 */
    if (scope == 0 && (rs == 11 || rs == 12)) { const std::string & fl = font_now(); if (!fl.empty()) text(c, 344, 288, fit(fl, 270), CY, K); }   /* build 5b-3: font now applied to the current work */
    const bool ibk = own && rs < 38 && (ovm[cur] & indiv_bit(S_CONF, rs));
    helpline(c, ibk ? phr_("indiv_help") : phr_("↑↓:선택, ←→:값, ○:결정, ×:뒤로, △:출하 시 설정"));
}
/* build 5b-3: "now applied: <font> (source)" for the current work. priority: launch method (font=) > work language default (font_kind) > system. */
inline const std::string & Ui::font_now() {
    if (fnow_cur == cur && fnow_kr == font_kr && fnow_jp == font_jp) return fnow_line;
    fnow_cur = cur; fnow_kr = font_kr; fnow_jp = font_jp; fnow_line.clear();
    if (cur < 0 || cur >= (int)games.size() || !env.values) return fnow_line;
    std::vector<ValRow> vr; if (!env.values(cur, vr)) return fnow_line;
    std::string fv, kd;
    for (auto & r : vr) { if (r.val.empty() || r.val[0] == '(') continue; if (r.key == "font") fv = r.val; else if (r.key == "font_kind") kd = r.val; }
    std::string nm, src;
    if (!fv.empty()) { nm = fv; src = phr_("fsrc_ent"); }
    else if (kd == "jp") { nm = font_jp.empty() ? std::string("kyuhachi_font") : font_jp; src = phr_("fsrc_lang"); }
    else if (kd == "kr") { nm = font_kr.empty() ? std::string("kyuhachi_font") : font_kr; src = phr_("fsrc_lang"); }
    else { nm = font_kr.empty() ? std::string("kyuhachi_font") : font_kr; src = phr_("fsrc_sys"); }
    fnow_line = std::string(phr_("font_now")) + " " + nm + " (" + src + ")";
    return fnow_line;
}
