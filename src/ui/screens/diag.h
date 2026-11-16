#pragma once
/* screens/diag.h -- S_DIAG: 설정 > 진단 (메뉴 정리안 §7, 슨상 결정 2026-11-16 14:35).
   진동 시험 · 컨트롤러 정보 · 진단 로그 저장 · 클립보드 복사. 실제 일은 호스트(app/diag_host.h)가 Env::diag / diag_save 로 한다. */
inline void Ui::d_diag(Canvas & c) {
    titlebar(c, "");
    rbox(c, 16, 24, 288, 224, WH, phr_("진단"));
    std::vector<std::string> a = { phr_("진동 시험"), phr_("컨트롤러 정보"), phr_("진단 로그 저장"), phr_("클립보드 복사") };
    items(c, 40, 46, 256, a, sel, 28);
    rbox(c, 320, 24, 304, 224, WH, phr_("결과"));
    { size_t p = 0; int ln = 0; const std::string & o = diag_out;
      while (p < o.size() && ln < 9) { size_t e = o.find('\n', p); if (e == std::string::npos) e = o.size(); text(c, 332, 44 + ln * 20, fit(o.substr(p, e - p), 280), WH, K); ln++; p = e + 1; } }
    if (diag_wait) text(c, 332, 200, phr_("느꼈나요?  ←: 느꼈음   →: 못 느꼈음"), YL, K);
    else if (!diag_ans.empty()) text(c, 332, 200, std::string(phr_("응답: ")) + diag_ans, CY, K);
    desc(c, 264, 64, { phr_("진동이 안 될 때: 진동 시험 → 진단 로그 저장 →"), phr_("그 파일(또는 복사한 글)을 올려 주세요."), phr_("이름·일련번호는 가려집니다.") });   /* t822: 3줄로 나눠 화면 안에 */
    helpline(c, phr_("↑↓:선택, ○:실행, ×:뒤로"));
}
inline void Ui::k_diag(UKey k, Act & a) {
    (void)a;
    if (k == UK_UP) sel = (sel + 3) % 4;
    else if (k == UK_DOWN) sel = (sel + 1) % 4;
    else if (diag_wait && (k == UK_LEFT || k == UK_RIGHT)) { diag_ans = (k == UK_LEFT) ? "느꼈음" : "못 느꼈음"; diag_wait = 0; }
    else if (k == UK_OK) {
        if (!env.diag) { diag_out = phr_("이 실행에서는 진단을 쓸 수 없습니다."); return; }
        if (sel == 0) { diag_ans.clear(); diag_out = env.diag(0, std::string()); diag_wait = 1; }
        else if (sel == 1) diag_out = env.diag(2, std::string());
        else if (sel == 2) { const std::string p = env.diag_save ? env.diag_save(env.diag(3, diag_ans)) : std::string(); diag_out = p.empty() ? std::string(phr_("저장하지 못했습니다.")) : std::string(phr_("저장함:")) + "\n" + p; }
        else if (sel == 3) { if (env.clip_set) { env.clip_set(env.diag(3, diag_ans)); diag_out = phr_("진단 로그를 클립보드에 복사했습니다."); } }
    } else if (k == UK_CANCEL) { diag_wait = 0; go(S_SET, 5); }
}
