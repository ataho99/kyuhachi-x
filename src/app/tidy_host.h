#pragma once
/* app/tidy_host.h -- kx-clean: 「목록 정리」(screens/tidy.h) 의 host 쪽. ui_env.h 의 gh_stempick 뒤에서 포함한다(gh_reload_games · gh_pick_folder 사용).
 * 원본 이미지·큐하치 작품 폴더는 지우거나 옮기지 않는다. 「빼기」= user.ini [game] tidy_hidden=1 (목록에서만 숨김, 되살리기 = 키 삭제).
 * 세이브는 기본으로 남기고, 사용자가 고르면 saves\_old\<id> 로 옮긴다(삭제 없음). 윈도·안드 같은 코드. */
static std::vector<std::string> gh_tidy_csv(const std::string & s) {
    std::vector<std::string> r; size_t p = 0;
    while (!s.empty() && p <= s.size()) { size_t q = s.find(',', p); if (q == std::string::npos) q = s.size(); if (q > p) r.push_back(s.substr(p, q - p)); p = q + 1; }
    return r;
}
/* game.ini [game] 에서 이미지 이름(소문자, 해시 아님)과 sha1 목록을 모은다 */
static void gh_tidy_names(std::map<std::string, std::string> & g, std::set<std::string> & names, std::set<std::string> & shas) {
    static const char * const dk[] = { "disk_a", "disk_b", "disk_c", "disk_d", "cd", "swap", "files" };
    for (auto k : dk) { auto it = g.find(k); if (it == g.end()) continue;
        for (auto & v : gh_tidy_csv(it->second)) { std::string b = aw::lower(aw::base_name_of(v)); if (!b.empty() && b != "-" && !aw::is_hex40(b)) names.insert(b); } }
    { auto it = g.find("sha1s"); if (it != g.end()) for (auto & h : gh_tidy_csv(it->second)) shas.insert(aw::lower(h)); }
}
static bool gh_tidy_has_files(const std::string & dir) { std::vector<std::string> f, s; gh_list_dir(dir, f, &s); return !f.empty() || !s.empty(); }
static void gh_tidy_scan(void) {
    ui::Ui & U = gApp.ui_; U.tidy_rows.clear();
    const std::string gd = gApp.exedir + "\\games";
    std::vector<std::string> files, subs; gh_list_dir(gd, files, &subs); std::sort(subs.begin(), subs.end());
    struct Ex { std::set<std::string> names, shas; std::string ns; };
    std::vector<Ex> ex;
    for (auto & d : subs) {
        if (d.empty() || d[0] == '_') continue;
        const std::string dir = gd + "\\" + d; prof::Ini gi; if (!prof::ini_load(dir + "/game.ini", gi) || !gi.count("game")) continue;
        std::map<std::string, std::string> & g = gi["game"]; if (g.count("dup_of")) continue;   /* 자동 중복 숨김(dup_of)은 이 화면 대상이 아니다 */
        ui::TidyRow r; r.id = d; r.title = d;
        { auto t = g.find("title"); if (t != g.end() && !t->second.empty()) r.title = t->second; }
        { prof::Ini u; if (prof::ini_load(dir + "/user.ini", u) && u.count("game")) { auto t = u["game"].find("title_user"); if (t != u["game"].end() && !t->second.empty()) r.title = t->second; r.hidden = u["game"].count("tidy_hidden") && u["game"]["tidy_hidden"] == "1"; } }
        { auto s = g.find("srcdir"); if (s != g.end()) r.src = s->second; }
        std::string imgdir; { auto s = g.find("imgdir"); if (s != g.end()) imgdir = s->second; }
        if (!r.src.empty() && !plat::exists(aw::u8_to_acp(r.src))) r.st = 1;   /* 원본 경로(폴더 또는 zip)가 없다 */
        if (!imgdir.empty() && !aw::dir_exists(aw::u8_to_acp(imgdir))) r.st = 1;
        r.saves = gh_tidy_has_files(gApp.savedir + "\\" + d);
        Ex e; gh_tidy_names(g, e.names, e.shas); e.ns = r.src.empty() ? std::string() : aw::norm_path(r.src);
        U.tidy_rows.push_back(r); ex.push_back(e);
    }
    for (size_t i = 0; i < U.tidy_rows.size(); i++) for (size_t j = i + 1; j < U.tidy_rows.size(); j++) {   /* 같은 원본 경로 + 같은 이미지(이름 또는 sha1) = 중복 의심 */
        if (U.tidy_rows[i].hidden || U.tidy_rows[j].hidden || ex[i].ns.empty() || ex[i].ns != ex[j].ns) continue;
        bool same = false; for (auto & n : ex[i].names) if (ex[j].names.count(n)) { same = true; break; }
        if (!same) for (auto & h : ex[i].shas) if (ex[j].shas.count(h)) { same = true; break; }
        if (!same) continue;
        if (U.tidy_rows[i].st == 0) U.tidy_rows[i].st = 2; if (U.tidy_rows[j].st == 0) U.tidy_rows[j].st = 2;
    }
    U.tidy_mark.assign(U.tidy_rows.size(), 0); U.tidy_ia = U.tidy_ib = -1;
}
static void gh_tidy_after_reload(void) {
    ui::Ui & U = gApp.ui_; gh_reload_games();
    if (gApp.cur_game < 0 || gApp.cur_game >= (int)U.games.size()) { gApp.cur_game = 0; U.cur = 0; U.pick = 0; }
    gh_tidy_scan();
}
static bool gh_tidy_move_saves(const std::string & id) {   /* saves\<id> → saves\_old\<id>[_n]  (삭제 없음) */
    const std::string src = gApp.savedir + "\\" + id; if (!aw::dir_exists(src)) return true;
    const std::string root = gApp.savedir + "\\_old"; aw::make_dir(root);
    std::string dst = root + "\\" + id; int k = 2; while (aw::dir_exists(dst)) dst = root + "\\" + id + "_" + std::to_string(k++);
    return plat::move_file(src, dst);
}
static void gh_tidy_do(int arg, int slot) {
    ui::Ui & U = gApp.ui_; const std::string gd = gApp.exedir + "\\games";
    if (arg == 0) { gh_tidy_scan(); return; }
    if (arg == 1) {   /* 목록에서 빼기 (slot=1: 세이브도 _old 로) */
        int n = 0, sv = 0;
        for (auto & id : U.tidy_ids) { const std::string dir = gd + "\\" + id; if (!aw::dir_exists(dir)) continue;
            if (prof::set_game_key(dir, "tidy_hidden", "1")) { n++; if (slot == 1 && gh_tidy_move_saves(id)) sv++; } }
        gh_log("TIDY hide %d (saves moved %d)", n, sv); gh_tidy_after_reload(); U.tidy_msg = n ? phr_("목록에서 뺐습니다.") : phr_("빼지 못했습니다."); return;
    }
    if (arg == 2) {   /* 합치기: 세이브·사용자 제목을 남길 쪽으로, 없앨 쪽은 숨김 */
        const std::string kd = gd + "\\" + U.tidy_keep_id, dd = gd + "\\" + U.tidy_drop_id;
        if (U.tidy_keep_id.empty() || U.tidy_drop_id.empty() || U.tidy_keep_id == U.tidy_drop_id || !aw::dir_exists(kd) || !aw::dir_exists(dd)) { U.tidy_msg = phr_("합치지 못했습니다."); return; }
        std::string tk, td; { prof::Ini u; if (prof::ini_load(kd + "/user.ini", u) && u.count("game") && u["game"].count("title_user")) tk = u["game"]["title_user"]; }
        { prof::Ini u; if (prof::ini_load(dd + "/user.ini", u) && u.count("game") && u["game"].count("title_user")) td = u["game"]["title_user"]; }
        if (tk.empty() && !td.empty()) prof::set_game_key(kd, "title_user", td);
        int mv = 0; const std::string sk = gApp.savedir + "\\" + U.tidy_keep_id, sd = gApp.savedir + "\\" + U.tidy_drop_id;
        if (aw::dir_exists(sd)) { aw::make_dir(sk); std::vector<std::string> f, s; gh_list_dir(sd, f, &s);
            for (auto & n : f) if (!plat::exists(sk + "\\" + n) && plat::move_file(sd + "\\" + n, sk + "\\" + n)) mv++; }   /* 같은 이름이 이미 있으면 남길 쪽 것을 지키고 없앨 쪽 것은 그 자리에 둔다 */
        bool ok = prof::set_game_key(dd, "tidy_hidden", "1"); if (ok) prof::set_game_key(dd, "merged_into", U.tidy_keep_id);
        gh_log("TIDY merge %s <- %s (saves moved %d) ok=%d", U.tidy_keep_id.c_str(), U.tidy_drop_id.c_str(), mv, ok ? 1 : 0);
        gh_tidy_after_reload(); U.tidy_msg = ok ? phr_("합쳤습니다.") : phr_("합치지 못했습니다."); return;
    }
    if (arg == 3) {   /* 숨긴 항목 되살리기 */
        int n = 0; for (auto & id : U.tidy_ids) { const std::string dir = gd + "\\" + id; if (aw::dir_exists(dir) && prof::set_game_key(dir, "tidy_hidden", "")) n++; }
        gh_log("TIDY restore %d", n); gh_tidy_after_reload(); U.tidy_msg = n ? phr_("되살렸습니다.") : phr_("되살리지 못했습니다."); return;
    }
    if (arg == 4) {   /* 다시 찾기: 원본 폴더를 다시 지정 — 같은 이미지(이름 또는 sha1)가 그 폴더에 있어야 받아들인다 */
        if (U.tidy_ids.empty()) return; const std::string id = U.tidy_ids[0], gi_path = gd + "\\" + id + "\\game.ini";
        prof::Ini gi; if (!prof::ini_load(gi_path, gi) || !gi.count("game")) { U.tidy_msg = phr_("다시 찾지 못했습니다."); return; }
        std::map<std::string, std::string> & g = gi["game"]; std::set<std::string> names, shas; gh_tidy_names(g, names, shas);
        std::string d; if (!gh_pick_folder(phr_("원본이 있는 폴더를 고르세요"), d)) return;
        std::string src; { auto s = g.find("srcdir"); if (s != g.end()) src = s->second; }
        const std::string sl = aw::lower(src); const bool zip = sl.size() > 4 && sl.compare(sl.size() - 4, 4, ".zip") == 0;
        std::vector<std::string> fl, sb; gh_list_dir(d, fl, &sb); bool ok = false; std::vector<aw::KVu> up;
        if (zip) { const std::string zn = aw::lower(aw::base_name_of(src)); for (auto & f : fl) if (aw::lower(f) == zn) { ok = true; up.push_back({ "srcdir", aw::acp_to_u8(aw::join(d, f)), 3 }); } }   /* zip 작품: 이미지는 풀어 둔 사본이라 zip 위치만 고친다 */
        else {
            size_t hit = 0; for (auto & f : fl) { if (!aw::img_ext(f)) continue;
                if (!names.empty()) { if (names.count(aw::lower(f))) hit++; }
                else { std::string h = gApp.shacache.sha1(aw::join(d, f)); if (!h.empty() && shas.count(aw::lower(h))) hit++; } }
            ok = names.empty() ? hit > 0 : hit >= names.size();   /* 이름으로 알던 작품은 이미지가 전부 있어야, 해시뿐이면 한 장 이상 맞아야 */
            if (ok) { up.push_back({ "imgdir", aw::acp_to_u8(d), 3 }); up.push_back({ "srcdir", aw::acp_to_u8(d), 3 }); }
        }
        if (ok) { aw::ini_patch(gi_path, up); gh_log("TIDY relocate %s -> %s", id.c_str(), d.c_str()); gh_tidy_after_reload(); U.tidy_msg = phr_("원본 폴더를 다시 지정했습니다."); }
        else U.tidy_msg = phr_("그 폴더에서 같은 이미지를 찾지 못했습니다.");
        return;
    }
}
