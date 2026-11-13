/* app/ui_env.h -- S9b: UI environment, game list, titlebar, add-works, help, key map. Moved unchanged from gensei_host.cpp lines 841-1313 (as of t54t). Included in place; single translation unit. */
/* ---- 슬롯 정보·THMB */
static bool gh_png_decode(const std::vector<uint8_t> & png, std::vector<uint32_t> & out, int & w, int & h) {
    png_image im; memset(&im, 0, sizeof im); im.version = PNG_IMAGE_VERSION;
    if (!png_image_begin_read_from_memory(&im, png.data(), png.size())) return false;
    im.format = PNG_FORMAT_BGRA; w = (int)im.width; h = (int)im.height; out.assign((size_t)w * h, 0);
    bool ok = png_image_finish_read(&im, NULL, out.data(), 0, NULL) != 0; png_image_free(&im); return ok;
}
static void gh_refresh_game_slots(int gi) {
    ui::UiGame & G = gApp.ui_.games[gi]; G.lastslot = -1; G.thmb.clear(); unsigned long long best = 0;
    for (int i = 0; i < 5; i++) {
        ui::SlotInfo & s = G.slot[i]; s = ui::SlotInfo();
        std::string p = gh_slot_n(gi, i); plat::FileInfo fa;
        if (!plat::stat_file(p, fa)) continue;
        s.used = true; s.bytes = (int)fa.size_lo; { plat::FileInfo fd_; if (plat::stat_file(p + ".dsk", fd_)) s.bytes += (int)fd_.size_lo; }   /* 1b: 디스크 압축본 포함 */
        plat::LocalTime st = plat::local_from_filetime(fa.mtime_ft);
        char b[32]; snprintf(b, sizeof b, "%02d-%02d %02d:%02d", st.month, st.day, st.hour, st.minute); s.stamp = b;
        unsigned long long t = fa.mtime_ft;
        if (t >= best) { best = t; G.lastslot = i; }
        std::vector<uint8_t> png; std::string err;
        if (PC98_SaveState_ReadThumb(p.c_str(), png, err) && !png.empty()) gh_png_decode(png, s.th, s.tw_, s.th_);
    }
    if (G.lastslot >= 0) { G.thmb = G.slot[G.lastslot].th; G.tw_ = G.slot[G.lastslot].tw_; G.th_ = G.slot[G.lastslot].th_; }
}
/* t48e (#646): 키 이름 → 키. 옛 이름은 gh_keyname 이 먼저 받아 지금과 똑같이 동작하고(rollup/pageup/rolldown/pagedown 포함), 거기에 없는 이름만 kbtable 라벨 표(pn::lookup)로 찾는다 */
static KBD_KEYS gh_keyname_pad(const std::string & n) {
    KBD_KEYS k = gh_keyname(n); if (k != KBD_NONE) return k;
    return pn::lookup(n, gh_kb_ibm());
}
static const char * const GH_PADBTN[PB_N] = { "btn_b", "btn_a", "btn_x", "btn_start", "btn_leftshoulder", "btn_rightshoulder", "btn_back", "btn_y", "btn_dpup", "btn_dpdown", "btn_dpleft", "btn_dpright", "btn_lefttrigger", "btn_righttrigger", "btn_leftstick", "btn_rightstick", "btn_ext0", "btn_ext1", "btn_ext2", "btn_ext3", "btn_ext4", "btn_ext5", "btn_ext6", "btn_ext7" };   /* 빌드 5a: SDL 표준 버튼 이름 + 추가 버튼 배정 */
static const char * const GH_PADFN[FN_N] = { "", "menu", "boost_hold", "boost_toggle", "quick_save", "quick_load", "osk", "screenshot", "next_disk", "fps" };   /* 값 "fn:<이름>" */
static const int GH_PADIDX[PB_N] = { PB_EAST, PB_SOUTH, PB_WEST, PB_START, PB_L1, PB_R1, PB_BACK, PB_NORTH, PB_DUP, PB_DDOWN, PB_DLEFT, PB_DRIGHT, PB_L2, PB_R2, PB_L3, PB_R3, PB_X0, PB_X1, PB_X2, PB_X3, PB_X4, PB_X5, PB_X6, PB_X7 };
static void gh_hot_reload(void) {   /* build 5a-2: hotkey table = defaults + global btn_hot_<fn> (sel+<button> | <button> | none) */
    kh_hot_defaults();
    for (int n = 0; n < 8; n++) {   /* build 5a-2b: registered extra buttons btn_xreg<n> = key|<code>|<name> or joy|<n>|<name> */
        kh_xb[n].kind = 0; kh_xb[n].code = 0;
        auto it = gApp.ui_.gpad.find(std::string("btn_xreg") + std::to_string(n)); if (it == gApp.ui_.gpad.end()) continue;
        const std::string & v = it->second; const size_t a = v.find('|'), b = a == std::string::npos ? a : v.find('|', a + 1); if (b == std::string::npos) continue;
        kh_xb[n].kind = v.compare(0, a, "joy") == 0 ? 2 : v.compare(0, a, "key") == 0 ? 1 : 0; kh_xb[n].code = atoi(v.substr(a + 1, b - a - 1).c_str());
        GPLOG("XBTN%d kind=%d code=%d", n, kh_xb[n].kind, kh_xb[n].code); }
    for (int q = 2; q < FN_N; q++) {
        if (q == FN_BOOST_HOLD || q == FN_BOOST_TOG) continue;
        auto it = gApp.ui_.gpad.find(std::string("btn_hot_") + GH_PADFN[q]); if (it == gApp.ui_.gpad.end() || it->second.empty()) continue;
        std::string s = it->second; bool sl = false; if (s.compare(0, 4, "sel+") == 0) { sl = true; s = s.substr(4); }
        int b = -1; if (s != "none") for (int i = 0; i < PB_N; i++) if (std::string(GH_PADBTN[i]).compare(4, std::string::npos, s) == 0) b = GH_PADIDX[i];
        if (s != "none" && b < 0) { gh_log("pad: hotkey %s=%s unknown name, ignored", GH_PADFN[q], it->second.c_str()); continue; }
        if (b == PB_START || b == PB_BACK) continue;
        kh_hot[q].btn = b; kh_hot[q].sel = sl; }
    GPLOG("HOTKEYS qs=%d/%d ql=%d/%d osk=%d/%d shot=%d/%d disk=%d/%d fps=%d/%d", kh_hot[FN_QSAVE].btn, (int)kh_hot[FN_QSAVE].sel, kh_hot[FN_QLOAD].btn, (int)kh_hot[FN_QLOAD].sel, kh_hot[FN_OSK].btn, (int)kh_hot[FN_OSK].sel, kh_hot[FN_SHOT].btn, (int)kh_hot[FN_SHOT].sel, kh_hot[FN_NEXTDISK].btn, (int)kh_hot[FN_NEXTDISK].sel, kh_hot[FN_FPS].btn, (int)kh_hot[FN_FPS].sel);
}
static bool gh_pad_parse(const std::string & v, GPadBind & b) {   /* none / mouse_left / mouse_right / 키 이름 */
    std::string s = v; for (auto & c : s) c = (char)tolower((unsigned char)c);
    b.fn = 0;
    if (s == "none") { b.type = PBT_NONE; b.key = KBD_NONE; return true; }
    if (s == "mouse_left") { b.type = PBT_ML; b.key = KBD_NONE; return true; }
    if (s == "mouse_right") { b.type = PBT_MR; b.key = KBD_NONE; return true; }
    if (s.compare(0, 3, "fn:") == 0) { for (int q = 1; q < FN_N; q++) if (s.compare(3, std::string::npos, GH_PADFN[q]) == 0) { b.type = PBT_FN; b.key = KBD_NONE; b.fn = q; return true; } return false; }
    KBD_KEYS k = gh_keyname_pad(s); if (k == KBD_NONE) return false; b.type = PBT_KEY; b.key = k; return true;
}
/* 프로필의 패드 표·never_map 을 입력 층에 적용 (#237: never_map 은 패드·단축키에 절대 배정 금지)
 * t48e: 순서 = 기본값(서양식이면 동·남 맞바꿈) < 전역 settings.ini btn_* < 프로필 옛 o/x/sq < 프로필 btn_* (이 작품 user.ini [pad] 가 가장 늦게 얹혀 이긴다).
 * refresh = true 면 (게임 도중 조작 설정을 바꿨을 때) 이 작품 user.ini [pad] 의 btn_* 를 파일에서 다시 읽는다 */
static void gh_apply_profile_input(const GProfile & p, bool refresh = false) {
    memset(kh_never, 0, sizeof(kh_never));
    std::vector<std::string> nm_all;   /* t48e3 (#662): never_map 은 ',' 와 '+' 둘 다 구분자 (예: XFER+ROLLUP+ROLLDOWN) */
    for (auto & part : prof::split_list(p.never_map)) for (auto & sub : prof::split_list(part, '+')) nm_all.push_back(sub);
    for (auto & nm : nm_all) { KBD_KEYS k = gh_keyname_pad(nm); if (k != KBD_NONE && (int)k < 256) { kh_never[(int)k] = true; gh_log("never_map: %s -> key %d", nm.c_str(), (int)k); } else gh_log("never_map: %s -> (알 수 없는 이름, 무시)", nm.c_str()); }
    if (!refresh) kh_western = (gApp.cur_game >= 0 && gApp.cur_game < (int)gApp.ui_.games.size() ? gApp.ui_.effective(gApp.cur_game) : gApp.ui_.g).pad == 1;
    kh_pb_defaults();
    std::map<std::string, std::string> pad = p.pad;
    if (refresh && gApp.cur_game >= 0 && gApp.cur_game < (int)gApp.ui_.games.size()) {
        for (auto it = pad.begin(); it != pad.end();) { if (it->first.compare(0, 4, "btn_") == 0) it = pad.erase(it); else ++it; }
        std::map<std::string, std::string> m; prof::get_pad_keys(gApp.ui_.games[gApp.cur_game].dir, m); for (auto & kv : m) pad[kv.first] = kv.second;
    }
    gh_hot_reload();
    /* 1) 전역 btn_* */
    for (int i = 0; i < PB_N; i++) { auto it = gApp.ui_.gpad.find(GH_PADBTN[i]); if (it != gApp.ui_.gpad.end() && !it->second.empty()) { GPadBind b; if (gh_pad_parse(it->second, b)) kh_pb[GH_PADIDX[i]] = b; else gh_log("pad: 전역 %s=%s 알 수 없는 이름, 무시", GH_PADBTN[i], it->second.c_str()); } }
    /* 2) 프로필 옛 이름 o/x/sq — 옛 gh_keyname 으로 지금과 똑같이 */
    auto get = [&](const char * k) -> std::string { auto it = pad.find(k); return it == pad.end() ? std::string() : it->second; };
    std::string o = get("o"), x = get("x"), sq = get("sq");
    /* t48e3 (#662): 옛 o=/x=/sq= 값이 기본값(space/esc/caps)과 같으면 「기본 따름」 — 결정 방식(일본식/서양식)과 전역 btn_* 을 따른다. 기본과 다른 옛 값만 지금처럼 우선 */
    { auto lc = [](std::string s) { for (auto & c : s) c = (char)tolower((unsigned char)c); return s; };
      if (lc(o) == "space") { gh_log("pad: 옛 o=%s = 기본값 → 기본 따름", o.c_str()); o.clear(); }
      if (lc(x) == "esc") { gh_log("pad: 옛 x=%s = 기본값 → 기본 따름", x.c_str()); x.clear(); }
      if (lc(sq) == "caps") { gh_log("pad: 옛 sq=%s = 기본값 → 기본 따름", sq.c_str()); sq.clear(); } }
    if (o == "mouse_left") { kh_padmouse = true; gh_log("pad: mouse mode (o=mouse_left)"); }
    else if (!o.empty()) { KBD_KEYS k = gh_keyname(o); if (k != KBD_NONE) { kh_pb[PB_EAST].type = PBT_KEY; kh_pb[PB_EAST].key = k; } }
    if (!x.empty() && x != "mouse_right") { KBD_KEYS k = gh_keyname(x); if (k != KBD_NONE) { kh_pb[PB_SOUTH].type = PBT_KEY; kh_pb[PB_SOUTH].key = k; } }
    if (!sq.empty()) { KBD_KEYS k = gh_keyname(sq); if (k != KBD_NONE) { kh_pb[PB_WEST].type = PBT_KEY; kh_pb[PB_WEST].key = k; } }
    /* 3) 프로필 btn_* (옛 이름 위에 얹는다) */
    for (int i = 0; i < PB_N; i++) { std::string v = get(GH_PADBTN[i]); if (v.empty()) continue; GPadBind b; if (gh_pad_parse(v, b)) kh_pb[GH_PADIDX[i]] = b; else gh_log("pad: 프로필 %s=%s 알 수 없는 이름, 무시", GH_PADBTN[i], v.c_str()); }
    /* never_map 에 걸린 키는 옛 규칙대로 되돌린다: 동 → Space(없으면 Esc) · 남 → Esc · 서 → 없음. 그 밖의 버튼은 없음 */
    for (int i = 0; i < PB_N; i++) if (kh_pb[i].type == PBT_KEY && (int)kh_pb[i].key < 256 && kh_never[(int)kh_pb[i].key]) {
        if (i == PB_EAST) kh_pb[i].key = KBD_space; else if (i == PB_SOUTH) kh_pb[i].key = KBD_esc; else { kh_pb[i].type = PBT_NONE; kh_pb[i].key = KBD_NONE; } }
    auto nm = [](const GPadBind & b) -> std::string { if (b.type == PBT_NONE) return "none"; if (b.type == PBT_ML) return "mouse_left"; if (b.type == PBT_MR) return "mouse_right"; if (b.type == PBT_FN) return std::string("fn:") + GH_PADFN[b.fn > 0 && b.fn < FN_N ? b.fn : 0]; return std::to_string((int)b.key); };
    gh_log("pad: western=%d east=%s south=%s west=%s start=%s l1=%s r1=%s select=%s%s", kh_western ? 1 : 0, nm(kh_pb[PB_EAST]).c_str(), nm(kh_pb[PB_SOUTH]).c_str(), nm(kh_pb[PB_WEST]).c_str(), nm(kh_pb[PB_START]).c_str(), nm(kh_pb[PB_L1]).c_str(), nm(kh_pb[PB_R1]).c_str(), nm(kh_pb[PB_BACK]).c_str(), refresh ? " (갱신)" : "");
    GPLOG("BINDS2 north=%s dpup=%s dpdown=%s dpleft=%s dpright=%s l2=%s r2=%s l3=%s r3=%s", nm(kh_pb[PB_NORTH]).c_str(), nm(kh_pb[PB_DUP]).c_str(), nm(kh_pb[PB_DDOWN]).c_str(), nm(kh_pb[PB_DLEFT]).c_str(), nm(kh_pb[PB_DRIGHT]).c_str(), nm(kh_pb[PB_L2]).c_str(), nm(kh_pb[PB_R2]).c_str(), nm(kh_pb[PB_L3]).c_str(), nm(kh_pb[PB_R3]).c_str());
    GPLOG("BINDS western=%d east=%s south=%s west=%s start=%s l1=%s r1=%s select=%s", kh_western ? 1 : 0, nm(kh_pb[PB_EAST]).c_str(), nm(kh_pb[PB_SOUTH]).c_str(), nm(kh_pb[PB_WEST]).c_str(), nm(kh_pb[PB_START]).c_str(), nm(kh_pb[PB_L1]).c_str(), nm(kh_pb[PB_R1]).c_str(), nm(kh_pb[PB_BACK]).c_str());
}

/* ------------------------------------------------------------------ t39: 실행 방법 UI 가 쓰는 환경(파일·저장) */
#define KH_WT_SUFFIX " - " KH_DISPNAME /*E1R*/   /* t49a (#811): 창 제목 접미(항상 전부 남김, 30바이트) */
#define KH_WT_TITLEMAX (113 - (int)(sizeof(KH_WT_SUFFIX) - 1))                /* 제목 쪽 상한 = 옛 합계 113 - 접미 30 */
static std::string gh_window_title(const ui::UiGame & tg, bool legacy) {   /* 작품 제목만 줄임, 한글 3바이트 경계 유지 */
    std::string tt = tg.title;
    if (!legacy && tt.size() > KH_WT_TITLEMAX) { if (!tg.short_t.empty()) tt = tg.short_t; if (tt.size() > KH_WT_TITLEMAX) { size_t k = KH_WT_TITLEMAX - 3; while (k > 0 && ((unsigned char)tt[k] & 0xC0) == 0x80) k--; tt = tt.substr(0, k) + "..."; } }
    return tt + KH_WT_SUFFIX;
}
static bool gh_make_blank_hdi(const std::string & path, int mb) {
    const uint32_t ss = 256, sec = 33, sur = 8; uint64_t cyl = ((uint64_t)(mb < 1 ? 1 : mb) * 1048576ull) / (ss * sec * sur); if (cyl < 1) cyl = 1; if (cyl > 4096) cyl = 4096;
    uint32_t sz = (uint32_t)(ss * sec * sur * cyl);
    FILE * f = fopen(path.c_str(), "wb"); if (!f) return false;
    uint32_t h[8] = { 0, 0, 0x1000, sz, ss, sec, sur, (uint32_t)cyl }; fwrite(h, 1, sizeof h, f);
    static unsigned char z[65536]; size_t pad = 0x1000 - sizeof h; bool ok = fwrite(z, 1, pad, f) == pad;
    for (uint32_t left = sz; ok && left;) { size_t k = left > sizeof z ? sizeof z : left; ok = fwrite(z, 1, k, f) == k; left -= (uint32_t)k; }
    fclose(f); if (!ok) plat::remove_file(path.c_str()); return ok;
}
static void gh_ui_env_setup(void) {
    ui::Env & E = gApp.ui_.env; E.sep = '\\';
    E.list_dir = [](const std::string & p) {
        std::vector<ui::FileItem> r; std::string pa = aw::sys_from_ini(p); std::vector<std::string> fs, ds; aw::list_dir(pa, fs, &ds);
        std::sort(ds.begin(), ds.end()); std::sort(fs.begin(), fs.end());
        for (auto & d : ds) { ui::FileItem f; f.name = aw::acp_to_u8(d); f.dir = true; r.push_back(f); }
        for (auto & n : fs) { ui::FileItem f; f.name = aw::acp_to_u8(n); uint64_t sz = 0, mt = 0; if (aw::file_stat(aw::join(pa, n), sz, mt)) f.size = (long long)sz; r.push_back(f); }
        return r; };
    E.roots = []() { return plat::drive_roots(); };
    E.has_img = [](int g, const std::string & n) {
        if (n.empty() || g < 0 || g >= (int)gApp.ui_.games.size()) return false; std::string pa = aw::sys_from_ini(n);
        bool abs = (pa.size() > 1 && pa[1] == ':') || pa[0] == '\\' || pa[0] == '/'; std::string full = abs ? pa : aw::join(gApp.ui_.imgdir_of(g), pa);
        uint64_t sz = 0, mt = 0; return aw::file_stat(full, sz, mt); };
    E.save = [](int g, const std::string & name, const ent::Kv & kv, const std::string & setdef) { return prof::save_entry(gApp.ui_.games[g].dir, name, kv, setdef); };
    E.sha_of = [](int g, const std::string & n) {
        if (n.empty() || g < 0 || g >= (int)gApp.ui_.games.size()) return std::string();
        const std::string pa = aw::sys_from_ini(n);
        const bool abs = (pa.size() > 1 && pa[1] == ':') || pa[0] == '\\' || pa[0] == '/';
        return gApp.shacache.sha1(abs ? pa : aw::join(gApp.ui_.imgdir_of(g), pa)); };
    E.find_sha = [](int g, const std::string & pfx) {
        auto scan = [&](int gi, bool own) {
            const std::string dir = gApp.ui_.imgdir_of(gi); std::vector<std::string> fs, ds; aw::list_dir(dir, fs, &ds);
            for (auto & f : fs) {
                if (ent::img_kind(f) == 0) continue;
                const std::string h = gApp.shacache.sha1(aw::join(dir, f));
                if (h.size() >= pfx.size() && h.compare(0, pfx.size(), pfx) == 0) return aw::acp_to_u8(own ? f : aw::join(dir, f));
            }
            return std::string(); };
        std::string r = (g >= 0 && g < (int)gApp.ui_.games.size()) ? scan(g, true) : std::string();
        for (int gi = 0; r.empty() && gi < (int)gApp.ui_.games.size(); gi++) if (gi != g) r = scan(gi, false);
        return r; };
    E.clip_get = []() { char * t = SDL_GetClipboardText(); std::string s = t ? t : ""; if (t) SDL_free(t); return s; };
    E.clip_set = [](const std::string & s) { SDL_SetClipboardText(s.c_str()); };
    E.khx_write = [](int g, const std::string & name, const std::string & text) {
        std::string fn = ent::clean_name(name); for (auto & ch : fn) if (strchr("\\/:*?\"<>|", ch)) ch = '_';
        FILE * f = fopen(aw::join(gApp.ui_.games[g].dir, aw::u8_to_acp(fn) + ".khx").c_str(), "wb"); if (!f) return false;
        fwrite(text.data(), 1, text.size(), f); fputc('\n', f); fclose(f); return true; };
    E.khx_list = [](int g) {
        std::vector<std::string> r, fs, ds; aw::list_dir(gApp.ui_.games[g].dir, fs, &ds); std::sort(fs.begin(), fs.end());
        for (auto & f : fs) if (ent::ext_of(f) == "khx") r.push_back(aw::acp_to_u8(f));
        return r; };
    E.khx_read = [](int g, const std::string & n) {
        FILE * f = fopen(aw::join(gApp.ui_.games[g].dir, aw::u8_to_acp(n)).c_str(), "rb"); if (!f) return std::string();
        char b[65536]; size_t m = fread(b, 1, sizeof b, f); fclose(f); return std::string(b, m); };
    E.del = [](int g, const std::string & name, bool reset) { return prof::delete_entry(gApp.ui_.games[g].dir, name, reset); };
    E.setkey = [](int g, const std::string & k, const std::string & v) { return prof::set_game_key(gApp.ui_.games[g].dir, k, v); };
    E.pad_get = [](int g, std::map<std::string, std::string> & m) { m.clear(); if (g >= 0 && g < (int)gApp.ui_.games.size()) prof::get_pad_keys(gApp.ui_.games[g].dir, m); };   /* t48e (#646): 이 작품 user.ini [pad] btn_* */
    E.pad_set = [](int g, const std::string & k, const std::string & v) { if (g < 0 || g >= (int)gApp.ui_.games.size()) return false; bool ok = prof::set_pad_key(gApp.ui_.games[g].dir, k, v); gh_log("pad_set: %s [pad] %s=%s -> %s", gApp.ui_.games[g].dir.c_str(), k.c_str(), v.c_str(), ok ? "OK" : "FAIL"); return ok; };
    E.padkind = []() { const int k = gensei_padkind(); return (k == 0 && gensei_pad_retro) ? 3 : k; };
    E.cheat = gh_cheat_op;   /* kx-cheat */
    E.raw_ctl = [](int m) { kh_raw_on = m != 0; kh_xreg_wait = m == 2; kh_xreg.clear(); kh_raw_exit = false; if (m) { kh_raw_n = 0; for (int i = 0; i < 8; i++) kh_raw_l[i].clear(); kh_raw_t0 = SDL_GetTicks(); } };
    E.raw_line = [](int i) -> std::string { if (i == -2) { std::string r = kh_xreg; kh_xreg.clear(); return r; } if (i < 0) return (kh_raw_exit || (kh_raw_on && SDL_GetTicks() - kh_raw_t0 > 180000u)) ? "EXIT" : ""; return i < 8 ? kh_raw_l[i] : std::string(); };
    E.pad_cap = [](int m, int & pb, bool & sel) -> int { if (m == 0) { kh_cap_begin(true); return 0; } if (m == 1) return kh_cap_poll(pb, sel); if (m == 2) { kh_cap_begin(false); return 0; } gh_hot_reload(); return 0; };
    E.padkeys = []() { return pn::picker_names(); };
    E.kbcells = []() { std::vector<ui::KbCell> r; for (auto & e : pn::table()) { const kbt::Key & k = kbt::g_keys[e.idx]; ui::KbCell c; c.label = k.label; c.name = e.name; c.x4 = k.x4; c.w4 = k.w4; c.row = k.row; c.rows = k.rows; r.push_back(c); } return r; };
    E.hdd_make = [](int g, int mb, std::string & out) {
        std::string dir = gApp.ui_.imgdir_of(g), id = gApp.ui_.games[g].id; for (auto & ch : id) if (!(isalnum((unsigned char)ch) || ch == '_' || ch == '-')) ch = '_';
        for (int i = 1; i < 100; i++) { std::string nm = id + "_hdd_" + std::to_string(i) + ".hdi"; uint64_t sz, mt; if (aw::file_stat(aw::join(dir, nm), sz, mt)) continue;
            if (!gh_make_blank_hdi(aw::join(dir, nm), mb)) return false; out = nm; gh_log("hdd_make: %s %dMB", nm.c_str(), mb); return true; }
        return false; };
    E.save_test = [](int g, const std::string & name, const ent::Kv & kv) {
        (void)g; std::string td = gApp.exedir + "\\logs\\_test"; plat::mkdir((gApp.exedir + "\\logs").c_str()); plat::mkdir(td.c_str());
        plat::remove_file((td + "\\user.ini").c_str()); return prof::save_entry(td, name, kv, name); };
    E.resolved = [](int g, ent::Kv & out) {
        out.clear(); if (g < 0 || g >= (int)gApp.ui_.games.size()) return false;
        std::string keep = prof::g_entry_req; prof::g_entry_req.clear(); GProfile p; bool ok = gh_profile(gApp.ui_.games[g].id, p); prof::g_entry_req = keep; if (!ok) return false;
        static const char * K[4] = { "disk_a", "disk_b", "disk_c", "disk_d" };
        for (int i = 0; i < 4; i++) if (p.disk[i]) out[K[i]] = aw::acp_to_u8(p.disk[i]);
        out["boot"] = std::string(1, p.bootletter); { auto it = p.kv.find("cd"); if (it != p.kv.end() && !it->second.empty()) out["cd"] = it->second; }
        for (const char * k : { "mouse", "font", "font_kind", "bootkeys", "autoexec", "dosexe", "dosargs" }) { auto it = p.kv.find(k); if (it != p.kv.end() && !it->second.empty()) out[k] = it->second; }
        return true; };
    E.exelist = [](int g, const std::string & dk, const std::string & dir, std::vector<ui::FileItem> & out) {   /* 2b: 이미지 안 파일 목록 */
        out.clear(); if (g < 0 || g >= (int)gApp.ui_.games.size() || dk.empty()) return false;
        std::string keep = prof::g_entry_req; prof::g_entry_req.clear(); GProfile p; bool ok = gh_profile(gApp.ui_.games[g].id, p); prof::g_entry_req = keep; if (!ok) return false;
        std::string full = aw::u8_to_acp(dk); if (!(full.size() > 1 && (full[1] == (char)58 || full[0] == (char)47))) full = p.imgdir + "\\" + full;   /* 이름이면 작품 이미지 폴더 기준 */
        std::vector<prof::DirEnt> v; if (!prof::dos_browse(full, dir, v)) return false;
        for (auto & e : v) { ui::FileItem x; x.name = e.name; x.dir = e.dir; x.size = (long long)e.size; out.push_back(x); } return true; };
    E.reload = [](int g) { ulist::reload_one(gApp.ui_.games, g, &gApp.vlabel); int keep = gApp.ui_.cur; gApp.ui_.rebuild_vis(keep); };
    E.values = [](int g, std::vector<ui::ValRow> & out) {   /* t48a (#614 ③): 키별 최종값·출처·덮인 값(기본 항목 기준) */
        out.clear(); if (g < 0 || g >= (int)gApp.ui_.games.size()) return false;
        std::string keep = prof::g_entry_req; prof::g_entry_req.clear(); GProfile p; bool ok = gh_profile(gApp.ui_.games[g].id, p); prof::g_entry_req = keep; if (!ok) return false;
        static const char * const PRI[] = { "title", "disk_a", "disk_b", "disk_c", "disk_d", "boot", "swap", "cd", "keys", "bootkeys", "autoexec", "cycles", "font", "font_kind", "lang_eff", "lang", "mouse", "sound", "joystick", "entry_name" };
        std::vector<std::string> ks; for (auto & kv : p.kv) if (kv.first.compare(0, 6, "__val.") == 0) ks.push_back(kv.first.substr(6));
        auto pr = [&](const std::string & k) -> int { for (size_t i = 0; i < sizeof PRI / sizeof PRI[0]; i++) if (k == PRI[i]) return (int)i; return 1000; };
        std::stable_sort(ks.begin(), ks.end(), [&](const std::string & a, const std::string & b) { int x = pr(a), y = pr(b); return x != y ? x < y : a < b; });
        for (auto & k : ks) { ui::ValRow r; r.key = k; r.val = p.kv["__val." + k]; auto s = p.kv.find("__src." + k); r.src = s == p.kv.end() ? std::string() : s->second; auto c = p.kv.find("__cov." + k); r.cov = c == p.kv.end() ? std::string() : c->second; if (r.val.empty()) r.val = phr_("(비어 있음)"); out.push_back(r); }
        { ui::ValRow r; r.key = phr_("(실행 막힘 사유)"); r.val = gApp.ui_.games[g].block.empty() ? std::string(phr_("없음 (실행 가능)")) : gApp.ui_.games[g].block; r.src = phr_("계산"); out.push_back(r); }
        return true; };
    E.font_group = [](const std::string & nm) { for (auto & f : gApp.ui_.fonts) if (f.name == nm) return f.group; return std::string(); };
}
static void gh_delete_slot(int gi, int slot) { plat::remove_file(gh_slot_n(gi, slot).c_str()); plat::remove_file((gh_slot_n(gi, slot) + ".dsk").c_str()); gh_refresh_game_slots(gi); }   /* S3 retro-test token */
static void gh_spawn_test(const std::string & id, const std::string & entry, int secs) {
    std::string exe = plat::exe_path(); std::string td = gApp.exedir + "\\logs\\_test";
    std::string cmd = "\"" + exe + "\" \"" + id + "\" --entry \"" + aw::u8_to_acp(entry) + "\" --userini \"" + td + "\\user.ini\" --sec " + std::to_string(secs > 0 ? secs : 20) + " --bkosd 1 --savedir \"" + td + "\\saves\" --windowed --noboot --out \"" + td + "\"";
    unsigned long perr = 0;
    if (plat::spawn(exe, cmd, gApp.exedir, &perr)) { gh_log("test run: %s", cmd.c_str()); }
    else gh_log("test run FAIL err=%lu", (unsigned long)perr);
}
static void gh_launch_note(ui::Ui & U, const ui::Act & a) {   /* 실행 요청의 항목 이름을 프로필 결정에 넘기고 최근 실행 시각을 적는다 */
    prof::g_entry_req = (a.t == ui::A_START) ? a.entry : (a.entry.empty() ? U.sel_entry : a.entry);
    if (a.game >= 0 && a.game < (int)U.games.size()) U.games[a.game].last_run = (long long)time(NULL);
}
/* t726: DB 작품의 디스크 자리는 sha1 로 적혀 있다 — 교체 목록에는 imgdir 에서 그 해시의 실제 이미지 이름을 찾아 보이고, 못 찾으면 「디스크 n」 */
static std::string gh_fd_real(const GProfile & p, const std::string & h, const std::vector<std::string> & order) {
    std::vector<std::string> fl; aw::list_dir(p.imgdir, fl, NULL);
    for (auto & f : fl) { if (!aw::img_ext(f)) continue; const std::string x = gApp.shacache.sha1(aw::join(p.imgdir, f)); if (!x.empty() && aw::lower(x) == aw::lower(h)) { std::string n = f; size_t dd = n.find_last_of('.'); if (dd != std::string::npos && n.size() - dd <= 5) n = n.substr(0, dd); return n; } }
    size_t k = 0; for (; k < order.size(); k++) if (order[k] == h) break;
    return std::string(phr_("디스크")) + " " + std::to_string(k + 1);
}
static void gh_rebuild_fds(const GProfile & p, ui::UiGame & g) {   /* 디스크 교체 후보 = 지금 실행하는 항목의 배치 */
    std::string demo = p.kv.count("demo") ? p.kv.at("demo") : std::string(); std::vector<std::string> files; auto addf = [&](const char * f) { if (!f || !*f) return; for (auto & x : files) if (x == f) return; files.push_back(f); };
    for (int i = 0; i < 2; i++) addf(p.disk[i]); for (auto & s : p.swap) addf(s.c_str()); addf(demo.c_str());
    auto flb = [&](const std::string & f) -> std::string { std::string n = f; if (aw::is_hex40(n)) { std::vector<std::string> ord; for (int q = 0; q < 2; q++) if (p.disk[q] && *p.disk[q]) ord.push_back(p.disk[q]); for (auto & s : p.swap) ord.push_back(s); return gh_fd_real(p, n, ord); } size_t dd = n.find_last_of('.'); if (dd != std::string::npos && n.size() - dd <= 5) n = n.substr(0, dd); return n; };
    g.fds.clear(); g.cur_fd[0].clear(); g.cur_fd[1].clear(); for (auto & f : files) g.fds.push_back(flb(f));
    for (int i = 0; i < 2; i++) if (p.disk[i]) g.cur_fd[i] = flb(p.disk[i]);
    gApp.fdfiles[p.id] = files;
}
static void gh_font_rebuild(void) {   /* E3R: 글꼴 목록 합집합(①fontdir ②내부 ③SD ④등록 ⑤앱, 이름 중복은 먼저 것만) */
    ui::Ui & U = gApp.ui_;
    std::vector<std::string> ds; kha_font_dirs(ds);
    std::string exd = aw::dir_of(gh_res_font_path("kyuhachi_font"));
    bool have = false;
    for (size_t i = 0; i < ds.size(); i++) if (ds[i] == exd) have = true;
    if (!have) ds.push_back(exd);
    U.fonts.clear(); std::set<std::string> seen;
    for (size_t i = 0; i < ds.size(); i++) {
        std::vector<std::string> ff; aw::list_dir(ds[i], ff, NULL);
        std::vector<fontreg::Info> inf = fontreg::scan(ds[i], ff, "\\");
        for (size_t k = 0; k < inf.size(); k++) {
            if (!seen.insert(inf[k].name).second) continue;
            ui::UiFont u; u.name = inf[k].name; u.group = inf[k].group;
            u.path = inf[k].path; u.why = inf[k].why; u.ok = inf[k].ok;
            U.fonts.push_back(u);
        }
    }
}
static stem::Table g_stem;   /* #L6: 실행 파일 이름 줄기 점수표(gamedb_nk_stem.tsv, 없으면 로드 안 됨 = 기능 꺼짐) */
struct StemJob { std::string id; std::vector<std::string> paths; bool hd = false; };
static std::deque<StemJob> g_stemq; static bool g_stem_relo = false;   /* #L6: 목록이 뜬 뒤 한 프레임에 한 작품씩 판정(이미지당 2초 상한 — 넘으면 조용히 「미확인」). 결과는 game.ini 에 남겨 다음부터 다시 읽지 않음 */
static int g_stem_skip_gid = 0, g_stem_skip_done = 0; static bool g_stem_qlogged = false;   /* #P3-4: 대기열 진단 — 목록 구성 때 세고, 첫 gh_stem_step 에서 한 줄 */
static void gh_reload_games(void);
static void gh_stem_step(void) {
    if (!g_stem_qlogged) { g_stem_qlogged = true; gh_log("STEM queue=%d skip=%d(이미 gid 있음) skip2=%d(stem_done·nk_gid·variant_of 있음) table=%d", (int)g_stemq.size(), g_stem_skip_gid, g_stem_skip_done, g_stem.loaded ? 1 : 0); if (gApp.log) fflush(gApp.log); }
    if (g_stemq.empty()) { if (g_stem_relo) { g_stem_relo = false; gh_reload_games(); } return; }
    StemJob j = g_stemq.front(); g_stemq.pop_front(); const std::string gini = gApp.exedir + "\\games\\" + j.id + "\\game.ini"; uint64_t fsz = 0, fmt_ = 0; if (!g_stem.loaded || !aw::file_stat(gini, fsz, fmt_)) { gh_log("STEM %s → none(%s)", j.id.c_str(), !g_stem.loaded ? "표 없음" : "game.ini 없음"); if (gApp.log) fflush(gApp.log); return; }
    std::vector<prof::ImgInfo> ims; for (auto & f : j.paths) ims.push_back(prof::probe(f));
    std::set<std::string> st; stem::work_stems(ims, st, 2000); stem::Verdict v = stem::judge(g_stem, st);
    if (v.autolink && !v.top.empty()) { aw::ini_patch(gini, { { "boot_gid", stem::pick_gid(g_stem.g[v.top[0].g], j.hd), 0 }, { "boot_how", "stem", 0 }, { "stem_done", "1", 3 } }); g_stem_relo = true; }
    else { std::string cp = v.top.empty() ? std::string() : stem::cand_pack(g_stem, v, j.hd); if (cp.empty()) aw::ini_patch(gini, { { "stem_done", "1", 3 } }); else { aw::ini_patch(gini, { { "stem_cand", cp, 3 }, { "stem_done", "1", 3 } }); for (auto & g : gApp.ui_.games) if (g.id == j.id) g.stemc = cp; } }
    { const bool au = v.autolink && !v.top.empty(); std::string cp2 = (!au && !v.top.empty()) ? stem::cand_pack(g_stem, v, j.hd) : std::string();
      std::string res = au ? std::string("auto ") + stem::pick_gid(g_stem.g[v.top[0].g], j.hd) : (!cp2.empty() ? std::string("cand ") + std::to_string(std::count(cp2.begin(), cp2.end(), '|') + 1) : std::string("none(") + (st.empty() ? "줄기 0개 — 이미지를 못 읽었거나 실행 파일 없음" : "표에 맞는 작품 없음") + ")");
      gh_log("STEM %s → %s  [이미지 %d개 · 줄기 %d개 · 남은 대기 %d]", j.id.c_str(), res.c_str(), (int)ims.size(), (int)st.size(), (int)g_stemq.size()); if (gApp.log) fflush(gApp.log); }
}
/* 원본 경로(game.ini 의 UTF-8)가 정말 없는지. ANSI 로 못 바꾼 한글 경로(ACP 가 한글이 아닌 PC)는 읽을 수 없으니 「모름」= 없다고 하지 않는다 (t722 결함: 한글 srcdir 작품이 전부 「원본 없음」으로 막혔다) */
static bool gh_path_gone(const std::string & u8, bool isdir) {
    if (u8.empty()) return false;
#if defined(_WIN32)
    /* t723: ANSI 로 못 바꾸는 한글 경로는 u8_to_acp 가 '?' 로 바꿔 돌려줘(57be024 의 「그대로 돌려줌」 가정이 틀렸다) 있는 폴더도 없다고 나왔다 — 유니코드 API 로 직접 본다 */
    std::wstring w; if (!plat::utf8_to_wide(u8, w)) return false;
    const DWORD at = GetFileAttributesW(w.c_str()); if (at == INVALID_FILE_ATTRIBUTES) return true;
    return isdir ? !(at & FILE_ATTRIBUTE_DIRECTORY) : false;
#else
    return isdir ? !aw::dir_exists(u8) : !plat::exists(u8);
#endif
}
static bool gh_tidy_hidden(const std::string & dir) {   /* kx-clean: 목록 정리에서 뺀 작품(user.ini [game] tidy_hidden=1) — 목록에서만 숨긴다 */
    prof::Ini u; return prof::ini_load(dir + "/user.ini", u) && u.count("game") && u["game"].count("tidy_hidden") && u["game"]["tidy_hidden"] == "1";
}
static void gh_nk_load_once(void);   /* #L5: 목록을 만들 때 기동 상태표를 쓰므로 미리 선언(정의는 아래 r017 절) */
static void gh_ui_init_games(void) {
    struct ErGuard { std::string k; ErGuard() : k(prof::g_entry_req) { prof::g_entry_req.clear(); } ~ErGuard() { prof::g_entry_req = k; } } er_guard;   /* t48a (#615/#616): 직전 실행의 항목 이름(전역 한 칸)이 모든 작품의 목록 판정에 끼지 않게 한다 */
    gApp.ui_.games.clear(); gApp.vlabel.clear(); gh_ui_env_setup();
    gh_font_rebuild();   /* E3R: 합집합 ①fontdir ②내부 ③SD ④등록 ⑤앱 (t39 단일 폴더 대체) */
    struct E { int order; ui::UiGame g; };
    std::vector<E> es;
    std::vector<std::string> files, subs; gh_list_dir(gApp.exedir + "\\games", files, &subs);
    std::sort(subs.begin(), subs.end());
    auto lbl = [](const GProfile & p, int i) -> std::string {
        static const char L[4] = { 'A', 'B', 'C', 'D' };
        std::string k = std::string("label_") + (char)('a' + i); auto it = p.kv.find(k);
        if (it != p.kv.end()) return std::string(1, L[i]) + ": " + it->second;
        if (!p.disk[i]) return std::string(); std::string n = p.disk[i]; size_t d = n.find_last_of('.'); if (d != std::string::npos && n.size() - d <= 5) n = n.substr(0, d);
        if (n.size() > 34) {   /* kx-ars: keep the tail (arle-game vs rulue-data) */
            std::string t = n.substr(n.size() - 31); size_t b = 0;
            while (b < t.size() && ((unsigned char)t[b] & 0xC0) == 0x80) b++; t = t.substr(b);
            size_t sp = t.find(' '); if (sp != std::string::npos && sp < 10) t = t.substr(sp + 1);
            n = "..." + t;
        }
        return std::string(1, L[i]) + ": " + n;
    };
    for (auto & d : subs) {
        if (gh_tidy_hidden(gApp.exedir + "\\games\\" + d)) continue;   /* kx-clean */
        GProfile p; if (!gh_profile(d, p)) continue;
        if (p.kv.count("dup_of")) continue;   /* 1b-3: 같은 이미지의 중복 작품은 목록에서만 숨긴다(폴더는 옮기지 않음) */
        E e; e.order = 100; auto o = p.kv.find("order"); if (o != p.kv.end()) e.order = atoi(o->second.c_str());
        e.g.id = d; e.g.title = p.title; e.g.src = p.src; e.g.ov = p.ov;
        e.g.title_db = p.title;   /* t49a (#809): 사용자 지정 표시 제목 — user.ini [game] title_user= (비면 DB·game.ini 제목) */
        { prof::Ini u_; std::string ud = gApp.exedir + "\\games\\" + d; if (prof::ini_load(ud + "/user.ini", u_) && u_.count("game") && u_["game"].count("title_user")) e.g.title_user = u_["game"]["title_user"]; if (!e.g.title_user.empty()) e.g.title = e.g.title_user; }
        {   /* t39: 언어·short·변형·글꼴 묶음 */
            ulist::fill(e.g, gApp.exedir + "\\games\\" + d, p.imgdir, p.kv, p.title); e.g.need_install = p.install_required;
            if (!e.g.title_user.empty()) e.g.short_t.clear();   /* t49a: 바꾼 제목은 자동 줄임을 새로 만든다 */
            { auto vl = p.kv.find("variant_label"); if (vl != p.kv.end() && !vl->second.empty()) gApp.vlabel[d] = vl->second; }
            std::string fn = p.font; { size_t sl = fn.find_last_of("\\/"); if (sl != std::string::npos) fn = fn.substr(sl + 1); if (fn.size() > 4 && (fn.compare(fn.size() - 4, 4, ".bmp") == 0 || fn.compare(fn.size() - 4, 4, ".BMP") == 0)) fn.resize(fn.size() - 4); }
            if (fn.empty()) { auto fk = p.kv.find("font_kind"); e.g.fgroup = (fk != p.kv.end() && fk->second == "jp") ? "jis" : (e.g.lang == "jp" ? "jis" : "ks2350"); }
            else { e.g.fgroup = "custom"; e.g.gfont = fn; for (auto & f : gApp.ui_.fonts) if (f.ok && f.name == fn && p.font.find_first_of("\\/:.") == std::string::npos) e.g.fgroup = f.group; }
        }
        { auto kvs = [&](const char * k) -> std::string { auto it = p.kv.find(k); return it == p.kv.end() ? std::string() : it->second; };
          e.g.company = kvs("company"); e.g.year = kvs("year"); e.g.genre = kvs("genre"); e.g.patch = kvs("patch"); e.g.devices = kvs("devices");
          { libmeta::load(gApp.exedir + "/library_meta.tsv"); const libmeta::Row * lr = libmeta::find(d);   /* v0.6.8: 라이브러리 메타(없으면 건너뜀) */
            if (lr) { e.g.title_jp = lr->title_jp; for (int q = 0; q < 3; q++) e.g.intro[q] = lr->intro[q]; if (e.g.company.empty()) e.g.company = lr->publisher; if (e.g.year.empty()) e.g.year = lr->year; if (e.g.genre.empty()) e.g.genre = lr->genre; } }
          if (e.g.devices.empty()) { std::string dv; auto add = [&](const char * s) { if (!dv.empty()) dv += " · "; dv += s; }; std::string mo = kvs("mouse"), so = kvs("sound"), jo = kvs("joystick");
              if (so.find("86") != std::string::npos) add(phr_("86 보드")); if (so.find("rhythm") != std::string::npos) add(phr_("리듬 WAV"));
              if (mo == "required") { std::map<std::string, std::string> kk = p.kv; kk["__id"] = d; add(aw::mouse_released(kk) ? phr_("마우스 지원(시험)") : phr_("마우스 필수")); } else if (mo == "pointer") add(phr_("마우스 지원")); else if (mo == "optional") add(phr_("마우스 선택"));
              if (jo.find("opn") != std::string::npos) add(phr_("OPN 조이스틱")); e.g.devices = dv; }
          std::string demo = kvs("demo"); e.g.has_demo = !demo.empty();
          {   /* r017: 판정 등급 · 출처 · 실행 불가 사유(목록을 만들 때마다 계산 — 지원이 늘면 다시 등록하지 않아도 풀린다) */
              bool autoreg = kvs("auto") == "1", curated = p.src == "DB" || kvs("curated") == "1"; e.g.autoreg = autoreg;
              std::string gr = kvs("grade"); e.g.grade = gr.empty() ? '-' : gr[0]; e.g.judge = kvs("judge"); e.g.jshort = kvs("jshort"); e.g.metasrc = kvs("metasrc");
              e.g.orig = kvs("orig"); e.g.dbhit = kvs("dbhit"); e.g.stemc = kvs("stem_cand");   /* #L6 */
              {   /* r017 이전 문구(내부 용어)가 들어 있는 ini 는 쉬운 말로 바꿔 보여 준다 */
                  auto rep = [](std::string & s, const char * a, const char * b) { size_t k; while ((k = s.find(a)) != std::string::npos) s.replace(k, strlen(a), b); };
                  rep(e.g.jshort, "A 큐레이션 DB 일치", "확인됨 (DB 일치)"); rep(e.g.judge, "A 큐레이션 DB(해시 일치): ", "원판과 같은 파일 — "); rep(e.g.judge, "큐레이션 DB", "DB");
                  rep(e.g.jshort, "B 변형 · ", "추정 · "); rep(e.g.jshort, "C 후보 · ", "후보 · "); }
              if (gr.empty() && p.src == "DB") { e.g.grade = 'A'; e.g.dbhit = p.title; e.g.judge = phr_("원판과 같은 파일 — ") + p.title + phr_(" (DB 일치)"); e.g.jshort = phr_("확인됨 (DB 일치)"); e.g.metasrc = e.g.metasrc.empty() ? std::string("DB") : e.g.metasrc; }
              else if (gr.empty() && !autoreg) {   /* game.ini 로 직접 정한 작품: DB 일치 여부도 같이 보인다 */
                  if (!e.g.dbhit.empty()) { e.g.grade = 'A'; e.g.jshort = phr_("직접 지정 · DB 일치"); e.g.judge = phr_("game.ini 로 직접 지정 — 파일은 DB의 '") + e.g.dbhit + phr_("' 와 일치"); }
                  else { e.g.jshort = phr_("직접 지정"); e.g.judge = phr_("game.ini 로 직접 지정 (DB와 일치하는 파일 없음)"); } }
              {   /* #L5: 기동 상태 — 제목 키(DB 일치 이름 우선)와 디스크 수가 같은 gamedb_nk 항목이 정확히 하나일 때만 잇는다(못 찾으면 「미확인」). 등록 때 nk_gid 를 적어 두는 안은 이번 빌드에 없음 */
                  gh_nk_load_once();
                  if (gApp.nk_.loaded && !gApp.nk_.boot.empty()) {
                      std::string gid = kvs("nk_gid");   /* 1) game.ini 에 적어 둔 gamedb_nk 번호가 있으면 그것 */
                      if (gid.empty()) { const std::string sh = kvs("sha1s"); size_t pp = 0; while (pp < sh.size() && gid.empty()) { size_t q = sh.find(',', pp); if (q == std::string::npos) q = sh.size(); auto it = gApp.nk_.bysha.find(sh.substr(pp, q - pp)); if (it != gApp.nk_.bysha.end()) gid = gApp.nk_.g[(size_t)it->second.first].gid; pp = q + 1; } }   /* 2) 이미지 sha1 이 gamedb_nk 원판과 같으면(앞 디스크부터) 그 작품 */
                      if (gid.empty()) gid = kvs("boot_gid");   /* 2.5) 등록 때 원판 이름으로 자동 기입했거나 사람이 적은 boot_gid */
                      if (gid.empty()) {   /* 3) orig= → srcdir 끝 이름(원판 이름)으로 표를 찾아 후보가 정확히 1개일 때만. 0개·2개 이상·한글 제목 흐린 찾기는 하지 않음(추측 금지) → 「미확인」 */
                          for (int w2 = 0; w2 < 2; w2++) { const std::string nm = nk::src_base(kvs(w2 ? "srcdir" : "orig")); if (!nk::src_key_ok(nm)) continue; nk::by_name(gApp.nk_, nm, gid); break; } }
                      if (!gid.empty()) g_stem_skip_gid++; else if (g_stem.loaded && (!kvs("stem_done").empty() || !kvs("nk_gid").empty() || !kvs("variant_of").empty())) g_stem_skip_done++;   /* #P3-4 */
                      if (gid.empty() && g_stem.loaded && kvs("stem_done").empty() && kvs("nk_gid").empty() && kvs("variant_of").empty()) {   /* #L6: 이미 등록된 작품은 목록이 뜬 뒤 한 작품씩(gh_stem_step) 판정 — 여기서는 대기열에만 넣는다 */
                          static std::set<std::string> tried; if (tried.insert(d).second) { StemJob j; j.id = d; j.hd = p.disk[2] || p.disk[3];
                              auto addi = [&](const std::string & f) { if (f.empty()) return; bool ab = (f.size() > 1 && f[1] == ':') || f[0] == '\\' || f[0] == '/'; j.paths.push_back(ab ? f : aw::join(p.imgdir, f)); };
                              for (int i = 0; i < 4; i++) if (p.disk[i]) addi(p.disk[i]); for (auto & s : p.swap) addi(s); g_stemq.push_back(j); } }
                      if (!gid.empty()) { e.g.nkgid = gid; auto bi = gApp.nk_.boot.find(gid); if (bi != gApp.nk_.boot.end()) { e.g.bootst = bi->second.st; e.g.bootconf = bi->second.conf; } }
                  } }
              prof::Profile q; q.install_required = p.install_required; q.reason = p.reason; q.disk.assign(4, std::string()); for (int i = 0; i < 4; i++) if (p.disk[i]) q.disk[i] = p.disk[i];
              std::map<std::string, std::string> kv = p.kv; if (curated) kv["curated"] = "1"; kv["__id"] = d; if (!autoreg) kv.erase("mouse");   /* 직접 정한 작품은 마우스 때문에 막지 않는다 */
              e.g.dosnote = aw::dos_note(q, kv); e.g.block = aw::block_reason(q, kv); if (e.g.block.empty() && !kvs("block").empty() && kvs("block").find("마우스 필수") == std::string::npos) e.g.block = kvs("block");   /* t48a (#615): 마우스 필수 문구는 막는 사유가 아님 */
              if (autoreg) { std::string sd, idr; { prof::Ini gi_; if (prof::ini_load(gApp.exedir + "\\games\\" + d + "/game.ini", gi_) && gi_.count("game")) { auto & G_ = gi_["game"]; if (G_.count("srcdir")) sd = G_["srcdir"]; if (G_.count("imgdir")) idr = G_["imgdir"]; } }   /* t724: p.kv 값은 ANSI 로 바뀐 것이라 한글 경로가 깨진다 — game.ini 의 UTF-8 원문으로 본다(목록 정리와 같은 읽기) */
                  if (gh_path_gone(sd, false) || gh_path_gone(idr, true)) { e.g.nosrc = true; e.g.block = phr_("원본 없음 — 작품 추가 > 목록 정리에서 다시 찾기 / 목록에서 빼기"); } }   /* #144-2: 원본이 없는 작품은 실행하지 않는다 */
              gh_log("LISTBLK id=%s src=%s auto=%d curated=%d mouse=%s entry_req='%s' block='%s' boot=%s/%s", d.c_str(), p.src.c_str(), autoreg ? 1 : 0, curated ? 1 : 0, kvs("mouse").c_str(), prof::g_entry_req.c_str(), e.g.block.c_str(), kvs("__val.boot").c_str(), kvs("__src.boot").c_str()); }
          auto flb = [&](const std::string & f) -> std::string {
              if (p.disk[0] && f == p.disk[0] && !kvs("label_a").empty()) return kvs("label_a"); if (p.disk[1] && f == p.disk[1] && !kvs("label_b").empty()) return kvs("label_b");
              if (!demo.empty() && f == demo) return kvs("label_demo").empty() ? std::string(phr_("오프닝(데모) 디스크")) : kvs("label_demo");
              std::string n = f; if (aw::is_hex40(n)) { std::vector<std::string> ord; for (int q = 0; q < 2; q++) if (p.disk[q] && *p.disk[q]) ord.push_back(p.disk[q]); for (auto & s : p.swap) ord.push_back(s); return gh_fd_real(p, n, ord); } size_t dd = n.find_last_of('.'); if (dd != std::string::npos && n.size() - dd <= 5) n = n.substr(0, dd); return n; };
          std::vector<std::string> files; auto addf = [&](const char * f) { if (!f || !*f) return; for (auto & x : files) if (x == f) return; files.push_back(f); };
          for (int i = 0; i < 2; i++) addf(p.disk[i]); for (auto & s : p.swap) addf(s.c_str()); addf(demo.c_str());
          for (auto & f : files) { e.g.fds.push_back(flb(f)); }
          gApp.fdfiles[d] = files;
          for (int i = 0; i < 2; i++) if (p.disk[i]) e.g.cur_fd[i] = flb(p.disk[i]);
          int nfd = 0; for (auto & f : files) if (f != demo) nfd++; bool hd = p.disk[2] || p.disk[3];
          e.g.disknote = hd ? (nfd ? "HDD + FD " + std::to_string(nfd) + phr_("장") : std::string("HDD")) : (nfd ? "FD " + std::to_string(nfd) + phr_("장") : std::string("-"));
          if (!demo.empty()) e.g.disknote += phr_(" + 오프닝");
          { std::string mo = kvs("more"); if (!mo.empty()) { e.g.more = mo; size_t k; while ((k = mo.find(" | ")) != std::string::npos) mo.replace(k, 3, " / "); e.g.disknote += phr_("  함께 있는 이미지: ") + mo; } } }
        std::vector<std::string> ls; for (int i = 0; i < 4; i++) { std::string l = lbl(p, i); if (!l.empty()) ls.push_back(l); }
        if (ls.size() > 0) e.g.disk[0] = ls[0]; if (ls.size() > 1) e.g.disk[1] = ls[1];
        e.g.boot_note = p.install_required ? std::string(phr_("하드 설치 전용 (설치 단계 필요)")) : std::string(phr_("부팅  ")) + p.bootletter + phr_(" 드라이브");
        if (p.install_required && ls.empty()) e.g.disk[0] = phr_("부팅 가능한 디스크 없음");
        es.push_back(e);
    }
    std::stable_sort(es.begin(), es.end(), [](const E & a, const E & b) { return a.order < b.order; });
    for (auto & e : es) gApp.ui_.games.push_back(e.g);
    ulist::load_all(gApp.ui_.games, &gApp.vlabel);
    {   /* 빌드 3 (#547): 분류 집계 로그 + 업그레이드 뒤 첫 실행 한 번 안내(파일 이동 없음) */
        int nko = 0, njp = 0, nun = 0; for (auto & g_ : gApp.ui_.games) { if (!g_.variant_of.empty() || g_.id.empty()) continue; if (g_.lang == "ko") nko++; else if (g_.lang == "jp") njp++; else nun++; }
        gh_log("CLASS ko=%d jp=%d un=%d", nko, njp, nun);
        const std::string mk_ = gApp.exedir + "\\unclass_notice.txt";
        if (nun > 0 && !plat::exists(mk_)) { FILE * nf_ = fopen(mk_.c_str(), "wb"); if (nf_) { fputs("1\n", nf_); fclose(nf_); }
            gApp.note = std::string(phr_("분류가 확실하지 않은 작품")) + " " + std::to_string(nun) + " " + phr_("(미분류 탭으로 옮김)"); gApp.note_until = SDL_GetTicks() + 8000u; }
    }
    if (gApp.ui_.games.empty()) {
        ui::UiGame g; g.id = ""; g.title = phr_("작품이 없습니다"); g.disk[0] = phr_("games 폴더에 작품 폴더를 넣어 주세요"); g.boot_note = ""; gApp.ui_.games.push_back(g);
        if (!gApp.uitest_dir.empty()) {   /* 캡처 시험용 표본 */
            gApp.ui_.games.clear(); const char * T[4][2] = { { "hukyou", phr_("환세풍광전") }, { "kaitou", phr_("환세쾌도전") }, { "kitan", phr_("환세희담") }, { "torimono", phr_("환세포물장") } };
            for (int i = 0; i < 4; i++) { ui::UiGame x; x.id = T[i][0]; x.title = T[i][1]; x.disk[0] = std::string("A: ") + T[i][1] + phr_(" 디스크"); x.boot_note = phr_("부팅  A 드라이브"); gApp.ui_.games.push_back(x); }
        }
    }
    gApp.ui_.build = std::string(KH_VER_DISP);   /* E1R: menu shows tag-free */
    gh_log("version: %s", KH_VERSION);   /* E1R: full tag stays in log */   /* t52a (#53): 내부 빌드명은 로그에만 */
    gh_log("games: %d", (int)gApp.ui_.games.size());
}
/* ------------------------------------------------------------------ r017: 작품 추가 · 게임 폴더 자동 등록 */
#include "platform/plat_dialog.h"   /* S9e-N 3: OS headers and dialogs live in platform */
static void gh_nk_load_once(void) {
    static bool done = false; if (done) return; done = true;
    std::string p = gApp.nkdb_path.empty() ? gApp.exedir + "\\gamedb_nk.tsv" : gApp.nkdb_path;
    if (nk::load(p, gApp.nk_)) { nk::load_sig(gApp.exedir + "\\gamedb_nk_sig.tsv", gApp.nk_); gh_log("gamedb_nk: %d games, %d sha1 (nk=%s tosec=%s)", (int)gApp.nk_.g.size(), (int)gApp.nk_.bysha.size(), gApp.nk_.ver_nk.c_str(), gApp.nk_.ver_tosec.c_str()); }
    else gh_log("gamedb_nk: none (%s)", p.c_str());
    { int nb = nk::load_boot(gApp.exedir + "\\gamedb_nk_boot.tsv", gApp.nk_); gh_log("gamedb_nk_boot: %d rows", nb); }   /* #L5: 없으면 0 — 앱은 지금과 똑같이 동작 */
    { bool ok = stem::load(gApp.exedir + "\\gamedb_nk_stem.tsv", g_stem); gh_log("gamedb_nk_stem: %s (%d games, %d stems)", ok ? "ok" : "none", (int)g_stem.g.size(), (int)g_stem.stems.size()); }   /* #L6 */
    gApp.shacache.load(gApp.exedir + "\\sha_cache.tsv");
}
static plat::native_win gh_hwnd(void) { return gApp.win ? plat::window_native(gApp.win) : NULL; }

/* ---- t52a (#54/#55): 제목표시줄이 윈도우 앱 테마를 따른다 ---------------------------------------------
 * HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\AppsUseLightTheme == 1 이면 밝게, 그 밖(값 없음·0·읽기 실패)은 어둡게.
 * DwmSetWindowAttribute 20(DWMWA_USE_IMMERSIVE_DARK_MODE) 을 먼저, 실패하면 19(구 Win10 1809~1903 값). 둘 다 실패하거나 DWM 이 없으면(Win7/8, Win10 1809 미만)
 * 아무것도 하지 않아 기본 제목표시줄이 그대로 남는다. 실행 중 앱 테마가 바뀌면(WM_SETTINGCHANGE "ImmersiveColorSet") 다시 적용.
 * dwmapi.dll 은 LoadLibrary 로 부르므로 링크 목록은 늘지 않는다. */
static bool gh_apps_light(void) {
    return plat::apps_use_light_theme();
}
static void gh_titlebar_apply(const char * why, bool force) {
    try {
        plat::native_win hw = gh_hwnd(); if (!hw) return;
        if (gApp.tb_last == -3 && !force) return;
        int want = gh_apps_light() ? 0 : 1;
        if (!force && want == gApp.tb_last) return;
        unsigned at = 0; long r = 0; int dr = plat::dwm_dark_titlebar(hw, want != 0, &at, &r);
        if (dr == 1) { if (gApp.tb_last != -3) gh_log("TITLEBAR %s: DwmSetWindowAttribute 없음 -> 기본 제목표시줄 유지", why); gApp.tb_last = -3; return; }
        if (dr == 2) { if (gApp.tb_last != -3) gh_log("TITLEBAR %s: 실패 hr=0x%08lX -> 기본 제목표시줄 유지 (apps_light=%d)", why, (unsigned long)r, 1 - want); gApp.tb_last = -3; return; }
        bool changed = (want != gApp.tb_last); gApp.tb_last = want;
        if (changed) {
            gh_log("TITLEBAR %s: apps_light=%d -> %s 제목표시줄 (attr=%u)", why, 1 - want, want ? "어두운" : "밝은", (unsigned)at);
            if (gApp.win && !(SDL_GetWindowFlags(gApp.win) & SDL_WINDOW_FULLSCREEN_DESKTOP)) plat::redraw_frame(hw);   /* 바로 다시 그리게 */
        }
    } catch (...) { }
}
static void gh_titlebar_event(const SDL_Event * e) {
    try {
        if (plat::is_theme_change_event(e)) {
            gh_titlebar_apply("settingchange", true);
        } else if (e->type == SDL_WINDOWEVENT && gApp.win && (e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED || e->window.event == SDL_WINDOWEVENT_RESTORED || e->window.event == SDL_WINDOWEVENT_SHOWN)) {
            unsigned now = SDL_GetTicks(); if (now - gApp.tb_tick > 400u) { gApp.tb_tick = now; gh_titlebar_apply("window", true); }   /* 전체화면 <-> 창 전환 뒤에도 유지 */
        }
    } catch (...) { }
}

#if defined(__ANDROID__)
static bool gh_file_explorer(const char * title_u8, std::vector<std::string> & files);   /* app/folder_pick.h (#B2 파일 모드) */
#endif
static bool gh_pick_files(std::vector<std::string> & out) {   /* 시스템(ANSI) 경로로 돌려준다 */
#if defined(__ANDROID__)
    return gh_file_explorer(phr_("추가할 디스크 이미지를 고르세요 (여러 개 가능)"), out);   /* #B2: 앱 안 탐색기 파일 모드 (안드로이드는 시스템 파일 창 없음) */
#endif
    std::string t = aw::u8_to_acp(phr_("추가할 디스크 이미지를 고르세요 (여러 개 가능)"));
    std::string f1 = aw::u8_to_acp(phr_("디스크 이미지")), f2 = aw::u8_to_acp(phr_("모든 파일"));
    std::string flt = f1 + std::string(1, '\0') + "*.fdi;*.hdi;*.nhd;*.d88;*.d77;*.88d;*.98d;*.nfd;*.hdm;*.dup;*.xdf;*.dsk;*.fim;*.flp;*.img;*.ima;*.thd;*.hdd;*.hdn;*.dim;*.tfd;*.fdd;*.bin;*.zip" + std::string(1, '\0') + f2 + std::string(1, '\0') + "*.*" + std::string(2, '\0');
    std::vector<std::string> parts; if (!plat::pick_files_a(gh_hwnd(), t, flt, parts)) return false;
    if (parts.size() == 1) out.push_back(parts[0]); else { for (size_t i = 1; i < parts.size(); i++) out.push_back(aw::join(parts[0], parts[i])); }
    return !out.empty();
}
#if defined(__ANDROID__)
static bool gh_folder_explorer(const char * title_u8, std::string & out);   /* app/folder_pick.h */
#endif
static bool gh_pick_folder(const char * title_u8, std::string & out) {
#if defined(__ANDROID__)
    return gh_folder_explorer(title_u8, out);
#else
    std::string t = aw::u8_to_acp(title_u8);
    return plat::pick_folder_a(gh_hwnd(), t, out);
#endif
}
static aw::Ctx gh_aw_ctx(void) {
    gh_db_load_once(); gh_nk_load_once(); aw::Ctx c; c.games_dir = gApp.exedir + "\\games"; c.on_dir = gh_res_on_work; c.db = &gApp.db; c.nk = gApp.nk_.loaded ? &gApp.nk_ : NULL; c.stab = g_stem.loaded ? &g_stem : NULL; c.cache = &gApp.shacache; { std::vector<std::string> ml; int mv = aw::merge_dups(c, &ml); for (auto & l : ml) gh_log("%s", l.c_str()); if (mv) gh_log("중복 합침 %d건", mv); } aw::index_registered(c); return c;
}
static void gh_reload_games(void) {   /* 목록을 다시 만들고 보던 작품을 id 로 되찾는다 */
    std::string keep = (gApp.cur_game >= 0 && gApp.cur_game < (int)gApp.ui_.games.size()) ? gApp.ui_.games[gApp.cur_game].id : std::string();
    const int lang_keep_ = gApp.ui_.lang;   /* #638 */
    ui::settings_save(gApp.ui_, gApp.cfgpath);
    gApp.fdfiles.clear();
    gh_ui_init_games(); ui::settings_load(gApp.ui_, gApp.cfgpath); gApp.ui_.lang = lang_keep_;   { FILE * zf_ = fopen(gApp.cfgpath.c_str(), "rb"); int ex_ = zf_ != NULL; if (zf_) fclose(zf_); int cg_ = gApp.cur_game; bool ht_ = cg_ >= 0 && cg_ < (int)ui::Ui::MAXG && gApp.ui_.has_t[cg_]; gh_log("SETDIAG %s path=%s file=%d g[scan=%d mask=%d bleed=%d glow=%d curve=%d ratio=%d] cur_game=%d has_t=%d games=%d", "after-list-reload", gApp.cfgpath.c_str(), ex_, gApp.ui_.g.scan, gApp.ui_.g.mask, gApp.ui_.g.bleed, gApp.ui_.g.glow, gApp.ui_.g.curve, gApp.ui_.g.ratio, cg_, (int)ht_, (int)gApp.ui_.games.size()); }
    for (size_t i = 0; i < gApp.ui_.games.size(); i++) if (gApp.ui_.games[i].id == keep) { gApp.cur_game = (int)i; gApp.ui_.cur = (int)i; gApp.ui_.pick = (int)i; }
    if (!gApp.headless) for (size_t i = 0; i < gApp.ui_.games.size(); i++) gh_refresh_game_slots((int)i);
}
static aw::Summary gh_add_works(const std::vector<aw::Work> & ws, const char * what, bool user = false) {
    aw::Ctx c = gh_aw_ctx(); c.user_add = user; aw::Summary S; aw::run_works(c, ws, S); gApp.shacache.save();
    if (c.unhid > 0) { gh_log("ADDWORK unhid %d (목록에서 뺐던 작품을 다시 추가)", c.unhid); gh_reload_games(); }   /* 사용자가 같은 원본을 다시 추가하면 숨김을 푼다 */
    char h[200]; snprintf(h, sizeof h, "%s: 추가%d 있음%d 건너뜀%d 불가%d", what, S.added, S.exists, S.skipped, S.blocked);
    gApp.ui_.add_log.clear(); gApp.ui_.add_log.push_back(what); { char sb[200]; snprintf(sb, sizeof sb, phr_("추가%d 있음%d 건너뜀%d 불가%d"), S.added, S.exists, S.skipped, S.blocked); gApp.ui_.add_log.push_back(sb); }   /* #B4: 낱말 앞 공백을 키로 쓰면 문구표 읽기에서 잘려 글자가 붙었다 → 공백이 안쪽에 있는 한 줄 틀 */ for (auto & l : S.lines) gApp.ui_.add_log.push_back(l);
    gh_log("ADDWORK %s", h); for (auto & l : S.lines) gh_log("ADDWORK   %s", l.c_str());
    gh_log("ADDWORK sha1 cache hits=%u miss=%u", gApp.shacache.hits, gApp.shacache.miss);
    return S;
}
/* 게임 폴더: 켤 때마다 훑어 새 작품만 자동 등록 */
static int gh_scan_gamefolder(const char * why) {
    kha_lib_init();   /* t137 */
    if (gApp.ui_.gamefolder.empty()) return 0;
    std::string dir = aw::u8_to_acp(gApp.ui_.gamefolder); if (!aw::dir_exists(dir)) { gh_log("GAMEFOLDER %s: 폴더 없음 (%s)", why, dir.c_str()); if (!gApp.ui_.add_log.empty()) gApp.ui_.add_log.clear(); gApp.ui_.add_log.push_back(phr_("게임 폴더를 찾을 수 없습니다")); return 0; }
    std::vector<aw::Work> ws; aw::collect(dir, 3, ws); aw::Summary S = gh_add_works(ws, why);
    gh_log("GAMEFOLDER %s: %d 후보, 추가 %d", why, (int)ws.size(), S.added);
    S.added += kha_scan_extra(why);   /* t137: 등록 폴더 */
    if (S.added > 0) gh_reload_games(); return S.added;
}
#if defined(__ANDROID__)
static const char * fpt(int i);
static void gh_android_savedir(void);
#endif
static void gh_add_action(int arg) {
    ui::Ui & U = gApp.ui_;
    if (arg == 0) { std::vector<std::string> fs; if (!gh_pick_files(fs)) return; std::vector<aw::Work> ws; aw::from_files(fs, ws); aw::Summary S = gh_add_works(ws, phr_("파일 추가"), true); if (S.added) { gh_reload_games(); U.add_new = S.added_ids; U.add_new_why = S.added_why; U.open_lang(U.add_new, U.add_new_why, ui::S_ADD); } }
    else if (arg == 1) { std::string d; if (!gh_pick_folder(phr_("추가할 작품 폴더를 고르세요"), d)) return; kha_lib_add(d); std::vector<aw::Work> ws; aw::collect(d, 3, ws); aw::Summary S = gh_add_works(ws, phr_("폴더 추가"), true); if (S.added) { gh_reload_games(); U.add_new = S.added_ids; U.add_new_why = S.added_why; U.open_lang(U.add_new, U.add_new_why, ui::S_ADD); } }
    else if (arg == 2) { std::string d; if (!gh_pick_folder(phr_("「게임 폴더」를 고르세요 — 켤 때마다 새 작품을 찾아 등록합니다"), d)) return; U.gamefolder = aw::acp_to_u8(d); ui::settings_save(U, gApp.cfgpath); gh_scan_gamefolder(phr_("게임 폴더 지정"));
#if defined(__ANDROID__)
    gh_android_savedir();
    if (U.games.size() == 1 && U.games[0].id.empty()) { U.add_log = { fpt(14), fpt(15) }; U.scr = ui::S_ADD; }   /* t55e: empty folder notice */
#endif
    }
    else if (arg == 3) { gh_scan_gamefolder(phr_("다시 검색")); if (!U.add_log.empty() && U.gamefolder.empty()) U.add_log.clear(); }
    else if (arg == 4) { kha_lib_clear(); U.gamefolder.clear(); ui::settings_save(U, gApp.cfgpath); U.add_log.clear(); U.add_log.push_back(phr_("게임 폴더를 해제했습니다 (등록된 작품은 그대로입니다)")); }
}
/* t44a (#494): 사용자 「언어」 지정 — user.ini [game] lang=ko|jp (자동 = 키 삭제). 저장 후 목록을 다시 만들고 보던 작품·항목 화면 대상을 id 로 되찾는다 */
/* #L6: 원판 후보 고르기 — 고른 후보의 번호를 game.ini 대신 user.ini [game] boot_gid= 로 저장(사람이 정한 값: 자동 판정보다 우선). 판 구분(한글판·원판)은 건드리지 않는다. */
static void gh_stempick(int gi, int cand) {
    ui::Ui & U = gApp.ui_; if (gi < 0 || gi >= (int)U.games.size()) return;
    std::vector<ui::StemC> cs = ui::stem_cands_of(U.games[gi].stemc); if (cand < 0 || cand >= (int)cs.size() || cs[cand].gid.empty()) return;
    std::string id = U.games[gi].id, eid = (U.entg >= 0 && U.entg < (int)U.games.size()) ? U.games[U.entg].id : std::string();
    bool ok = prof::set_game_key(U.games[gi].dir, "boot_gid", cs[cand].gid); if (ok) prof::set_game_key(U.games[gi].dir, "boot_how", "pick");
    gh_log("STEMPICK %s boot_gid=%s ok=%d", id.c_str(), cs[cand].gid.c_str(), ok ? 1 : 0);
    gh_reload_games();
    if (!eid.empty()) for (size_t i = 0; i < U.games.size(); i++) if (U.games[i].id == eid) U.entg = (int)i;
    if (!ok) U.msg = phr_("원판을 저장하지 못했습니다.");
}
#include "app/tidy_host.h"   /* kx-clean: 목록 정리(S_TIDY) host 쪽 */
static void gh_setlang(int gi, int mode) {
    ui::Ui & U = gApp.ui_; if (gi < 0 || gi >= (int)U.games.size()) return;
    std::string id = U.games[gi].id, eid = (U.entg >= 0 && U.entg < (int)U.games.size()) ? U.games[U.entg].id : std::string();
    const char * v = mode == 1 ? "ko" : mode == 2 ? "jp" : "";
    bool ok = prof::set_game_key(U.games[gi].dir, "lang", v);
    gh_log("SETLANG %s lang=%s ok=%d", id.c_str(), *v ? v : "(auto)", ok ? 1 : 0);
    gh_reload_games();
    if (!eid.empty()) for (size_t i = 0; i < U.games.size(); i++) if (U.games[i].id == eid) U.entg = (int)i;
    if (!ok) U.msg = phr_("언어를 저장하지 못했습니다.");
}
/* t49a (#809): 표시 제목 저장 — user.ini [game] title_user= (빈 값 = 키 삭제 = DB 제목 복귀). 원본 DB·game.ini 무변경 */
static void gh_settitle(int gi, const std::string & t) {
    ui::Ui & U = gApp.ui_; if (gi < 0 || gi >= (int)U.games.size()) return;
    std::string id = U.games[gi].id, eid = (U.entg >= 0 && U.entg < (int)U.games.size()) ? U.games[U.entg].id : std::string();
    bool ok = prof::set_game_key(U.games[gi].dir, "title_user", t);
    gh_log("SETTITLE %s title_user=%s ok=%d", id.c_str(), t.empty() ? "(없음)" : t.c_str(), ok ? 1 : 0);
    gh_reload_games();
    if (!eid.empty()) for (size_t i = 0; i < U.games.size(); i++) if (U.games[i].id == eid) U.entg = (int)i;
    if (!ok) U.msg = phr_("표시 제목을 저장하지 못했습니다.");
    else if (!t.empty()) U.msg = phr_("표시 제목을 바꿨습니다. 원본 DB는 그대로입니다.");
}
/* --addwork PATH [--dry] [--report DIR] : 파일/폴더를 등록(또는 시험 판정)하고 판정표(match_table.tsv)를 남긴다 — 헤드리스 시험용 */
static void gh_cli_addwork(void) {
    std::vector<aw::Work> ws; std::string p = gApp.addwork_cli;
    if (aw::dir_exists(p)) aw::collect(p, 3, ws); else { std::vector<std::string> v; v.push_back(p); aw::from_files(v, ws); }
    aw::Ctx c = gh_aw_ctx(); aw::Summary S; std::string tsv = phr_("작업\t파일(크기)\t등급\t판정(짧게)\t판정(전체)\tid\t제목\t제작사\t연도\t출처\t실행불가\t비고\n");
    for (auto & w : ws) {
        aw::Result r = aw::register_work(c, w, gApp.aw_dry);
        if (r.st == aw::Result::ADDED) { S.added++; if (!r.block.empty()) S.blocked++; } else if (r.st == aw::Result::EXISTS) S.exists++; else S.skipped++;
        std::string fl; for (auto & f : w.files) { uint64_t sz = 0, mt = 0; aw::file_stat(aw::join(w.dir, f), sz, mt); if (!fl.empty()) fl += " | "; fl += aw::acp_to_u8(f) + " (" + std::to_string((unsigned long long)sz) + ")"; }
        auto cl = [](std::string s) { for (auto & ch : s) if (ch == '\t' || ch == '\n' || ch == '\r') ch = ' '; return s; };
        tsv += cl(aw::pretty(w.name)) + "\t" + cl(fl) + "\t" + (r.grade.empty() ? std::string("-") : r.grade) + "\t" + cl(r.jshort) + "\t" + cl(r.judge) + "\t" + r.id + "\t" + cl(r.title) + "\t" + cl(r.maker) + "\t" + r.year + "\t" + cl(r.metasrc) + "\t" + cl(r.block) + "\t" + cl(r.note) + "\n";
        gh_log("ADDWORK%s %s | %s | %s | %s", gApp.aw_dry ? "(dry)" : "", aw::pretty(w.name).c_str(), r.grade.c_str(), r.judge.c_str(), r.note.c_str());
    }
    gh_log("ADDWORK summary: works=%d added=%d exists=%d skipped=%d blocked=%d dry=%d", (int)ws.size(), S.added, S.exists, S.skipped, S.blocked, (int)gApp.aw_dry);
    if (!gApp.report_dir.empty()) { plat::mkdir(gApp.report_dir.c_str()); FILE * f = fopen((gApp.report_dir + "\\match_table.tsv").c_str(), "wb"); if (f) { fwrite(tsv.data(), 1, tsv.size(), f); fclose(f); } }
    gApp.shacache.save(); if (!gApp.aw_dry && S.added) gh_reload_games();
}
/* --batest : 「배속 중 소리」 DSP 단독 시험(소리 장치 없음). 440 Hz 사인을 x 배 속도로 먹이고 출력의 음높이·크기·끊김을 잰다.
 * 기대: 빠르게(1) → 440*x Hz · 음높이 유지(2) → 440 Hz */
static void gh_batest(void) {
    const double SR = 44100.0; const int NEED = 441, CALLS = 700; int xs[5] = { 2, 3, 4, 6, 8 }; int bad = 0;
    for (int xi = 0; xi < 5; xi++) for (int mode = 1; mode <= 2; mode++) {
        int x = xs[xi]; ba::Eng * e = new ba::Eng; static int32_t l[8192], r[8192]; std::vector<float> out; out.reserve((size_t)NEED * CALLS); long long n = 0; int under = 0;
        for (int c = 0; c < CALLS; c++) {
            int fed = NEED * x; for (int i = 0; i < fed; i++, n++) { int32_t s = (int32_t)(8000.0 * sin(2.0 * 3.14159265358979323846 * 440.0 * (double)n / SR)); e->feed1(s, s); }
            e->note(fed, NEED, (double)x); int made = e->render(mode, (double)x, NEED, l, r); if (made < NEED) under++;
            for (int i = 0; i < made; i++) out.push_back((float)l[i]);
            for (int i = made; i < NEED; i++) out.push_back(out.empty() ? 0.0f : out.back());
        }
        size_t a = (size_t)SR; size_t cnt = 0; double ss = 0; for (size_t i = a + 1; i < out.size(); i++) { if (out[i - 1] <= 0 && out[i] > 0) cnt++; ss += (double)out[i] * out[i]; }
        double secs = (double)(out.size() - a) / SR, hz = cnt / secs, rms = sqrt(ss / (double)(out.size() - a)); double want = mode == 1 ? 440.0 * x : 440.0;
        bool ok = fabs(hz - want) / want < 0.04 && under <= CALLS / 50 && rms > 1500;
        if (!ok) bad++;
        gh_log("BATEST x=%d mode=%d(%s) freq=%.1fHz expect=%.0fHz rms=%.0f under=%d/%d drops=%u %s", x, mode, mode == 1 ? "빠르게" : "음높이 유지", hz, want, rms, under, CALLS, e->drops, ok ? "OK" : "FAIL");
        delete e;
    }
    gh_log("BATEST done: %s", bad ? "FAIL" : "ALL OK");
}
/* ---- 입력 → UI 키 */
/* t49a (#812): 메뉴 키 매핑 표 — gh_ev2uk 가 읽고 조작 설명(helpline) 번역도 같은 표에서 이름을 만든다(키 이름 한 곳). 설정 노출 없음 */
struct GhKeyRow { ui::UKey uk; const char * sym; const char * name; int n; SDL_Keycode k[4]; };
static const GhKeyRow GH_KEYTAB[] = {
    { ui::UK_UP, "↑", "↑", 1, { SDLK_UP } }, { ui::UK_DOWN, "↓", "↓", 1, { SDLK_DOWN } }, { ui::UK_LEFT, "←", "←", 1, { SDLK_LEFT } }, { ui::UK_RIGHT, "→", "→", 1, { SDLK_RIGHT } },
    { ui::UK_OK, "○", "Enter", 3, { SDLK_RETURN, SDLK_KP_ENTER, SDLK_SPACE } },
    { ui::UK_CANCEL, "×", "Esc", 4, { SDLK_ESCAPE, SDLK_BACKSPACE, SDLK_F12, SDLK_AC_BACK } },
    { ui::UK_SQ, "□", "S", 1, { SDLK_s } }, { ui::UK_TRI, "△", "D", 1, { SDLK_d } }, { ui::UK_SELECT, "SEL", "Tab", 1, { SDLK_TAB } },
    { ui::UK_L, "L", "Q", 1, { SDLK_q } }, { ui::UK_R, "R", "E", 1, { SDLK_e } }, { ui::UK_PROF, "R3", "P", 1, { SDLK_p } },   /* v0.6.8: 작품 선택 화면 실행 방법 바꾸기 */
};
static ui::UKey gh_keytab_lookup(SDL_Keycode sym) { for (auto & r : GH_KEYTAB) for (int i = 0; i < r.n; i++) if (r.k[i] == sym) return r.uk; return ui::UK_NONE; }
static void gh_help_init(void) {   /* 같은 표에서 helpline 번역표를 만든다(첫 키 이름만; 방향키는 그대로) */
    ov::g_helpkeys.clear(); std::string ln, rn;
    for (auto & r : GH_KEYTAB) { if (r.uk == ui::UK_UP || r.uk == ui::UK_DOWN || r.uk == ui::UK_LEFT || r.uk == ui::UK_RIGHT) continue; if (r.uk == ui::UK_L) { ln = r.name; continue; } if (r.uk == ui::UK_R) { rn = r.name; continue; } ov::HelpKey hk; hk.sym = r.sym; hk.name = r.name; ov::g_helpkeys.push_back(hk); if (r.uk == ui::UK_SELECT) { ov::HelpKey h2; h2.sym = "SELECT"; h2.name = r.name; ov::g_helpkeys.push_back(h2); } }
    ov::HelpKey a; a.sym = "L R"; a.name = ln + " " + rn; ov::g_helpkeys.push_back(a); ov::HelpKey b; b.sym = "L/R"; b.name = ln + "/" + rn; ov::g_helpkeys.push_back(b);
}
#if defined(__ANDROID__)
/* t55a: 메뉴 터치 최소판. SDL 손가락 이벤트를 키 이벤트로 바꿔 밀어 넣어 기존 메뉴 루프(게임 중·시작 메뉴 둘 다)가 그대로 처리하게 한다.
 * 탭 = 그 줄로 이동 + 결정(Enter) · 4:3 틀 바깥 탭 = 취소(Esc) · 세로 끌기 = 위/아래 한 칸씩. 좌표는 gh_publish 의 소프트웨어 경로(논리 640x480 레터박스; 16:10 이면 400줄 가운데, 4:3 이면 세로 늘림)와 같은 식. */
static void gh_touch_key(SDL_Keycode k) { SDL_Event ke; SDL_zero(ke); ke.type = SDL_KEYDOWN; ke.key.keysym.sym = k; ke.key.state = SDL_PRESSED; SDL_PushEvent(&ke); ke.type = SDL_KEYUP; ke.key.state = SDL_RELEASED; SDL_PushEvent(&ke); }
static int gh_touch_fl(double v) { int i = (int)v; return (v < (double)i) ? i - 1 : i; }
static void gh_kb_warp(double & u, double & v);   /* kbd_event.h */
static bool gh_touch_pos(float fx, float fy, int & vx, int & vy) {   /* finger (window 0..1) to menu canvas (640x400); outside the frame gives out-of-range values */
    if (gApp.use_gl && gApp.win) {   /* t57a: GL output -- same rectangle the render thread draws into (gl_rect) */
        int dw = 0, dh = 0, x = 0, y = 0, rw = 0, rh = 0; SDL_GL_GetDrawableSize(gApp.win, &dw, &dh); if (dw <= 0 || dh <= 0) return false;
        gl_rect(dw, dh, gApp.pub_w > 0 ? gApp.pub_w : 640, gApp.pub_h > 0 ? gApp.pub_h : 400, gh_fit_eff(), x, y, rw, rh); if (rw <= 0 || rh <= 0) return false;
        double fu = ((double)fx * dw - x) / rw, fv = ((double)fy * dh - y) / rh;   /* kx-touch: 곡면이면 마우스 경로와 같은 warp */
        if ((int)gApp.shader == (int)SH_CRT && (int)gApp.fit != (int)FIT_INT && gApp.curve > 0.0f) gh_kb_warp(fu, fv);
        vx = gh_touch_fl(fu * 640.0); vy = gh_touch_fl(fv * 400.0); if (gh_menu_sq()) vx = gh_menu_ux(vx); return true;
    }
    int ow = 0, oh = 0; if (!gApp.ren || SDL_GetRendererOutputSize(gApp.ren, &ow, &oh) != 0 || ow <= 0 || oh <= 0) return false;
    double sx = ow / 640.0, sy = oh / 480.0, sc = sx < sy ? sx : sy, ox = (ow - 640.0 * sc) / 2.0, oy = (oh - 480.0 * sc) / 2.0;
    double lx = ((double)fx * ow - ox) / sc, ly = ((double)fy * oh - oy) / sc;
    vx = gh_touch_fl(lx); vy = (gh_fit_eff() == FIT_1610) ? gh_touch_fl(ly - 40.0) : gh_touch_fl(ly * 400.0 / 480.0); if (gh_menu_sq()) vx = gh_menu_ux(vx); return true;
}
static void gh_touch_feed(const SDL_Event & e) {
    pwr::mclk::note_touch();   /* E1R: touch input stays silent */
    static bool dn = false, mv = false, hz = false; static SDL_FingerID fid = 0; static float x0 = 0, y0 = 0; static int ay = 0, ax = 0;
    const int STEP = 22, SLOP = 14;
    if (e.type == SDL_FINGERDOWN) { if (dn) return; int vx, vy; if (!gh_touch_pos(e.tfinger.x, e.tfinger.y, vx, vy)) return; dn = true; mv = false; fid = e.tfinger.fingerId; x0 = e.tfinger.x; y0 = e.tfinger.y; ay = vy; return; }
    if (!dn || e.tfinger.fingerId != fid) return;
    int vx, vy; if (!gh_touch_pos(e.tfinger.x, e.tfinger.y, vx, vy)) { if (e.type == SDL_FINGERUP) dn = false; return; }
    if (e.type == SDL_FINGERMOTION) {
        if (!mv) { int sx, sy; gh_touch_pos(x0, y0, sx, sy); if (abs(vy - sy) < SLOP && abs(vx - sx) < SLOP) return; mv = true; ay = sy; ax = sx; hz = abs(vx - sx) > abs(vy - sy); }   /* kx-touch: 가로 끌기 = 왼쪽/오른쪽 */
        if (hz) { while (vx - ax >= STEP) { gh_touch_key(SDLK_RIGHT); ax += STEP; } while (ax - vx >= STEP) { gh_touch_key(SDLK_LEFT); ax -= STEP; } return; }
        while (vy - ay >= STEP) { gh_touch_key(SDLK_UP); ay += STEP; }
        while (ay - vy >= STEP) { gh_touch_key(SDLK_DOWN); ay -= STEP; }
        return; }
    if (e.type == SDL_FINGERUP) {
        dn = false; if (mv) { gh_log("TOUCH swipe end v=(%d,%d)", vx, vy); return; }
        const bool ba = gApp.ui_.scr != ui::S_MAIN;   /* kx-touch: ← 가 그려지는 화면 */
        if (vx < 0 || vx >= 640 || vy < 0 || vy >= 400) { if (ba) { gh_log("TOUCH tap v=(%d,%d) outside -> ignored", vx, vy); return; } gh_log("TOUCH tap v=(%d,%d) outside -> cancel", vx, vy); gh_touch_key(SDLK_ESCAPE); return; }
        int bx0 = 0, by0 = 0;
        if (ba && gApp.ui_.back_xy(bx0, by0) && vx >= bx0 && vx < bx0 + 48 && vy >= by0 && vy < by0 + 32) { gh_log("TOUCH tap v=(%d,%d) back arrow -> cancel", vx, vy); gh_touch_key(SDLK_ESCAPE); return; }
        ui::Ui & U = gApp.ui_;
        if (U.scr == ui::S_PICK && U.prof_w > 0 && vx >= U.prof_x && vx < U.prof_x + U.prof_w && vy >= U.prof_y && vy < U.prof_y + U.prof_h) { gh_log("TOUCH tap v=(%d,%d) 실행 줄 -> 다음 방법", vx, vy); gh_touch_key(SDLK_p); return; }   /* v0.6.8 */
        if ((U.scr == ui::S_START || U.scr == ui::S_QUIT || U.scr == ui::S_YN) && U.cbw > 0
            && (vx < U.cbx || vx >= U.cbx + U.cbw || vy < U.cby || vy >= U.cby + U.cbh)) {   /* kx-touch: 가벼운 팝업 바깥 탭 = 취소 */
            gh_log("TOUCH tap v=(%d,%d) outside popup -> cancel", vx, vy); gh_touch_key(SDLK_ESCAPE); return; }
        if (U.scr == ui::S_CONF || U.scr == ui::S_SCREEN) {   /* #638: tap on an option of a choice line = select that option (move to its row, then left/right) */
            for (const ov::ChHit & ch : ov::g_chit) {
                if (vx < ch.x || vx >= ch.x + ch.w || vy < ch.y || vy >= ch.y + ch.h) continue;
                const int cyc = ch.y + ch.h / 2; const ui::Ui::Hit * rh = NULL;
                for (const ui::Ui::Hit & t : U.hits) if (t.x + t.w <= 320 && cyc >= t.y && cyc < t.y + t.h) rh = &t;
                if (!rh) break;
                int d = rh->i - rh->sel, dv = ch.idx - ch.cur;
                gh_log("TOUCH tap v=(%d,%d) option %d/%d (cur %d) on row %d (sel %d) -> select", vx, vy, ch.idx, ch.n, ch.cur, rh->i, rh->sel);
                for (; d > 0; d--) gh_touch_key(SDLK_DOWN); for (; d < 0; d++) gh_touch_key(SDLK_UP);
                for (; dv > 0; dv--) gh_touch_key(SDLK_RIGHT); for (; dv < 0; dv++) gh_touch_key(SDLK_LEFT);
                return; } }
        for (int h = (int)U.hits.size() - 1; h >= 0; h--) {   /* rows drawn later (on top) first */
            const ui::Ui::Hit & t = U.hits[h];
            if (vx >= t.x && vx < t.x + t.w && vy >= t.y && vy < t.y + t.h) {
                int d = t.i - t.sel; gh_log("TOUCH tap v=(%d,%d) row %d (sel %d, move %d) -> ok", vx, vy, t.i, t.sel, d);
                for (; d > 0; d--) gh_touch_key(SDLK_DOWN); for (; d < 0; d++) gh_touch_key(SDLK_UP);
                ov::g_tap_half = (U.scr == ui::S_CONF || U.scr == ui::S_SCREEN) ? ((vx >= t.x + t.w / 2) ? 1 : -1) : 0; ov::g_tap_ms = ov::tap_now_ms();   /* kx-touch: 값 줄 절반 탭 */
                gh_touch_key(SDLK_RETURN); return; } }
        gh_log("TOUCH tap v=(%d,%d) no row (ignored)", vx, vy);
    }
}
#endif
/* v0.6.8: 패드 십자·왼스틱 누르고 있기 반복 (첫 반복 400ms 뒤, 이후 80ms). 메뉴 입력 루프가 gh_pad_rep() 을 매 바퀴 부른다 */
static ui::UKey g_rep_k = ui::UK_NONE; static uint32_t g_rep_next = 0;
static void gh_rep_set(ui::UKey k) { g_rep_k = k; g_rep_next = SDL_GetTicks() + 400u; }
static void gh_pad_rep(void) { if (g_rep_k == ui::UK_NONE) return; uint32_t n = SDL_GetTicks(); if (n < g_rep_next) return; g_rep_next = (n > g_rep_next + 160u ? n : g_rep_next) + 80u; SDL_Event ke; SDL_zero(ke); ke.type = SDL_KEYDOWN; ke.key.keysym.sym = g_rep_k == ui::UK_UP ? SDLK_UP : g_rep_k == ui::UK_DOWN ? SDLK_DOWN : g_rep_k == ui::UK_LEFT ? SDLK_LEFT : SDLK_RIGHT; ke.key.state = SDL_PRESSED; ke.key.repeat = 1; SDL_PushEvent(&ke); }   /* 키보드 반복과 같은 길로 흘려 보낸다(repeat=1 → 입력 장치 표시는 안 바뀜) */
static ui::UKey gh_ev2uk(const SDL_Event & e, bool * anykey) {
    if (e.type == SDL_CONTROLLERBUTTONUP) { int b = e.cbutton.button; if (b == SDL_CONTROLLER_BUTTON_DPAD_UP || b == SDL_CONTROLLER_BUTTON_DPAD_DOWN || b == SDL_CONTROLLER_BUTTON_DPAD_LEFT || b == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) g_rep_k = ui::UK_NONE; }
    if (kh_raw_event(e)) return ui::UK_NONE;   /* extra-buttons tab */
    if (kh_cap_event(e)) return ui::UK_NONE;   /* hotkey tab waiting for a combo */
    if (gApp.ui_.scr == ui::S_PAD) {   /* pad tab: pressing L3 R3 L2 R2 jumps the cursor to that button (L1 R1 stay tab keys) */
        int jb = -1;
        if (e.type == SDL_CONTROLLERBUTTONDOWN) { if (e.cbutton.button == SDL_CONTROLLER_BUTTON_LEFTSTICK) jb = PB_L3; else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSTICK) jb = PB_R3; }
        else if (e.type == SDL_CONTROLLERAXISMOTION && (e.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT || e.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)) {
            static bool jon[2]; const int q = e.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT ? 1 : 0;
            if (!jon[q] && e.caxis.value > 16000) { jon[q] = true; jb = q ? PB_R2 : PB_L2; } else if (jon[q] && e.caxis.value < 8000) jon[q] = false; }
        if (jb >= 0) { gApp.ui_.pad_jump(jb); return ui::UK_NONE; } }
    using namespace ui;
    if (anykey) *anykey = (e.type == SDL_KEYDOWN && !e.key.repeat) || e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_MOUSEBUTTONDOWN;
    { int nd = -1; if (e.type == SDL_KEYDOWN && !e.key.repeat) nd = 1; else if (e.type == SDL_CONTROLLERBUTTONDOWN) nd = 0; else if (e.type == SDL_CONTROLLERAXISMOTION && (e.caxis.value > 16000 || e.caxis.value < -16000)) nd = 0;   /* t49a (#812): 마지막 입력 장치(마우스 제외) */
      if (nd >= 0 && ov::g_helpdev != nd) { ov::g_helpdev = nd; if (gApp.helplog) gh_log("HELPDEV 입력 장치 -> %s (clock=game)", nd ? "키보드" : "패드"); } }
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.mod & KMOD_ALT) return UK_NONE;
        if (gApp.ui_.scr == ui::S_OSK && e.key.keysym.sym >= 0x20 && e.key.keysym.sym < 0x7f && (e.key.keysym.sym != SDLK_SPACE || gApp.ui_.osk_hg)) return UK_NONE;   /* t39: 글자 입력 화면에서는 글자 키가 패드 키로 겹치지 않게 (t48b: 한글 키보드에서는 공백도 글자 이벤트로만) */
        if (gApp.ui_.scr == ui::S_OSK && e.key.keysym.sym == SDLK_BACKSPACE) return UK_SQ;   /* t48b: 글자 입력 화면의 백스페이스 = 지움(예전에는 취소로 나가 버렸다) */
        if (gApp.ui_.scr == ui::S_OSK && gApp.ui_.osk_hg && (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER)) return UK_SELECT;   /* t48b: 한글 키보드에서 엔터 = 완료 */
        return gh_keytab_lookup(e.key.keysym.sym);   /* t49a: 표 한 곳(GH_KEYTAB) */
    }
    if (e.type == SDL_CONTROLLERBUTTONDOWN) {
        switch (e.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_START: return UK_OK;
        case SDL_CONTROLLER_BUTTON_B: return kh_western ? UK_CANCEL : UK_OK;   /* t48e (#651 4): 일본식 = 동쪽(B/○) 결정 · 서양식 = 남쪽(A/×) 결정 */
        case SDL_CONTROLLER_BUTTON_A: return kh_western ? UK_OK : UK_CANCEL;
        case SDL_CONTROLLER_BUTTON_X: return UK_SQ; case SDL_CONTROLLER_BUTTON_Y: return UK_TRI; case SDL_CONTROLLER_BUTTON_BACK: return UK_SELECT;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return UK_PROF; case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return UK_L; case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return UK_R;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: gh_rep_set(UK_UP); return UK_UP; case SDL_CONTROLLER_BUTTON_DPAD_DOWN: gh_rep_set(UK_DOWN); return UK_DOWN; case SDL_CONTROLLER_BUTTON_DPAD_LEFT: gh_rep_set(UK_LEFT); return UK_LEFT; case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: gh_rep_set(UK_RIGHT); return UK_RIGHT;
        default: return UK_NONE; }
    }
    if (e.type == SDL_CONTROLLERAXISMOTION && (e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX || e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)) {
        static bool st[4] = { false, false, false, false };   /* L R U D */
        int ax = e.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX ? 0 : 2; int v = e.caxis.value;
        bool & neg = st[ax], & pos = st[ax + 1];
        if (!neg && v < -16000) { neg = true; gh_rep_set(ax == 0 ? UK_LEFT : UK_UP); return ax == 0 ? UK_LEFT : UK_UP; } if (neg && v > -8000) { neg = false; g_rep_k = UK_NONE; }
        if (!pos && v > 16000) { pos = true; gh_rep_set(ax == 0 ? UK_RIGHT : UK_DOWN); return ax == 0 ? UK_RIGHT : UK_DOWN; } if (pos && v < 8000) { pos = false; g_rep_k = UK_NONE; }
    }
    return UK_NONE;
}
