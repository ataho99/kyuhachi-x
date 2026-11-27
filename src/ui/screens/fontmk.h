#pragma once
/* screens/fontmk.h -- t867: 설정 > 글꼴 > 「내 글꼴 가져오기」 (S_FONT 의 fnt_mode 4). 파일 고르기 → 용도(둘 다 / 게임 / 메뉴) → 크기·문턱 미리보기 → 구워 저장. 구운 결과는 이 기기(res\fonts)에만 저장한다. 지우기 = 글꼴 목록에서 △ 두 번 (mk_delete). */
inline void Ui::mk_open(int from_mode) {
    mk = fontbake::State(); mk.ret_mode = from_mode; mk.ret_prev = fnt_prev_sel; mk.st = 0; fnt_mode = 4;
    mk_base_g.clear(); mk_base_m.clear(); ov::BmpFont t;
    for (auto & f : fonts) if (f.ok && f.name == "kr_mg15") { if (t.load(f.path)) mk_base_g = t.d; break; }
    for (auto & f : mfonts) if (f.ok && (f.name == "mg15" || f.name == "galmuri14")) { if (t.load(f.path)) { mk_base_m = t.d; if (f.name == "mg15") break; } }
}
inline void Ui::mk_close() {
    mk_b.close(); std::vector<uint8_t>().swap(mk_base_g); std::vector<uint8_t>().swap(mk_base_m); mk_pv.ok = false; std::vector<uint8_t>().swap(mk_pv.d); mk_pv2.ok = false; std::vector<uint8_t>().swap(mk_pv2.d);
    const int rm = mk.ret_mode, rp = mk.ret_prev; mk = fontbake::State(); open_font(rm, fnt_ret); fnt_prev_sel = rp;
}
inline void Ui::mk_rebake() {
    auto one = [&](int pu, ov::BmpFont & pv, fontbake::Info & inf) -> bool {
        std::vector<uint8_t> out; const std::vector<uint8_t> & bs = pu == 0 ? mk_base_g : mk_base_m;
        if (!mk_b.bake(pu, mk.size, mk.thr, bs.size() == 524350 ? &bs : NULL, out, inf)) return false;
        pv.d.swap(out); pv.ok = true; return true;
    };
    mk_pv.ok = false; mk_pv2.ok = false; bool ok = true;
    if (mk.purpose == 0) ok = one(0, mk_pv, mk.info);
    else if (mk.purpose == 1) ok = one(1, mk_pv, mk.info);
    else ok = one(0, mk_pv, mk.info) && one(1, mk_pv2, mk_info2);
    if (!ok) { mk.msg = phr_("글꼴 파일을 읽을 수 없습니다"); mk_pv.ok = false; mk_pv2.ok = false; return; }
    mk.msg.clear();
}
inline void Ui::mk_save() {
    std::string gd, md; if (!env.fontmk_dirs || !env.fontmk_dirs(gd, md)) { mk.msg = phr_("저장 위치를 찾지 못했습니다"); return; }
    const bool both = mk.purpose == 2; std::vector<int> ws; if (both) { ws.push_back(0); ws.push_back(1); } else ws.push_back(mk.purpose);
    const int mfull = both ? mk_info2.full : mk.info.full; const std::string base = "u_" + mk.stem;
    auto fname = [&](int pu, int n) { std::string s = base + (n > 1 ? std::to_string(n) : std::string()); if (pu == 1) s += "_" + std::to_string(mfull); return s; };
    auto dirof = [&](int pu) { return pu == 0 ? gd : md; };
    int n = 1; for (; n < 100; n++) { bool any = false; for (size_t i = 0; i < ws.size(); i++) { FILE * t = fopen((dirof(ws[i]) + env.sep + fname(ws[i], n) + ".bmp").c_str(), "rb"); if (t) { fclose(t); any = true; } } if (!any) break; }
    for (size_t i = 0; i < ws.size(); i++) { const ov::BmpFont & s = (ws[i] == 1 && both) ? mk_pv2 : mk_pv; if (!s.ok || s.d.size() != 524350) { mk.msg = phr_("저장하지 못했습니다"); return; } }
    std::vector<std::string> done;
    for (size_t i = 0; i < ws.size(); i++) {
        const ov::BmpFont & s = (ws[i] == 1 && both) ? mk_pv2 : mk_pv; const std::string path = dirof(ws[i]) + env.sep + fname(ws[i], n) + ".bmp";
        FILE * f = fopen(path.c_str(), "wb"); bool ok = f && fwrite(s.d.data(), 1, s.d.size(), f) == s.d.size(); if (f) fclose(f);
        if (!ok) { if (f) ::remove(path.c_str()); for (size_t k = 0; k < done.size(); k++) ::remove(done[k].c_str()); mk.msg = phr_("저장하지 못했습니다"); return; }
        done.push_back(path);
    }
    const std::string dn = mk.disp + (n > 1 ? " " + std::to_string(n) : std::string());
    for (size_t i = 0; i < ws.size(); i++) {
        const std::string nm = fname(ws[i], n); FILE * t = fopen((dirof(ws[i]) + env.sep + nm + ".txt").c_str(), "wb"); if (t) { fwrite(dn.data(), 1, dn.size(), t); fclose(t); }
        fontbake::names()[nm] = dn; adv_cache.erase(nm); if (ws[i] == 1) mk.saved2 = nm; else mk.saved = nm;
    }
    if (mk.purpose == 1) { mk.saved = mk.saved2; mk.saved2.clear(); }
    mk.msg = std::string(phr_("저장했습니다")) + ": " + dn; if (env.fontmk_rescan) env.fontmk_rescan(); mk.st = 3;
}
inline void Ui::mk_delete(const std::string & o, Act & a) {
    if (o.compare(0, 2, "u_") != 0) return;   /* 내 글꼴(u_…)만 지운다 */
    std::string path; if (fnt_mode == 3) { for (auto & f : mfonts) if (f.name == o) path = f.path; } else { for (auto & f : fonts) if (f.name == o) path = f.path; }
    if (path.empty()) return;
    ::remove(path.c_str()); size_t d = path.find_last_of('.'); if (d != std::string::npos) ::remove((path.substr(0, d) + ".txt").c_str());
    fontbake::names().erase(o); adv_cache.erase(o);
    if (font_kr == o) font_kr.clear(); if (font_jp == o) font_jp.clear(); if (font_ui == o) font_ui = mf_have("mg12") ? std::string("mg12") : std::string("builtin");
    const int m_ = fnt_mode, so = sel, rp = fnt_prev_sel;
    if (env.fontmk_rescan) env.fontmk_rescan();
    open_font(m_, fnt_ret); fnt_prev_sel = rp; sel = std::min(so, (int)fnt_opts.size() - 1); if (sel < 0) sel = 0;
    menu_font_apply(); dirty = true; save_now = true; a.t = A_APPLY;
}
inline void Ui::d_fontmk(Canvas & c) {
    titlebar(c, ""); rbox(c, 16, 24, 608, 296, WH, phr_("내 글꼴 가져오기")); const int W_ = 568;
    if (mk.st == 0) {
        text(c, 40, 52, fit(phr_("내 기기의 글꼴 파일(TTF · OTF)을 골라 이 앱에서 직접 구워 씁니다."), W_), WH, K);
        text(c, 40, 76, fit(phr_("구운 결과는 이 기기에만 저장됩니다."), W_), WH, K);
        text(c, 40, 100, fit(phr_("글꼴 라이선스는 사용자 책임입니다. 구운 파일을 배포하지 마세요."), W_), OR, K);
        text(c, 40, 148, fit(phr_("○:파일 고르기, ×:뒤로"), W_), YL, K);
        if (!mk.msg.empty()) text(c, 40, 190, fit(mk.msg, W_), OR, K);
        helpline(c, phr_("○:파일 고르기, ×:뒤로")); return;
    }
    if (mk.st == 1) {
        static const int ORD[3] = { 2, 0, 1 }; text(c, 40, 48, fit(mk.disp, W_), YL, K); text(c, 40, 76, fit(phr_("어디에 쓸까요?"), W_), WH, K);
        std::vector<std::string> r; for (int i = 0; i < 3; i++) r.push_back(std::string(mk.purpose == ORD[i] ? "● " : "  ") + (ORD[i] == 2 ? phr_("게임 + 메뉴 둘 다") : ORD[i] == 0 ? phr_("게임 한글 (16칸)") : phr_("메뉴 글꼴")));
        const int pos = mk.purpose == 2 ? 0 : mk.purpose == 0 ? 1 : 2; rowlist(c, 40, 104, 560, 3, r, pos, 24);
        helpline(c, phr_("↑↓:선택, ○:다음, ×:뒤로")); return;
    }
    if (mk.st == 3) {
        text(c, 40, 56, fit(mk.msg, W_), YL, K); text(c, 40, 100, fit(phr_("○:바로 적용, ×:닫기"), W_), WH, K);
        helpline(c, phr_("○:바로 적용, ×:닫기")); return;
    }
    const fontbake::Info & I = mk.info;
    text(c, 40, 44, fit(mk.disp + "  -  " + (mk.purpose == 2 ? phr_("게임 + 메뉴 둘 다") : mk.purpose == 0 ? phr_("게임 한글 (16칸)") : phr_("메뉴 글꼴")), W_), YL, K);
    text(c, 40, 68, fit(std::string(phr_("크기")) + ": " + std::to_string(mk.size) + " < >      " + phr_("문턱") + ": " + std::to_string(mk.thr) + " \xE2\x86\x91\xE2\x86\x93", W_), WH, K);
    text(c, 40, 92, fit(std::string(phr_("한글 글자 수")) + ": " + std::to_string(I.hangul) + "     " + phr_("한글 상자") + ": " + std::to_string(I.bw) + "x" + std::to_string(I.bh), W_), CY, K);
    if (I.clip > 0) text(c, 40, 116, fit(std::string(phr_("칸 밖으로 잘린 글자")) + ": " + std::to_string(I.clip), W_), OR, K);
    const int y0 = 144;
    auto menu_prev = [&](const ov::BmpFont & bm, int full, int y, bool four) {
        const std::string nm = "u_pv_" + std::to_string(full); adv_cache.erase(nm);
        const ov::BmpFont * sv = ov::g_menu_bmp; const int * sa_ = ov::g_menu_advp; const int sh_ = ov::g_menu_half, sf_ = ov::g_menu_full;
        ov::g_menu_bmp = &bm; ov::g_menu_advp = mf_adv(nm, &bm); ov::g_menu_half = mf_half(nm); ov::g_menu_full = mf_full(nm);
        text(c, 40, y, phr_("설정 > 화면 효과 · 글꼴 · 소리 (Screen effects)"), WH, K); text(c, 40, y + 26, phr_("여기까지 오다니 훌륭하다. 가나다 ABC 123"), WH, K);
        if (four) { text(c, 40, y + 52, phr_("닭 읽 몫 넓 핥 않 곬 값 왜 웨 뙤 휑 쫓"), WH, K); text(c, 40, y + 78, "Settings Sound BGM 0123 .,?!", WH, K); }
        ov::g_menu_bmp = sv; ov::g_menu_advp = sa_; ov::g_menu_half = sh_; ov::g_menu_full = sf_;
    };
    if (mk_pv.ok) {
        if (mk.purpose == 0) {
            text_bmp(c, mk_pv, 40, y0, phr_("여기까지 오다니 훌륭하다. 가나다 ABC 123"), WH, K); text_bmp(c, mk_pv, 40, y0 + 26, phr_("이 앞은 위험해. 무리하지 마, 알겠지?"), WH, K);
            text_bmp(c, mk_pv, 40, y0 + 52, phr_("닭 읽 몫 넓 핥 않 곬 값 왜 웨 뙤 휑 쫓"), WH, K); text_bmp(c, mk_pv, 40, y0 + 78, "The quick brown fox, 0123456789 .,?!", WH, K);
        } else if (mk.purpose == 1) menu_prev(mk_pv, I.full, y0, true);
        else if (mk_pv2.ok) {
            text_bmp(c, mk_pv, 40, y0, phr_("여기까지 오다니 훌륭하다. 가나다 ABC 123"), WH, K); text_bmp(c, mk_pv, 40, y0 + 26, phr_("닭 읽 몫 넓 핥 않 곬 값 왜 웨 뙤 휑 쫓"), WH, K);
            menu_prev(mk_pv2, mk_info2.full, y0 + 62, false);
        }
        text(c, 40, 262, fit(phr_("기호·일본 문자는 기본 글꼴로 채웁니다."), W_), GY, K);
    } else text(c, 40, y0, fit(mk.msg, W_), OR, K);
    if (!mk.msg.empty() && mk_pv.ok) text(c, 40, 288, fit(mk.msg, W_), OR, K);
    helpline(c, phr_("↑↓:문턱, ←→:크기, ○:저장, ×:뒤로"));
}
inline void Ui::k_fontmk(UKey k, Act & a) {
    if (mk.st == 0) {
        if (k == UK_CANCEL) { mk_close(); return; }
        if (k == UK_OK) {
            std::string p; if (!env.fontmk_pick || !env.fontmk_pick(p)) return;
            const int rc = mk_b.open(p);
            if (rc == 1) { mk.msg = phr_("글꼴 파일을 읽을 수 없습니다"); return; }
            if (rc == 2 || mk_b.hangul_have() < 2000) { mk.msg = phr_("한글 글자가 모자라는 글꼴입니다"); mk_b.close(); return; }
            mk.path = p; mk.stem = fontbake::stem_of(p); mk.disp = env.acp_u8 ? env.acp_u8(fontbake::raw_stem(p)) : mk.stem; if (mk.disp.empty()) mk.disp = mk.stem; mk.msg.clear(); mk.purpose = 2; mk.st = 1;
        }
        return;
    }
    if (mk.st == 1) {
        static const int ORD[3] = { 2, 0, 1 };
        if (k == UK_UP || k == UK_DOWN) { int pos = mk.purpose == 2 ? 0 : mk.purpose == 0 ? 1 : 2; pos = (pos + (k == UK_DOWN ? 1 : 2)) % 3; mk.purpose = ORD[pos]; }
        else if (k == UK_CANCEL) { mk.st = 0; mk_b.close(); }
        else if (k == UK_OK) { mk.thr = 110; mk.size = mk_b.auto_size(mk.thr); mk_rebake(); mk.st = 2; }
        return;
    }
    if (mk.st == 2) {
        if (k == UK_LEFT || k == UK_RIGHT) { const int s = mk.size + (k == UK_RIGHT ? 1 : -1); if (s >= 10 && s <= 16) { mk.size = s; mk_rebake(); } }
        else if (k == UK_UP || k == UK_DOWN) { const int t = mk.thr + (k == UK_UP ? 10 : -10); if (t >= 40 && t <= 200) { mk.thr = t; mk_rebake(); } }
        else if (k == UK_OK) mk_save();
        else if (k == UK_CANCEL) mk.st = 1;
        return;
    }
    if (k == UK_OK) {
        if (mk.purpose != 1) font_kr = mk.saved;
        if (mk.purpose != 0) { const std::string & mn = mk.purpose == 2 ? mk.saved2 : mk.saved; if (mf_have(mn)) { font_ui = mn; menu_font_apply(); } }
        dirty = true; save_now = true; a.t = A_APPLY; mk_close();
    } else if (k == UK_CANCEL) mk_close();
}
