#pragma once
/* android_ext.h -- t137 (지시 #490, 공지 482~486): 안드로이드 전용.
 * 라이브러리 기본 폴더 = 내부 저장소/Kyuhachi (games\ fonts\), 등록 폴더 목록, 패드 라벨 방식, 소리 버퍼, 앱 생명주기·장치 로그, 상태 불러오기 진단.
 * 설정은 <앱 폴더>\android.ini (padlayout= auto|retro|xbox · sndbuf= 0..3 · lib= 경로 여러 줄). settings.ini 키는 건드리지 않는다. */
#include <string>
#include <vector>
#include <algorithm>   /* E3R: SD 후보 정렬 */
#include <dirent.h>   /* E3R: /storage 열거 */
static const char * const KHA_BASE = "/storage/emulated/0/Kyuhachi";   /* 사용자에게는 「내부 저장소/Kyuhachi」 로만 보인다 */
static std::vector<std::string> g_kha_libs;
static volatile bool g_kha_fg = false;
static std::string kha_cfgpath(void) { return gApp.exedir + "\\android.ini"; }
static bool kha_pad_is_retro(SDL_JoystickID id) {   /* t147 (공지522): 패드 이름에 xbox/ps 가 없으면 닌텐도 배치(retro) */
    if (ov::g_lblcfg == 1) return true; if (ov::g_lblcfg == 2) return false;
    SDL_GameController * gc = SDL_GameControllerFromInstanceID(id); const char * nm = gc ? SDL_GameControllerName(gc) : NULL;
    return gensei_padkind_of(nm) == 0;
}
static int SDLCALL kha_pad_swap_watch(void *, SDL_Event * e) {   /* 이벤트가 큐에 들어가기 전에 버튼 번호를 바꾼다 → 게임·메뉴 모두 같은 값으로 본다 */
    if (e->type != SDL_CONTROLLERBUTTONDOWN && e->type != SDL_CONTROLLERBUTTONUP) return 0;
    Uint8 b = e->cbutton.button, n = b;
    if (b == SDL_CONTROLLER_BUTTON_A) n = SDL_CONTROLLER_BUTTON_B; else if (b == SDL_CONTROLLER_BUTTON_B) n = SDL_CONTROLLER_BUTTON_A; else if (b == SDL_CONTROLLER_BUTTON_X) n = SDL_CONTROLLER_BUTTON_Y; else if (b == SDL_CONTROLLER_BUTTON_Y) n = SDL_CONTROLLER_BUTTON_X;
    if (n != b && kha_pad_is_retro(e->cbutton.which)) { static int pn = 0; if (pn < 12) { pn++; gh_log("KHA padswap %d→%d (%d/12)", (int)b, (int)n, pn); } e->cbutton.button = n; }
    return 0;
}
static void kha_pad_swap_reg(void) { static bool r = false; if (r || !SDL_WasInit(SDL_INIT_VIDEO)) return; r = true; SDL_AddEventWatch(kha_pad_swap_watch, NULL); gh_log("KHA padswap watch registered (A<->B, X<->Y when pad is retro)"); }
static void kha_cfg_load(void) {
    kha_pad_swap_reg();
    ov::g_kha_save = kha_cfg_save;
    g_kha_libs.clear();
    FILE * f = fopen(kha_cfgpath().c_str(), "r"); if (!f) { gh_log("KHA cfg: android.ini missing (defaults padlayout=auto sndbuf=%d)", ov::g_sndbuf); return; }
    char ln[1100];
    while (fgets(ln, sizeof ln, f)) {
        std::string s = ln; while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        size_t e = s.find('='); if (e == std::string::npos) continue; std::string k = s.substr(0, e), v = s.substr(e + 1);
        if (k == "padlayout") ov::g_lblcfg = (v == "retro") ? 1 : (v == "xbox") ? 2 : 0;
        else if (k == "sndbuf") { int n = atoi(v.c_str()); if (n >= 0 && n < 4) ov::g_sndbuf = n; }
        else if (k == "cpucore") { ov::g_cpucore = atoi(v.c_str()) ? 1 : 0; }   /* t733: 줄이 없으면 기본(빠름) */
        else if (k == "lib" && !v.empty()) g_kha_libs.push_back(v);
        else if (k == "fontdir") ov::g_kha_fontdir = v;   /* E3R */
    }
    fclose(f);
    gh_log("KHA cfg: padlayout=%s sndbuf=%d libs=%d", ov::g_lblcfg == 1 ? "retro" : ov::g_lblcfg == 2 ? "xbox" : "auto", ov::g_sndbuf, (int)g_kha_libs.size());
}
static void kha_cfg_save(void) {
    FILE * f = fopen(kha_cfgpath().c_str(), "w"); if (!f) { gh_log("KHA cfg: save failed"); return; }
    fprintf(f, "padlayout=%s\nsndbuf=%d\n", ov::g_lblcfg == 1 ? "retro" : ov::g_lblcfg == 2 ? "xbox" : "auto", ov::g_sndbuf);
    fprintf(f, "cpucore=%d\n", ov::g_cpucore);   /* t733 */
    for (auto & d : g_kha_libs) fprintf(f, "lib=%s\n", d.c_str());
    if (!ov::g_kha_fontdir.empty()) fprintf(f, "fontdir=%s\n", ov::g_kha_fontdir.c_str());   /* E3R */
    fclose(f);
}
static void kha_lib_init(void) {
    if (gh_all_files_ok()) {
        std::string b = KHA_BASE; plat::mkdir(b); plat::mkdir(b + "/games"); plat::mkdir(b + "/fonts");
        std::string g = b + "/여기에_넣으세요.txt";
        if (!plat::exists(g)) { FILE * f = fopen(g.c_str(), "w"); if (f) { fputs(phr_("games 폴더 안에 게임 폴더(.fdi .hdi .d88 등)를 넣고 앱에서 「다시 검색」을 누르세요.\nfonts 폴더에는 글꼴 BMP 를 넣으세요.\n"), f); fclose(f); } }
    }
    if (gApp.ui_.gamefolder.empty()) { gApp.ui_.gamefolder = std::string(KHA_BASE) + "/games"; ui::settings_save(gApp.ui_, gApp.cfgpath); gh_log("KHA default game folder = %s", gApp.ui_.gamefolder.c_str()); }
}
static int gh_scan_gamefolder(const char * why);
static void gh_android_savedir(void);
static void kha_perm_ask_once(void) {   /* 창을 만들기 전(시작 초반) 1회: 모든 파일 접근 허용 화면을 열고, 돌아올 때까지 기다린 뒤 정상 시작 (창이 없을 때 쉬어야 복귀 암전이 없다) */
    if (gh_all_files_ok()) return;
    std::string mk = gApp.exedir + "\\perm_asked.txt";
    if (plat::exists(mk)) { gh_log("KHA all-files access missing (settings screen already opened once)"); return; }
    { FILE * f = fopen(mk.c_str(), "w"); if (f) { fputs("1\n", f); fclose(f); } }
    gh_log("KHA opening all-files access screen (once, before window creation)"); gh_all_files_ask();
    unsigned t0 = SDL_GetTicks(); bool fg = false, bg = false;
    for (;;) {
        SDL_Event ev; while (SDL_PollEvent(&ev)) { if (ev.type == SDL_APP_DIDENTERBACKGROUND) bg = true; if (ev.type == SDL_APP_DIDENTERFOREGROUND) fg = true; }
        if (bg && fg) break; if (SDL_GetTicks() - t0 > 20000u && !bg) break; if (SDL_GetTicks() - t0 > 300000u) break; SDL_Delay(100);
    }
    gh_log("KHA access result: %s (bg=%d fg=%d)", gh_all_files_ok() ? "granted" : "denied", (int)bg, (int)fg);
}
static int kha_scan_extra(const char * why) {
    int tot = 0;
    for (auto & d : g_kha_libs) {
        std::string dir = aw::u8_to_acp(d); if (!aw::dir_exists(dir)) { gh_log("KHA registered folder missing: %s", dir.c_str()); continue; }
        std::vector<aw::Work> ws; aw::collect(dir, 3, ws); aw::Summary S = gh_add_works(ws, why);
        gh_log("KHA registered folder %s: candidates %d, added %d", dir.c_str(), (int)ws.size(), S.added); tot += S.added;
    }
    return tot;
}
static void kha_lib_add(const std::string & d) {
    for (auto & x : g_kha_libs) if (x == d) return;
    g_kha_libs.push_back(d); kha_cfg_save(); gh_log("KHA folder registered: %s (total %d)", d.c_str(), (int)g_kha_libs.size());
}
static void kha_lib_clear(void) { g_kha_libs.clear(); kha_cfg_save(); }
static void kha_font_dirs(std::vector<std::string> & out) {   /* E3R: ①fontdir ②내부 ③SD ④등록 (⑤앱은 호출 쪽) */
    out.clear();
    if (!ov::g_kha_fontdir.empty()) out.push_back(ov::g_kha_fontdir);
    out.push_back(std::string(KHA_BASE) + "/fonts");
    if (gh_all_files_ok()) {   /* E3R: 권한 없으면 SD 후보 스킵 */
        DIR * d = opendir("/storage");
        if (d) {
            std::vector<std::string> ns; struct dirent * e;
            while ((e = readdir(d)) != NULL) {
                std::string n = e->d_name;
                if (n.empty() || n[0] == '.') continue;
                if (n == "emulated" || n == "self") continue;
                ns.push_back(n);
            }
            closedir(d); std::sort(ns.begin(), ns.end());
            for (size_t i = 0; i < ns.size(); i++) out.push_back("/storage/" + ns[i] + "/Kyuhachi/fonts");
        }
    }
    for (size_t i = 0; i < g_kha_libs.size(); i++) out.push_back(g_kha_libs[i] + "/fonts");
    std::vector<std::string> u;   /* E3R: 중복 제거(순서 유지) */
    for (size_t i = 0; i < out.size(); i++) { bool dup = false;
        for (size_t k = 0; k < u.size(); k++) if (u[k] == out[i]) dup = true;
        if (!dup) u.push_back(out[i]); }
    out.swap(u);
}
static std::string kha_font_lookup(const std::string & name) {   /* E3R: ①→②→③→④, 없으면 빈 문자열 */
    std::vector<std::string> ds; kha_font_dirs(ds);
    for (size_t i = 0; i < ds.size(); i++) { std::string p = ds[i] + "/" + name + ".bmp"; if (plat::exists(p)) return p; }
    return std::string();
}
static bool kha_is_sd(const std::string & p) {   /* E3R: /storage/UUID (emulated 제외) */
    if (p.compare(0, 9, "/storage/") != 0) return false;
    return p.compare(9, 9, "emulated/") != 0;
}
static void kha_font_rescan(void) {   /* E3R: 목록 다시 찾기 + 한 줄 안내 */
    gh_font_rebuild();
    int okf = 0; std::string sdn;
    for (size_t i = 0; i < gApp.ui_.fonts.size(); i++) {
        const ui::UiFont & f = gApp.ui_.fonts[i];
        if (!f.ok) continue;
        if (!ov::g_kha_fontdir.empty() && f.path.compare(0, ov::g_kha_fontdir.size(), ov::g_kha_fontdir) == 0) okf++;
        if (sdn.empty() && kha_is_sd(f.path)) sdn = f.name;
    }
    if (!ov::g_kha_fontdir.empty() && okf == 0) {
        gApp.note = T("글꼴 폴더를 못 찾음 — 직접 지정하세요"); gApp.note_until = SDL_GetTicks() + 8000u;
    } else if (!sdn.empty()) {
        gApp.note = std::string(T("SD카드에서 글꼴 찾음: ")) + sdn; gApp.note_until = SDL_GetTicks() + 6000u;
    }
    gh_log("KHA fontrescan: fonts=%d fontdir=%s", (int)gApp.ui_.fonts.size(),
        ov::g_kha_fontdir.empty() ? "(auto)" : ov::g_kha_fontdir.c_str());
}
static void kha_fontdir_set(const std::string & d) {   /* E3R: fontdir 저장 → 다시 찾기 */
    ov::g_kha_fontdir = d; kha_cfg_save(); kha_font_rescan();
}
static bool kha_fontdir_pick(void) {   /* E3R: 폴더 고르기 → fontdir 저장 → 다시 찾기 */
    std::string d;
    if (!gh_folder_explorer(T("글꼴 폴더 고르기"), d)) return false;
    while (d.size() > 1 && (d.back() == '/' || d.back() == '\\')) d.pop_back();
    kha_fontdir_set(d);
    gh_log("KHA fontdir picked: %s", d.c_str());
    return true;
}
static std::string kha_mixer_conf(void) {
    static const int bs[4] = { 512, 1024, 2048, 4096 }, pb[4] = { 20, 40, 60, 100 }; char b[96];
    snprintf(b, sizeof b, "[mixer]\nblocksize=%d\nprebuffer=%d\n", bs[ov::g_sndbuf], pb[ov::g_sndbuf]);
    gh_log("KHA sound buffer level %d: blocksize=%d prebuffer=%d", ov::g_sndbuf, bs[ov::g_sndbuf], pb[ov::g_sndbuf]); return b;
}
/* 상태 불러오기 뒤 검은 화면 진단: 불러온 직후·0.5·1.5·4초 뒤 프레임버퍼 크기와 비검정 화소 수 */
static unsigned g_kha_ld_at = 0; static int g_kha_ld_ph = 0;
static void kha_ld_log(const char * tag, int ph) {
    size_t n = gApp.fb.size(), nz = 0; for (size_t i = 0; i < n; i += 5) if (gApp.fb[i] & 0xFFFFFFu) nz++;
    gh_log("LOADDIAG %s ph=%d fb=%dx%d n=%u nonblack(1/5)=%u gl=%d boot_tick=%d ui_dirty=%d", tag, ph, gApp.fbw, gApp.fbh, (unsigned)n, (unsigned)nz, (int)gApp.use_gl, gApp.boot_tick ? 1 : 0, (int)gApp.ui_.dirty);
}
static void kha_ld_diag(const char * tag) { g_kha_ld_at = SDL_GetTicks(); if (!g_kha_ld_at) g_kha_ld_at = 1; g_kha_ld_ph = 0; kha_ld_log(tag, 0); }
static void kha_ld_tick(unsigned now) {
    if (!g_kha_ld_at) return; static const unsigned T[3] = { 500u, 1500u, 4000u };
    if (g_kha_ld_ph < 3 && now - g_kha_ld_at >= T[g_kha_ld_ph]) { kha_ld_log("after", g_kha_ld_ph + 1); g_kha_ld_ph++; if (g_kha_ld_ph >= 3) g_kha_ld_at = 0; }
}
/* 복귀 암전 수정(2차): SDL 안드로이드는 앱이 돌아올 때 EGL 컨텍스트를 이벤트 펌프(메인) 스레드에 붙인다(이미 다른 스레드가 쥐고 있으면 새로 만들어 버려 셰이더·텍스처가 사라진다).
 * 우리 그리기는 별도 gl 스레드이므로: 내려가기 직전에 gl 스레드를 쉬게(컨텍스트 놓음) -> 복귀 때 메인이 붙인 컨텍스트를 놓고 -> 잠시 뒤 gl 스레드 재개. */
static void kha_glfix(const char * why) {
    if (!gApp.glthr || !gApp.win) return;
    { SDL_GLContext cur = SDL_GL_GetCurrentContext(); gh_log("APPLIFE glfix(%s): current context cur=%p glctx=%p%s", why, (void *)cur, (void *)gApp.glctx, (cur && cur != gApp.glctx) ? " differs (newly created)" : ""); }
    SDL_GL_MakeCurrent(gApp.win, NULL); g_kha_hold = SDL_GetTicks() + 500u; if (!g_kha_hold) g_kha_hold = 1;
    gh_log("APPLIFE glfix(%s): main-thread context released, gl thread resumes in 0.5 s", why);
}
static void kha_ev_watch(const SDL_Event * e) {
    kha_pad_swap_reg();
    switch (e->type) {
    case SDL_APP_WILLENTERBACKGROUND: {
        gh_log("APPLIFE willenterbackground (emulation and sound paused)");
        if (gApp.glthr) { g_kha_hold = 0xFFFFFFFFu; unsigned t0 = SDL_GetTicks(); while (!g_kha_glidle && SDL_GetTicks() - t0 < 400u) SDL_Delay(5); gh_log("APPLIFE gl thread paused idle=%d (%u ms)", (int)g_kha_glidle, SDL_GetTicks() - t0); }
        break; }
    case SDL_APP_DIDENTERBACKGROUND: gh_log("APPLIFE didenterbackground"); break;
    case SDL_APP_WILLENTERFOREGROUND: gh_log("APPLIFE willenterforeground"); kha_glfix("willfg"); break;
    case SDL_APP_DIDENTERFOREGROUND: g_kha_fg = true; gh_log("APPLIFE didenterforeground (resumed)"); kha_glfix("didfg"); break;
    case SDL_WINDOWEVENT:
        if (g_kha_hold == 0xFFFFFFFFu && (e->window.event == SDL_WINDOWEVENT_RESTORED || e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED || e->window.event == SDL_WINDOWEVENT_EXPOSED)) kha_glfix("win"); { static int wn = 0; int ev = e->window.event; if (wn < 60 && (ev == SDL_WINDOWEVENT_SIZE_CHANGED || ev == SDL_WINDOWEVENT_RESIZED || ev == SDL_WINDOWEVENT_FOCUS_GAINED || ev == SDL_WINDOWEVENT_RESTORED)) { wn++; int dw = 0, dh = 0; if (gApp.win) SDL_GL_GetDrawableSize(gApp.win, &dw, &dh); gh_log("KHA win ev=%d d1=%d d2=%d drawable=%dx%d (%d/60)", ev, e->window.data1, e->window.data2, dw, dh, wn); } }
        break;
    case SDL_FINGERDOWN: case SDL_MOUSEBUTTONDOWN: { static int tn = 0; if (tn < 40) { tn++; gh_log("KHA touch arrived type=0x%x (%d/40)", (unsigned)e->type, tn); } break; }   /* t146: 오버레이 안 눌림 진단 */
    case SDL_RENDER_DEVICE_RESET: gh_log("APPLIFE SDL_RENDER_DEVICE_RESET (context recreated)"); break;
    case SDL_RENDER_TARGETS_RESET: gh_log("APPLIFE SDL_RENDER_TARGETS_RESET"); break;
    case SDL_AUDIODEVICEADDED: gh_log("AUDIODEV added which=%d capture=%d", (int)e->adevice.which, (int)e->adevice.iscapture); break;
    case SDL_AUDIODEVICEREMOVED: gh_log("AUDIODEV removed which=%d capture=%d", (int)e->adevice.which, (int)e->adevice.iscapture); break;
    default: break;
    }
}
/* APK 에 든 기본 자원(글꼴·리듬 소리·셰이더·문구·게임 DB)을 앱 폴더로 한 번 풀어 둔다 — 이미 있는 파일은 건드리지 않는다. 사용자가 내부 저장소/Kyuhachi\fonts 에 넣은 글꼴이 먼저 쓰인다. */
static void kha_seed_assets(void) {
    static const char * const T[][2] = {
        { "kha/fonts/kyuhachi_font.bmp", "res\\fonts\\kyuhachi_font.bmp" }, { "kha/fonts/kyuhachi_font.fnt", "res\\fonts\\kyuhachi_font.fnt" }, { "kha/fonts/kyuhachi_font_sbcs.fnt", "res\\fonts\\kyuhachi_font_sbcs.fnt" },
        { "kha/fonts/shinonome16_jp.fnt", "res\\fonts\\shinonome16_jp.fnt" },
        { "kha/rhythm/2608_bd.wav", "res\\rhythm\\2608_bd.wav" }, { "kha/rhythm/2608_sd.wav", "res\\rhythm\\2608_sd.wav" }, { "kha/rhythm/2608_top.wav", "res\\rhythm\\2608_top.wav" },
        { "kha/rhythm/2608_hh.wav", "res\\rhythm\\2608_hh.wav" }, { "kha/rhythm/2608_tom.wav", "res\\rhythm\\2608_tom.wav" }, { "kha/rhythm/2608_rim.wav", "res\\rhythm\\2608_rim.wav" },
        { "kha/shaders/pc98_sharp.glsl", "res\\shaders\\pc98_sharp.glsl" }, { "kha/shaders/pc98_crt.glsl", "res\\shaders\\pc98_crt.glsl" },
        { "kha/dsn/fdd_insert.wav", "res\\disk-noises\\fdd_insert.wav" }, { "kha/dsn/fdd_eject.wav", "res\\disk-noises\\fdd_eject.wav" }, { "kha/dsn/fdd_read.wav", "res\\disk-noises\\fdd_read.wav" },
        { "kha/dsn/fdd_spin.wav", "res\\disk-noises\\fdd_spin.wav" },
        { "kha/dsn/hdd_spin.wav", "res\\disk-noises\\hdd_spin.wav" }, { "kha/dsn/hdd_spinup.wav", "res\\disk-noises\\hdd_spinup.wav" }, { "kha/dsn/fdd_seek1.wav", "res\\disk-noises\\fdd_seek1.wav" }, { "kha/dsn/fdd_seek2.wav", "res\\disk-noises\\fdd_seek2.wav" },
        { "kha/dsn/hdd_seek1.wav", "res\\disk-noises\\hdd_seek1.wav" }, { "kha/dsn/hdd_seek2.wav", "res\\disk-noises\\hdd_seek2.wav" },
        { "kha/dsn/CREDITS.txt", "res\\disk-noises\\CREDITS.txt" },
        { "kha/dsn/CREDITS_hdd.txt", "res\\disk-noises\\CREDITS_hdd.txt" },
        { "kha/dsn/boot_click.wav", "res\\disk-noises\\boot_click.wav" },
        { "kha/dsn/boot_spinup.wav", "res\\disk-noises\\boot_spinup.wav" },
        { "kha/dsn/hum_loop.wav", "res\\disk-noises\\hum_loop.wav" },
        { "kha/dsn/boot_noise.wav", "res\\disk-noises\\boot_noise.wav" },
        { "kha/dsn/boot_beep.wav", "res\\disk-noises\\boot_beep.wav" },
        { "kha/dsn/crt_on.wav", "res\\disk-noises\\crt_on.wav" }, { "kha/dsn/crt_off.wav", "res\\disk-noises\\crt_off.wav" },   /* t714: CRT 켜짐/꺼짐 소리 — 풀기 목록에 빠져 노바에서 「CRT sfx 없음」 */
        { "kha/dsn/CREDITS_boot.txt", "res\\disk-noises\\CREDITS_boot.txt" },
        { "kha/flags.ini", "res\\flags.ini" }, { "kha/gamecontrollerdb.txt", "res\\gamecontrollerdb.txt" },
        { "kha/strings_ko.txt", "strings_ko.txt" }, { "kha/strings_en.txt", "strings_en.txt" }, { "kha/strings_ja.txt", "strings_ja.txt" },
        { "kha/gamedb.ini", "gamedb.ini" }, { "kha/gamedb_nk.tsv", "gamedb_nk.tsv" }, { "kha/gamedb_nk_boot.tsv", "gamedb_nk_boot.tsv" }, { "kha/gamedb_nk_stem.tsv", "gamedb_nk_stem.tsv" } };   /* #L5: 기동 상태표(빌드 때 assets/kha 에 복사) */
    plat::mkdir(gApp.exedir + "\\res"); plat::mkdir(gApp.exedir + "\\res\\fonts"); plat::mkdir(gApp.exedir + "\\res\\rhythm"); plat::mkdir(gApp.exedir + "\\res\\shaders"); plat::mkdir(gApp.exedir + "\\res\\disk-noises");
    int made = 0, have = 0, miss = 0;
    for (auto & p : T) {
        std::string dst = gApp.exedir + "\\" + p[1]; if (plat::exists(dst) && strncmp(p[1], "strings_", 8) != 0 && strncmp(p[1], "gamedb_nk_boot", 14) != 0 && strncmp(p[1], "gamedb_nk_stem", 14) != 0) { have++; continue; }   /* 4: 문구 파일은 APK 것으로 매번 갱신(사용자 덮어쓰기는 strings.txt) */
        SDL_RWops * r = SDL_RWFromFile(p[0], "rb"); if (!r) { miss++; continue; }
        Sint64 n = SDL_RWsize(r); std::vector<char> b; if (n > 0) { b.resize((size_t)n); size_t got = SDL_RWread(r, b.data(), 1, (size_t)n); b.resize(got); } SDL_RWclose(r);
        FILE * f = fopen(dst.c_str(), "wb"); if (f) { fwrite(b.data(), 1, b.size(), f); fclose(f); made++; } else miss++;
    }
    gh_log("KHA default resources unpacked: new %d, existing %d, failed/missing %d", made, have, miss);
}
/* 소리 버퍼: 공통 믹서 설정 문자열의 blocksize/prebuffer 값을 단계에 맞춰 바꾼다 (별도 ParseConfigString 호출은 쓰지 않는다) */
static std::string kha_mixer_patch(std::string cc) {
    static const int bs[4] = { 512, 1024, 2048, 4096 }, pb[4] = { 20, 40, 60, 100 };
    auto rp = [&](const std::string & a, const std::string & b) { size_t p = cc.find(a); if (p != std::string::npos) cc.replace(p, a.size(), b); };
    rp("blocksize=512", "blocksize=" + std::to_string(bs[ov::g_sndbuf])); rp("prebuffer=20", "prebuffer=" + std::to_string(pb[ov::g_sndbuf]));
    rp("core=normal", ov::g_cpucore ? "core=dynamic" : "core=normal"); gh_log("KHA cpu core: %s", ov::g_cpucore ? "dynamic(빠름)" : "normal(호환)");   /* t733 */
    gh_log("KHA sound buffer level %d: blocksize=%d prebuffer=%d", ov::g_sndbuf, bs[ov::g_sndbuf], pb[ov::g_sndbuf]); return cc;
}

/* ---- t145: 안드로이드 프로세스 재시작 (메인 메뉴로 → 창 안 전환 불가일 때의 폴백). 인자를 next_args.txt 에 적고 KhActivity.khRestart() → 별 프로세스 KhPhoenix 가 이 프로세스를 끝내고 앱을 다시 띄운다. */
#include <jni.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
extern "C" bool kha_relaunch_c(const char * cmd) {
    const char * ip = SDL_AndroidGetInternalStoragePath(); if (!ip || !*ip) { gh_log("KHA relaunch: internal storage path missing"); return false; }
    std::string p = std::string(ip) + "/next_args.txt";
    FILE * f = fopen(p.c_str(), "w"); if (!f) { gh_log("KHA relaunch: next_args write failed"); return false; }
    fputs(cmd, f); fputc('\n', f); fclose(f);
    JNIEnv * env = (JNIEnv *)SDL_AndroidGetJNIEnv(); jobject act = (jobject)SDL_AndroidGetActivity(); if (!env || !act) { gh_log("KHA relaunch: JNI unavailable"); return false; }
    jclass c = env->GetObjectClass(act); jmethodID m = c ? env->GetMethodID(c, "khRestart", "()V") : NULL;
    if (!m) { if (env->ExceptionCheck()) env->ExceptionClear(); gh_log("KHA relaunch: khRestart missing"); return false; }
    env->CallVoidMethod(act, m); if (env->ExceptionCheck()) { env->ExceptionClear(); gh_log("KHA relaunch: exception"); return false; }
    gh_log("KHA relaunch requested: %s", cmd); fflush(NULL); return true;
}
static bool kha_take_nextargs(std::vector<std::string> & out) {
    const char * ip = SDL_AndroidGetInternalStoragePath(); if (!ip || !*ip) return false;
    std::string p = std::string(ip) + "/next_args.txt"; struct stat st; if (stat(p.c_str(), &st) != 0) return false;
    bool fresh = (long)(time(NULL) - st.st_mtime) < 60; std::string line;
    { FILE * f = fopen(p.c_str(), "r"); if (f) { char b[4096]; if (fgets(b, sizeof b, f)) line = b; fclose(f); } }
    unlink(p.c_str()); if (!fresh) return false;
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
    std::string cur; bool q = false, has = false; std::vector<std::string> t;
    for (char ch : line) { if (ch == '"') { q = !q; has = true; } else if (ch == ' ' && !q) { if (has || !cur.empty()) t.push_back(cur); cur.clear(); has = false; } else cur += ch; }
    if (has || !cur.empty()) t.push_back(cur);
    if (t.size() < 2) return false;
    for (size_t i = 1; i < t.size(); i++) out.push_back(t[i]);
    return true;
}
