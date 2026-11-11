#pragma once
/* screens/stem.h -- #L6: S_STEM 화면 「원판 후보 고르기」. 이름 줄기 점수로 원판을 자동으로 정하지 못한 작품(1위/2위 비 < 2)마다
 * 상위 3 후보(게임 이름 · 일치 줄기 수)를 보여 주고, 고른 것은 game.ini boot_gid 로 저장한다(A_STEMPICK). 판 구분(한글판·원판)은 건드리지 않는다.
 * Members of struct Ui, defined out of line. 후보 문자열 형식 = game.ini stem_cand= 「gid~이름~일치수 | gid~이름~일치수 | …」(settings/kh_stem.h cand_pack). */
struct StemC { std::string gid, name; int n = 0; };
inline std::vector<StemC> stem_cands_of(const std::string & s) {
    std::vector<StemC> r; size_t p = 0;
    while (!s.empty() && p <= s.size()) {
        size_t q = s.find(" | ", p); std::string it = s.substr(p, q == std::string::npos ? std::string::npos : q - p);
        size_t a = it.find('~'), b = a == std::string::npos ? a : it.find('~', a + 1);
        if (b != std::string::npos) { StemC k; k.gid = it.substr(0, a); k.name = it.substr(a + 1, b - a - 1); k.n = atoi(it.c_str() + b + 1); r.push_back(k); }
        if (q == std::string::npos) break; p = q + 3;
    }
    return r;
}
inline void Ui::d_stem(Canvas & c) {
    titlebar(c, phr_("원판 후보 고르기"));
    std::vector<int> tg = stem_targets();
    rbox(c, 16, 24, 288, 224, WH, phr_("자동으로 못 정한 작품"));
    std::vector<std::string> r; std::vector<uint32_t> col;
    for (int gi : tg) { r.push_back(fit(short_of(games[gi]), 250)); col.push_back(0); }
    if (r.empty()) { r.push_back(phr_("(대상 없음)")); col.push_back(GY); }
    int ws = 0; if (stem_lvl == 1) { for (size_t i = 0; i < tg.size(); i++) if (tg[i] == stem_w) ws = (int)i; } else ws = sel;
    rowlist(c, 40, 48, 256, 7, r, ws, 24, &col);
    rbox(c, 320, 24, 304, 224, WH, stem_lvl == 1 ? phr_("원판 후보") : phr_("상세"));
    int wi = (stem_lvl == 1) ? stem_w : (sel >= 0 && sel < (int)tg.size() ? tg[sel] : -1);
    if (wi >= 0 && wi < (int)games.size()) {
        std::vector<StemC> cs = stem_cands_of(games[wi].stemc);
        text(c, 340, 44, fit(games[wi].title, 270), WH, K);
        if (stem_lvl == 1) {
            std::vector<std::string> cr; std::vector<uint32_t> cc;
            for (auto & k : cs) { cr.push_back(fit(k.name, 200) + "  " + std::to_string(k.n)); cc.push_back(k.gid.empty() ? GY : 0); }
            if (cr.empty()) { cr.push_back(phr_("(후보 없음)")); cc.push_back(GY); }
            rowlist(c, 336, 72, 276, 3, cr, sel, 24, &cc);
            text(c, 340, 156, phr_("이름 옆 숫자는 일치한 실행 파일 이름 수"), CY, K);
            if (sel >= 0 && sel < (int)cs.size() && cs[sel].gid.empty()) text(c, 340, 176, phr_("이 후보는 번호가 없어 고를 수 없습니다"), OR, K);
        } else {
            text(c, 340, 68, phr_("후보 (일치 줄기 수)"), CY, K);
            for (size_t i = 0; i < cs.size() && i < 3; i++) text(c, 340, 92 + (int)i * 22, fit(cs[i].name, 230) + "  " + std::to_string(cs[i].n), i == 0 ? WH : GY, K);
            if (cs.empty()) text(c, 340, 92, phr_("(후보 없음)"), GY, K);
            text(c, 340, 170, phr_("판 구분(한글판·원판)은 바꾸지 않습니다."), WH, K);
        }
    }
    desc(c, 264, 64, { phr_("실행 파일 이름이 가장 비슷한 원판 후보입니다. 고르면 그 원판으로 기동 상태를 표시합니다."), phr_("저장 위치: 작품 폴더 game.ini 의 boot_gid 항목 (고르기 전에는 「미확인」).") });
    helpline(c, stem_lvl == 1 ? phr_("↑↓:후보, ○:정함, ×:돌아가기") : phr_("↑↓:작품, ○:후보 보기, ×:돌아가기"));
}
inline void Ui::k_stem(UKey k, Act & a) {
    do { {
            std::vector<int> tg = stem_targets();
            if (stem_lvl == 0) {
                int n = std::max<int>((int)tg.size(), 1); if (sel >= n) sel = 0;
                if (k == UK_UP) sel = (sel + n - 1) % n; else if (k == UK_DOWN) sel = (sel + 1) % n;
                else if (k == UK_OK) { if (sel < (int)tg.size()) { stem_w = tg[sel]; stem_lvl = 1; sel = 0; } }
                else if (k == UK_CANCEL) { scr = S_ADD; sel = 6; }
            } else {
                int cn = (stem_w >= 0 && stem_w < (int)games.size()) ? (int)stem_cands_of(games[stem_w].stemc).size() : 0; int n = std::max(cn, 1);
                if (k == UK_UP) sel = (sel + n - 1) % n; else if (k == UK_DOWN) sel = (sel + 1) % n;
                else if (k == UK_OK) {
                    std::vector<StemC> cs = stem_cands_of(games[stem_w].stemc);
                    if (sel >= 0 && sel < (int)cs.size() && !cs[sel].gid.empty()) { a.t = A_STEMPICK; a.game = stem_w; a.arg = sel; dirty = true; stem_lvl = 0; sel = 0; }
                }
                else if (k == UK_CANCEL) { int ws = 0; for (size_t i = 0; i < tg.size(); i++) if (tg[i] == stem_w) ws = (int)i; stem_lvl = 0; sel = ws; }
            }
            break; }
    } while (0);
}
