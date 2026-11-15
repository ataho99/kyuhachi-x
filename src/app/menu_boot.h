/* app/menu_boot.h -- S9b: beep, logo, boot sequence, menu run. Moved unchanged from gensei_host.cpp lines 1314-1513 (as of t54t). Included in place; single translation unit. */
/* ---- 부팅 연출(약 1.8초) · 짧은 로딩 · 삐뽀(음량 ≈1/3 × 효과음 설정) */
static int g_beep_id = 0;   /* #B4: 삑 소리 번호(gau) */
static void gh_beep(float se_gain) {
    if (gApp.headless) return;
    int amp = (int)(3000.0f * se_gain); if (amp <= 0) return;
    std::vector<int16_t> pcm; const int R = 44100; const double CLK = 1996800.0;
    auto tone = [&](int div, int ms) { double f = CLK / div; int n = R * ms / 1000; for (int i = 0; i < n; i++) { double ph = fmod(i * f / R, 1.0); pcm.push_back((int16_t)(ph < 0.5 ? amp : -amp)); } };
    auto sil = [&](int ms) { pcm.insert(pcm.end(), (size_t)(R * ms / 1000), (int16_t)0); };
    tone(998, 90); sil(40); tone(1997, 160); sil(100);
    g_beep_id = gau::play(pcm.data(), pcm.size(), R); if (!g_beep_id) { gh_log("beep: open fail %s", gau::err); return; }   /* #B4: 공유 장치 */
    gh_log("beep: amp=%d", amp);
}
static void gh_beep_close(void) { if (g_beep_id) { gau::stop(g_beep_id); g_beep_id = 0; } }
/* t52d (#129): 시작 로고 — boot_logo_160.txt 1~18줄(160칸 × 4×8 픽셀: # 꽉 참 · + 체크무늬 50% · . 25% 점) + 아래 가운데 KYUHACHI(비트맵 글자). 검정 바탕 흰색 */
#include "boot_logo_x1.h"
#include "boot_logo_x2.h"
#include "crt_on_fx.h"   /* #S3d: CRT 켜짐 화면 연출(약·중·강) */
static unsigned char kh_logox_raw[64000]; static bool kh_logox_ok = false;
static bool kh_logox_load(void) {   /* build 11: base64 -> 640x400 2bpp indices (once) */
    if (kh_logox_ok) return true;
    static signed char dec[256]; for (int i = 0; i < 256; i++) dec[i] = -1;
    const char * al = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; for (int i = 0; i < 64; i++) dec[(unsigned char)al[i]] = (signed char)i;
    size_t o = 0; unsigned acc = 0; int bits = 0;
    const char * const * parts[2] = { KH_LOGOX_A, KH_LOGOX_B };
    const size_t cnt[2] = { sizeof KH_LOGOX_A / sizeof KH_LOGOX_A[0], sizeof KH_LOGOX_B / sizeof KH_LOGOX_B[0] };
    for (int p = 0; p < 2; p++) for (size_t i = 0; i < cnt[p]; i++) for (const char * s = parts[p][i]; *s; s++) {
        int v = dec[(unsigned char)*s]; if (v < 0) continue; acc = (acc << 6) | (unsigned)v; bits += 6;
        if (bits >= 8) { bits -= 8; if (o < sizeof kh_logox_raw) kh_logox_raw[o++] = (unsigned char)((acc >> bits) & 255); } }
    kh_logox_ok = (o == sizeof kh_logox_raw); return kh_logox_ok;
}
static const char * const KH_LOGO[18] = {
    "                                                                                                                                                                ",
    "                                                                                                                                                                ",
    "                                                                                                                                                                ",
    "                                                       ...........                                                  .++..                                       ",
    "                                                       .........                                            ...+++.........+                                    ",
    "                                                     .+........                                     +.......    .......+                                        ",
    "                                                    +++++++++                                      ++++++++    ++++++++                                         ",
    "                                      ##+++++++++++++++++++++++++++++++++++++                     #++++++#   ..+++++++                                          ",
    "                                                  ########.      +########                      .#######+     ########                                          ",
    "                                                +########       ########.                      +#######.     +#######+                                          ",
    "                                              .########.      +########                      +########       +########                                          ",
    "                                             +++++++++      .++++++++.           +         +++++++++        .+++++++++                                          ",
    "                                          ++.......+       +........           +         +........           ...........                                        ",
    "                                       ++++++++++.       +++..+.++         .++        .++++++++.             ++++++++++++.                                      ",
    "                                   .#+++++++++.        +++++++++        ++++.       #+++++++.                 ++++++++++++++                                    ",
    "                              .+#+++++++++.          ++++++++++++++#++++++.     +##+++++.                     .+++++++#++++++##+.                               ",
    "                       . +#########+.               +##################+   ..#####+.                           ###########++++++++###+  .                       ",
    "                 ++######+...                        ...............   +##+..                                   +##..                                           "
};

static void gh_logo_draw_old(ov::Canvas & c) {
    c.rect(0, 0, 640, 400, ov::K);
    const int y0 = (400 - 197) / 2;
    for (int r = 0; r < 18; r++) {
        const char * ls = KH_LOGO[r];
        for (int q = 0; q < 160; q++) { char ch = ls[q]; if (ch == ' ') continue;
            for (int yy = 0; yy < 8; yy++) for (int xx = 0; xx < 4; xx++) {
                int px = q * 4 + xx, py = r * 8 + yy; bool on = false;
                if (ch == '#') on = true; else if (ch == '+') on = (((px + py) & 1) == 0); else if (ch == '.') on = (((px & 1) == 0) && ((py & 1) == 0));
                if (on) c.set(px, y0 + py, ov::WH); } }
    }
    static uint32_t tb[64 * 16]; ov::Canvas t = { tb, 64, 16 }; t.rect(0, 0, 64, 16, ov::K); ov::text(t, 0, 0, "KYUHACHI", ov::WH, ov::K);
    const int tx = 320 - (8 * 24 - 8) / 2, ty = y0 + 155;
    for (int i = 0; i < 8; i++) for (int sy = 0; sy < 16; sy++) for (int sx = 0; sx < 8; sx++) {
        if (tb[sy * 64 + i * 8 + sx] == ov::K) continue;
        int dy0 = (sy * 3) / 2, dy1 = ((sy + 1) * 3) / 2; c.rect(tx + i * 24 + sx * 2, ty + dy0, 2, dy1 - dy0, ov::WH); }
}
static void gh_logo_draw(ov::Canvas & c) {   /* build 11: embedded 4-color logo; old two-tone drawing only if decoding fails */
    if (!kh_logox_load()) { gh_logo_draw_old(c); return; }
    for (int y = 0; y < 400; y++) for (int x = 0; x < 640; x++) {
        const unsigned char b = kh_logox_raw[y * 160 + (x >> 2)]; c.set(x, y, KH_LOGOX_PAL[(b >> (6 - 2 * (x & 3))) & 3]); }
}
static void gh_logo_frame(void) { static uint32_t buf[640 * 400]; ov::Canvas c = { buf, 640, 400 }; gh_logo_draw(c); gh_publish(buf, 640, 400); }
static void gh_boot_frame(int stage, bool cells) {
    static uint32_t buf[640 * 400]; ov::Canvas c = { buf, 640, 400 }; c.rect(0, 0, 640, 400, ov::K);
    if (stage >= 1) ov::text(c, 40, 112, std::string(KH_FULLNAME), ov::WH, ov::K);
    if (stage >= 2) ov::text(c, 40, 144, std::string("Now Loading") + std::string((size_t)(stage - 1 > 3 ? 3 : stage - 1), '.'), ov::WH, ov::K);
    if (cells) ov::fncells_boot(c);   /* t52a (#49) */
    gh_publish(buf, 640, 400);
}
/* v0.6.8: CRT 켜짐/꺼짐 연출 — 설정 「부팅 연출」(pwrseq) 에 묶음(끄면 둘 다 없음). 소리 res\disk-noises\crt_on.wav · crt_off.wav 는 합성(원본 아님), 44.1kHz 그대로 열어 SDL 이 장치 형식으로 바꾼다. */
static int g_crt_lvl = 2;   /* #S3d: 켜짐 화면 단계 0=예전 그대로 1=약(e) 2=중(a+b+e) 3=강(a+b+d+e). 기본 중 — 슨상이 시안을 고르면 이 값만 바꾼다. 꺼짐·소리는 무관 */
static int g_crt_id = 0; static unsigned g_crt_end = 0;   /* #B4: CRT 소리 번호(gau 공유 장치) */
static bool gh_crt_enabled(void) { return !gApp.headless && gApp.use_gl && gApp.uitest_dir.empty() && gApp.ft_dir.empty() && gApp.ui_.effective(gApp.cur_game).pwrseq != 0; }
static void gh_sfx_slot(const char * nm, int * pid, unsigned * pend, float gmul) {
    if (!gh_crt_enabled()) return;
    const float g = gApp.ui_.effective(gApp.cur_game).se_gain() * gmul; if (g <= 0.0f) return;
    std::string p = gApp.exedir + "\\res\\disk-noises\\" + nm; SDL_AudioSpec sp; Uint8 * wb = NULL; Uint32 wl = 0;
    std::vector<unsigned char> fb; { FILE * ff = fopen(p.c_str(), "rb"); if (ff) { unsigned char tb[4096]; size_t nn; while ((nn = fread(tb, 1, sizeof tb, ff)) > 0) fb.insert(fb.end(), tb, tb + nn); fclose(ff); } }   /* exedir 는 ANSI 문자열 — SDL_LoadWAV(경로)는 UTF-8 로 읽어 한글 폴더에서 실패하므로 메모리로 읽는다 */
    if (fb.empty() || !SDL_LoadWAV_RW(SDL_RWFromConstMem(fb.data(), (int)fb.size()), 1, &sp, &wb, &wl)) { gh_log("CRT sfx 없음 %s", p.c_str()); return; }
    if (sp.format != AUDIO_S16SYS || sp.channels != 1 || sp.freq < 8000) { SDL_FreeWAV(wb); gh_log("CRT sfx 형식 불가 %s", nm); return; }
    std::vector<int16_t> v((size_t)wl / 2); memcpy(v.data(), wb, v.size() * 2); SDL_FreeWAV(wb);
    for (size_t i = 0; i < v.size(); i++) { float f = v[i] * g; v[i] = (int16_t)(f > 32767.0f ? 32767.0f : f < -32768.0f ? -32768.0f : f); }
    if (*pid) { gau::stop(*pid); *pid = 0; }
    *pid = gau::play(v.data(), v.size(), sp.freq); if (!*pid) { gh_log("CRT sfx 장치 열기 실패 %s", gau::err); return; }
    *pend = SDL_GetTicks() + (unsigned)((uint64_t)v.size() * 1000 / (uint64_t)sp.freq) + 80u; gh_log("CRT sfx %s %dHz %.2fs gain=%.2f", nm, sp.freq, (double)v.size() / sp.freq, (double)g);
}
static void gh_crt_sfx(const char * nm) { gh_sfx_slot(nm, &g_crt_id, &g_crt_end, 0.8f); }
static int g_fdd_id = 0; static unsigned g_fdd_end = 0;   /* t795: 부팅 연출 FDD 소리(디스크 소리 설정 끔이면 없음) */
static void gh_fdd_sfx(const char * nm) { const int ds = gApp.ui_.effective(gApp.cur_game).dsnd; if (ds == 0 || (ds == 1 && strstr(nm, "read"))) return; gh_sfx_slot(nm, &g_fdd_id, &g_fdd_end, 0.9f); }
static inline uint32_t gh_crt_px(float r, float g, float b) { int R = r > 255 ? 255 : r < 0 ? 0 : (int)r, G = g > 255 ? 255 : g < 0 ? 0 : (int)g, B = b > 255 ? 255 : b < 0 ? 0 : (int)b; return 0xFF000000u | ((uint32_t)R << 16) | ((uint32_t)G << 8) | (uint32_t)B; }
static void gh_crt_frame(uint32_t * o, bool on, float p, const uint32_t * src, int lvl = -1) {   /* 640x400 한 장. p = 0..1 (켜짐은 on_ms(lvl) 에 대한 비율). lvl<0 이면 g_crt_lvl */
    if (lvl < 0) lvl = g_crt_lvl; if (on && lvl > 0) { crtfx::on_frame(o, p, lvl > 3 ? 3 : lvl); return; }   /* #S3d */
    const int W = 640, H = 400, cy = H / 2; for (int i = 0; i < W * H; i++) o[i] = 0xFF000000u;
    auto ease = [](float q) { q = q < 0 ? 0 : q > 1 ? 1 : q; return q * q * (3 - 2 * q); };
    const float GR = 205.0f, GG = 255.0f, GB = 215.0f;   /* 인광 흰초록 */
    if (on) {   /* 검은 화면 → 가로 밝은 줄이 양옆으로 퍼짐 → 위아래로 번지며 살짝 과밝았다가 가라앉음 */
        if (p < 0.4f) { const float q = p / 0.4f; const int hw = (int)(ease(q) * 320.0f); const float k = 0.55f + 0.45f * q;
            for (int dy = -3; dy <= 2; dy++) { const float kk = (dy == -1 || dy == 0) ? k : (dy == -2 || dy == 1) ? k * 0.35f : k * 0.12f; const int y = cy + dy; for (int x = 320 - hw; x < 320 + hw; x++) o[y * W + x] = gh_crt_px(GR * kk, GG * kk, GB * kk); } }
        else { const float q = (p - 0.4f) / 0.6f; const int hh = (int)(ease(q) * 200.0f) + 2; const float br = (1.0f - q) * (1.0f - q) * 1.35f;
            for (int y = cy - hh; y < cy + hh; y++) { if (y < 0 || y >= H) continue; const float d = fabsf((float)(y - cy) + 0.5f) / (float)hh; const float k = br * (1.0f - 0.45f * d) + (1.0f - q) * 0.03f;
                for (int x = 0; x < W; x++) o[y * W + x] = gh_crt_px(GR * k, GG * k, GB * k); } } }
    else {   /* 화면이 위아래로 눌려 가로줄이 됨(밝아짐) → 줄이 양옆에서 줄어 점이 됨 → 사라짐 */
        if (p < 0.65f) { const float e = ease(p / 0.65f); int hh = (int)(200.0f * (1.0f - e)); if (hh < 1) hh = 1; const float br = 1.0f + 0.9f * e, mix = e * e * 0.6f;
            for (int y = cy - hh; y < cy + hh; y++) { int sy = cy + (int)(((float)(y - cy) + 0.5f) * 200.0f / (float)hh); if (sy < 0) sy = 0; if (sy >= H) sy = H - 1; if (y < 0 || y >= H) continue;
                for (int x = 0; x < W; x++) { const uint32_t v = src ? src[sy * W + x] : 0u; const float r = ((v >> 16) & 255u) * br, g = ((v >> 8) & 255u) * br, b = (v & 255u) * br; o[y * W + x] = gh_crt_px(r * (1 - mix) + GR * mix, g * (1 - mix) + GG * mix, b * (1 - mix) + GB * mix); } } }
        else { const float q = (p - 0.65f) / 0.35f; const float e = ease(q); int hw = (int)(320.0f * (1.0f - e)); if (hw < 3) hw = 3; const float k = q < 0.75f ? 1.0f : (1.0f - (q - 0.75f) / 0.25f);
            for (int dy = -2; dy <= 1; dy++) { const float kk = ((dy == -1 || dy == 0) ? 1.0f : 0.3f) * k; const int y = cy + dy; for (int x = 320 - hw; x < 320 + hw; x++) o[y * W + x] = gh_crt_px(GR * kk * 1.1f, GG * kk * 1.1f, GB * kk * 1.1f); } } }
}
static void gh_crt_run(bool on, const uint32_t * src) {   /* 막아서 돌리는 연출 (켜짐 0.52~1.0초 단계별 · 꺼짐 0.46초 + 소리 끝날 때까지 최대 1.4초) */
    static uint32_t ob[640 * 400]; const unsigned dur = on ? crtfx::on_ms(g_crt_lvl > 3 ? 3 : g_crt_lvl) : 460u, t0 = SDL_GetTicks();   /* #S3d: 켜짐 길이는 단계별(520→최대 1000 ms, 소리 1.5 s 안) */
    for (;;) { const unsigned t = SDL_GetTicks() - t0; if (t >= dur) break; SDL_Event e; while (SDL_PollEvent(&e)) { if (e.type == SDL_QUIT) { GH_QUITSET(); return; } }
        gh_crt_frame(ob, on, (float)t / (float)dur, src); gh_publish(ob, 640, 400); SDL_Delay(16); }
    if (!on) { for (int i = 0; i < 640 * 400; i++) ob[i] = 0xFF000000u; gh_publish(ob, 640, 400); while (g_crt_id && gau::playing(g_crt_id) && SDL_GetTicks() < g_crt_end && SDL_GetTicks() - t0 < 1400u) SDL_Delay(10); }
}
/* t789: 진동(안드로이드 전용) — android.ini 의 vibe=0 이면 끔(기본 켬). KhActivity.khVibrate(ms, amp) 호출 */
#ifdef __ANDROID__
#include <jni.h>
#endif
static int g_vibe_on = -1;
static void gh_vibe(int ms, int amp) {
#ifdef __ANDROID__
    if (gApp.headless) return;
    if (!gApp.ui_.g.vibe) return;   /* t794: 메뉴 설정 */
    if (g_vibe_on < 0) { g_vibe_on = 1; FILE * f = fopen((gApp.exedir + "\\android.ini").c_str(), "r"); if (f) { char ln[300]; while (fgets(ln, sizeof ln, f)) if (!strncmp(ln, "vibe=", 5)) g_vibe_on = atoi(ln + 5) ? 1 : 0; fclose(f); } gh_log("vibe: %s", g_vibe_on ? "on" : "off"); }
    if (!g_vibe_on) return;
    { const int pc = gApp.ui_.g.vibe_pct; if (pc != 100) { ms = ms * pc / 100; if (ms < 1) ms = 1; amp = amp * pc / 100; if (amp < 1) amp = 1; if (amp > 255) amp = 255; } }   /* t798: 진동 세기(settings.ini vibe_pct) */
    JNIEnv * env = (JNIEnv *)SDL_AndroidGetJNIEnv(); jobject act = (jobject)SDL_AndroidGetActivity(); if (!env || !act) return;
    jclass c = env->GetObjectClass(act); static jmethodID m = NULL; if (!m && c) m = env->GetMethodID(c, "khVibrate", "(II)V");   /* t797: ID 캐시 */
    if (m) env->CallVoidMethod(act, m, (jint)ms, (jint)amp); if (env->ExceptionCheck()) env->ExceptionClear();
    if (c) env->DeleteLocalRef(c); env->DeleteLocalRef(act);   /* t797: 지역 참조 반납 */
#else
    (void)ms; (void)amp;
#endif
}
static void gh_vibe_menu(void) { static unsigned last = 0; unsigned n = SDL_GetTicks(); if (n - last < 45u) return; last = n; gh_vibe(6, 70); }   /* t790: 메뉴 조작마다 미세 진동 */
static int g_gh_memext = 14336;   /* 확장 메모리 KB — 메모리 카운트·DOS 화면 공용 */
/* t790: PC-98 부팅 메모리 검사 화면(실기 사진 기준) — 「CPU MODE  High」 / 「MEMORY 640KB + NNNNNKB OK」: 확장 메모리가 올라가고 OK. 글자는 CRT 처럼 살짝 점멸.
   android.ini: memext=extended memory KB (default 14336 = 14MB). A faint vibration every 1MB while counting up. true = skipped (any input). */
static bool gh_memcount_seq(void) {
    if (gApp.headless || !gApp.use_gl) return false;
    static uint32_t buf[640 * 400]; ov::Canvas c = { buf, 640, 400 };
    int ext = g_gh_memext; { FILE * f = fopen((gApp.exedir + "\\android.ini").c_str(), "r"); if (f) { char ln[300]; while (fgets(ln, sizeof ln, f)) if (!strncmp(ln, "memext=", 7)) { int v = atoi(ln + 7); if (v >= 0 && v <= 1000000) ext = v; } fclose(f); } }
    g_gh_memext = ext; const unsigned t_l2 = 220u, t_cnt0 = 420u, t_cnt1 = 1300u, t_end = 1750u; const unsigned t0 = SDL_GetTicks(); int lastm = -1; bool skipped = false; unsigned fr = 0;
    for (;;) {
        const unsigned t = SDL_GetTicks() - t0; if (t >= t_end) break;
        SDL_Event e; while (SDL_PollEvent(&e)) { bool ak = false; kh_stk_ev(e); gh_ev2uk(e, &ak); if (e.type == SDL_QUIT) { GH_QUITSET(); skipped = true; } if (ak) skipped = true; }
        if (skipped) break;
        int kb = ext; if (t < t_cnt0) kb = 0; else if (t < t_cnt1) kb = (int)((double)ext * (double)(t - t_cnt0) / (double)(t_cnt1 - t_cnt0)) / 128 * 128;
        if (t >= t_cnt0 && kb / 1024 != lastm) { lastm = kb / 1024; if (kb > 0 && kb < ext) gh_vibe(8, 90); }
        fr++; unsigned h = fr * 2654435761u; h ^= h >> 13; const int v = ((h % 19u) == 0u) ? 175 : 228 + (int)((h >> 5) % 28u);   /* 살짝 점멸: 대부분 228~255, 가끔 어두워짐 */
        const uint32_t col = 0xFF000000u | ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v;
        c.rect(0, 0, 640, 400, ov::K);
        ov::text(c, 16, 16, std::string("CPU MODE  High"), col, ov::K);
        if (t >= t_l2) { char ln[80]; if (t < t_cnt0) snprintf(ln, sizeof ln, "MEMORY 640KB"); else if (t < t_cnt1) snprintf(ln, sizeof ln, "MEMORY 640KB + %dKB", kb); else snprintf(ln, sizeof ln, "MEMORY 640KB + %dKB OK", ext);
            ov::text(c, 16, 32, std::string(ln), col, ov::K); }
        gh_publish(buf, 640, 400); SDL_Delay(16);
    }
    if (!skipped) gh_vibe(20, 140);
    return skipped;
}
/* t798: 타이핑 연출 또각 — 메뉴 클릭음(pwr::mclk::play)과 같은 합성음. 메뉴 클릭음 설정·효과음 음량을 따른다(부팅 중에는 mclk 가 막혀 있어 따로 낸다) */
static void gh_key_tick(bool enter) {
    if (gApp.headless) return; const ui::Settings st = gApp.ui_.effective(gApp.cur_game); if (!st.mclick) return;
    const float g = st.se_gain() * 0.25f * 1.6f; if (g <= 0.0f) return;
    static uint32_t lcg = 0xE1C4u; lcg = lcg * 1664525u + 1013904223u; uint32_t sd = 0xE1C4u + (uint32_t)((lcg >> 16) % 3u) * 7919u; int32_t prev = 0;
    const size_t n = enter ? 88u : 55u; std::vector<int16_t> pcm(n, 0);
    for (size_t j = 0; j < n; j++) { sd = sd * 1664525u + 1013904223u; int32_t r = (int32_t)((sd >> 9) & 0xFFFFu) - 32768; int32_t v = r; if (enter) { v = (r + 3 * prev) / 4; prev = r; }
        float env = 1.0f - (float)j / (float)n; float y = (float)v * (1.0f / 32768.0f) * env * 0.35f * g; if (y > 1.0f) y = 1.0f; else if (y < -1.0f) y = -1.0f; pcm[j] = dsn::to16(y); }
    gau::play(pcm.data(), pcm.size(), dsn::RATE);
}
/* true = 건너뜀. 아무 버튼이나 누르면 건너뛴다. */
/* t790: 부팅 「길게」 — NEC PC-9800 / MS-DOS 6.20 / VEM486 화면(실기 사진 기준)이 차례로 뜨고, 프롬프트에 KYUHACHI 를 쳐 넣는다. 글자는 살짝 점멸. true = 건너뜀(아무 입력). */
static bool gh_dos_seq(void) {
    if (gApp.headless || !gApp.use_gl) return false;
    static uint32_t buf[640 * 400]; ov::Canvas c = { buf, 640, 400 };
    struct JpGuard { bool p; JpGuard() : p(ov::g_jp_on) { ov::g_jp_on = true; } ~JpGuard() { ov::g_jp_on = p; } } jpg_;   /* t798: 부팅 화면은 UI 언어와 무관하게 일본어 글꼴(가나·문장부호) — 끝나면 원래대로 */
    struct DL { int row; unsigned t; int cy; const char * s; };
    char vem[160]; snprintf(vem, sizeof vem, "VEM486 installed : XMS/EMS/UCPI \xE3\x83\xA1\xE3\x83\xA2\xE3\x83\xAA\xE3\x81\xA8\xE3\x81\x97\xE3\x81\xA6 %d KBytes \xE4\xBD\xBF\xE7\x94\xA8\xE5\x8F\xAF\xE8\x83\xBD\xE3\x81\xA7\xE3\x81\x99\xE3\x80\x82", g_gh_memext);
    const DL ls[] = {
        { 1, 200, 0, "NEC PC-9800 \xE3\x82\xB7\xE3\x83\xAA\xE3\x83\xBC\xE3\x82\xBA \xE3\x83\x91\xE3\x83\xBC\xE3\x82\xBD\xE3\x83\x8A\xE3\x83\xAB \xE3\x82\xB3\xE3\x83\xB3\xE3\x83\x94\xE3\x83\xA5\xE3\x83\xBC\xE3\x82\xBF" },
        { 3, 500, 0, "\xE3\x83\x9E\xE3\x82\xA4\xE3\x82\xAF\xE3\x83\xAD\xE3\x82\xBD\xE3\x83\x95\xE3\x83\x88 MS-DOS \xE3\x83\x90\xE3\x83\xBC\xE3\x82\xB8\xE3\x83\xA7\xE3\x83\xB3 6.20" },
        { 4, 560, 0, "Copyright (C) 1981,1994 Microsoft Corp. / NEC Corporation" },
        { 5, 620, 0, "High Speed Boot controller for 98  Ver 3.7   Copyright (c) 1991-95 Masao" },
        { 7, 1300, 1, "VEM486 Virtual XMS / EMS / UCPI Memory Server   Version 1.31b13" },
        { 8, 1380, 1, "Copyright (C) 1995-1996 K.Ogino" },
        { 9, 1700, 0, "UMB\xE9\xA0\x98\xE5\x9F\x9F    : A5-A7 D0-D7 DC-DF E8-EF" },
        { 10, 1760, 0, "\xE7\x89\xB9\xE6\xAE\x8A\xE6\xA9\x9F\xE8\x83\xBD   : TRAP-FM" },
        { 11, 2100, 0, vem },
        { 14, 2600, 0, "\xE6\x97\xA2\xE3\x81\xABVEMEMM\xE3\x81\x8C\xE7\xB5\x84\xE3\x81\xBF\xE8\xBE\xBC\xE3\x81\xBE\xE3\x82\x8C\xE3\x81\xA6\xE3\x81\x84\xE3\x81\xBE\xE3\x81\x99\xE3\x80\x82" },
        { 16, 3000, 0, "FFFF:07CC-00" },
        { 17, 3000, 0, "High Speed Boot controller for 98  Ver 3.7   Copyright (c) 1991-95 Masao" },
    };
    const unsigned t_type0 = 3700u, t_ch = 95u, t_enter = t_type0 + 8u * t_ch + 350u, t_end = t_enter + 420u; const char * cmd = "KYUHACHI";
    const unsigned t0 = SDL_GetTicks(); bool skipped = false; unsigned fr = 0; int typed = 0; bool f_a = false, f_b = false, f_c = false, f_d = false;
    for (;;) {
        const unsigned t = SDL_GetTicks() - t0; if (t >= t_end) break;
        SDL_Event e; while (SDL_PollEvent(&e)) { bool ak = false; kh_stk_ev(e); gh_ev2uk(e, &ak); if (e.type == SDL_QUIT) { GH_QUITSET(); skipped = true; } if (ak) skipped = true; }
        if (skipped) break;
        fr++; unsigned h = fr * 2654435761u; h ^= h >> 13; const int v = ((h % 23u) == 0u) ? 180 : 232 + (int)((h >> 5) % 24u);
        const uint32_t wh = 0xFF000000u | ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v, cy = 0xFF000000u | ((uint32_t)(v / 6) << 16) | ((uint32_t)v << 8) | (uint32_t)v;
        if (!f_a && t >= 250u) { f_a = true; gh_fdd_sfx("fdd_seek2.wav"); }
        if (!f_b && t >= 700u) { f_b = true; gh_fdd_sfx("fdd_read.wav"); }
        if (!f_c && t >= 2300u) { f_c = true; gh_fdd_sfx("fdd_seek1.wav"); }
        if (!f_d && t >= t_enter) { f_d = true; gh_fdd_sfx("fdd_read.wav"); }
        c.rect(0, 0, 640, 400, ov::K);
        for (const DL & l : ls) if (t >= l.t) ov::text(c, 8, l.row * 16, std::string(l.s), l.cy ? cy : wh, ov::K);
        if (t >= 3000u && t < t_type0 && ((t / 380u) & 1u) == 0u) c.rect(8 + 12 * 8, 16 * 16, 8, 16, wh);
        if (t >= t_type0) {
            int n = (int)((t - t_type0) / t_ch); if (n > 8) n = 8; if (n > typed) { typed = n; gh_vibe(4, 60); gh_key_tick(false); }
            std::string pr = std::string("A>") + std::string(cmd, (size_t)n); ov::text(c, 8, 19 * 16, pr, wh, ov::K);
            if (n < 8 || t < t_enter) { if (((t / 300u) & 1u) == 0u) c.rect(8 + (int)pr.size() * 8, 19 * 16, 8, 16, wh); }
            if (t >= t_enter && typed == 8) { static bool ent = false; if (!ent) { ent = true; gh_vibe(10, 100); gh_key_tick(true); } }
        }
        gh_publish(buf, 640, 400); SDL_Delay(16);
    }
    return skipped;
}
/* t803: 검은 화면 구간 반복음(영상 34.2~38.2s) — 473Hz 70ms 사각파, 삑의 약 1/4 음량 */
static void gh_tick473(void) {
    if (gApp.headless) return; const float sg = gApp.ui_.effective(gApp.cur_game).se_gain(); const int amp = (int)(700.0f * sg); if (amp <= 0) return;
    std::vector<int16_t> pcm; const int R = 44100; const int n = R * 70 / 1000; for (int i = 0; i < n; i++) { double ph = fmod(i * 473.0 / R, 1.0); pcm.push_back((int16_t)(ph < 0.5 ? amp : -amp)); }
    pcm.insert(pcm.end(), (size_t)(R * 10 / 1000), (int16_t)0); gau::play(pcm.data(), pcm.size(), R);
}
/* t800: 부팅 연출(PC-98 부팅 영상 기준) 1/2 — 삑 → MEMORY 카운트 → 고정 → 검은 화면. 시각은 삑이 0. true = 건너뜀(아무 입력). */
static bool gh_memcount_seq2(void) {
    if (gApp.headless || !gApp.use_gl) return false;
    static uint32_t buf[640 * 400]; ov::Canvas c = { buf, 640, 400 };
    int ext = g_gh_memext; { FILE * f = fopen((gApp.exedir + "\\android.ini").c_str(), "r"); if (f) { char ln[300]; while (fgets(ln, sizeof ln, f)) if (!strncmp(ln, "memext=", 7)) { int v = atoi(ln + 7); if (v >= 0 && v <= 1000000) ext = v; } fclose(f); } }
    g_gh_memext = ext; const unsigned t_cnt0 = 860u, t_cnt1 = 1260u, t_off = 4150u, t_end = 10000u; const unsigned t0 = SDL_GetTicks(); bool skipped = false, beeped = false, vib = false, clk = false; unsigned fr = 0; int tk = 0;
    for (;;) {
        const unsigned t = SDL_GetTicks() - t0; if (t >= t_end) break;
        SDL_Event e; while (SDL_PollEvent(&e)) { bool ak = false; kh_stk_ev(e); gh_ev2uk(e, &ak); if (e.type == SDL_QUIT) { GH_QUITSET(); skipped = true; } if (ak) skipped = true; }
        if (skipped) break;
        /* t804: 반복음 제거 */
        if (!clk && t >= 4280u) { clk = true; gh_crt_sfx("boot_click.wav"); }   /* t801: MEMORY 줄 꺼진 뒤 딸깍(영상 32.734s) */
        if (!beeped) { beeped = true; if (!gApp.headless) { const float sg = gApp.ui_.effective(gApp.cur_game).se_gain(); int amp = (int)(3000.0f * sg); if (amp > 0) { std::vector<int16_t> pcm; const int R = 44100; const double f = 1000.0; const int n = R * 100 / 1000; for (int i = 0; i < n; i++) { double ph = fmod(i * f / R, 1.0); pcm.push_back((int16_t)(ph < 0.5 ? amp : -amp)); } pcm.insert(pcm.end(), (size_t)(R * 60 / 1000), (int16_t)0); g_beep_id = gau::play(pcm.data(), pcm.size(), R); } } }
        c.rect(0, 0, 640, 400, ov::K);
        if (t >= t_cnt0 && t < t_off) {
            int kb = ext; if (t < t_cnt1) kb = (int)((double)ext * (double)(t - t_cnt0) / (double)(t_cnt1 - t_cnt0)) / 128 * 128;
            if (t >= t_cnt1 && !vib) { vib = true; gh_vibe(20, 140); }
            fr++; unsigned h = fr * 2654435761u; h ^= h >> 13; const int v = ((h % 19u) == 0u) ? 175 : 228 + (int)((h >> 5) % 28u);
            const uint32_t col = 0xFF000000u | ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v;
            char ln[80]; if (t < t_cnt1) snprintf(ln, sizeof ln, "MEMORY 640KB + %dKB", kb); else snprintf(ln, sizeof ln, "MEMORY 640KB + %dKB OK", ext);
            ov::text(c, 16, 16, std::string(ln), col, ov::K);
        }
        gh_publish(buf, 640, 400); SDL_Delay(16);
    }
    return skipped;
}
/* t804: 시크음 앞 딸깍 — 영상 파형 기준 22ms, 1.9kHz 감쇠 사인+잡음. 플로피 효과음 설정(dsnd)·효과음 음량을 따른다 */
static void gh_seek_click(void) {
    if (gApp.headless) return; const ui::Settings st = gApp.ui_.effective(gApp.cur_game); if (st.dsnd == 0) return; const float g = st.se_gain() * 0.9f; if (g <= 0.0f) return;
    static uint32_t sd = 0x5EEC1u; const int R = dsn::RATE; const int n = R * 22 / 1000; std::vector<int16_t> pcm((size_t)n, 0);
    for (int j = 0; j < n; j++) { sd = sd * 1664525u + 1013904223u; const float nz = (float)((int32_t)((sd >> 9) & 0xFFFFu) - 32768) / 32768.0f; const float env = expf(-(float)j / (float)(R * 0.006)); const float y0 = (0.55f * sinf(6.2831853f * 1900.0f * (float)j / (float)R) + 0.45f * nz) * env * 0.35f * g; const float y = y0 > 1.0f ? 1.0f : (y0 < -1.0f ? -1.0f : y0); pcm[(size_t)j] = dsn::to16(y); }
    gau::play(pcm.data(), pcm.size(), R);
}
/* t800: 2/2 — 펑션키 줄 + NEC/MS-DOS 안내(+커서) → XMS 두 줄(플로피음) → 검은 0.5초. 시각은 이 단계 시작이 0. true = 건너뜀. */
static bool gh_dos_seq2(void) {
    if (gApp.headless || !gApp.use_gl) return false;
    static uint32_t buf[640 * 400]; ov::Canvas c = { buf, 640, 400 };
    struct JpGuard { bool p; JpGuard() : p(ov::g_jp_on) { ov::g_jp_on = true; } ~JpGuard() { ov::g_jp_on = p; } } jpg_;
    char x2[160]; snprintf(x2, sizeof x2, "%s %05d %s", "\xE7\xA2\xBA\xE4\xBF\x9D\xE3\x81\x97\xE3\x81\x9F\xE6\x8B\xA1\xE5\xBC\xB5\xE3\x83\xA1\xE3\x83\xA2\xE3\x83\xAA\xE5\xAE\xB9\xE9\x87\x8F\xE3\x81\xAF", g_gh_memext, "KB " "\xE3\x81\xA7\xE3\x81\x99");
    static const char * const FK[10] = { "CL", "CU", "CR", "SI", "SU", "VOID", "NWL", "INS", "REP", "" };
    const unsigned t_txt = 70u, t_xms = 4200u, t_scr = 8180u, t_end = 8680u, t_type0 = 4900u, t_ch = 95u, t_enter = t_type0 + 8u * t_ch + 350u;
    const unsigned t0 = SDL_GetTicks(); bool skipped = false; unsigned fr = 0; bool f1 = false, f2 = false, f3 = false, f4 = false, f5 = false, f6 = false, ent = false, k1 = false, k3 = false, k5 = false; int typed = 0;
    for (;;) {
        const unsigned t = SDL_GetTicks() - t0; if (t >= t_end) break;
        SDL_Event e; while (SDL_PollEvent(&e)) { bool ak = false; kh_stk_ev(e); gh_ev2uk(e, &ak); if (e.type == SDL_QUIT) { GH_QUITSET(); skipped = true; } if (ak) skipped = true; }
        if (skipped) break;
        if (!k1 && t >= 545u) { k1 = true; gh_seek_click(); }
        if (!k3 && t >= 3345u) { k3 = true; gh_seek_click(); }
        if (!k5 && t >= 6045u) { k5 = true; gh_seek_click(); }
        if (!f1 && t >= 570u) { f1 = true; gh_fdd_sfx("fdd_seek2.wav"); }
        if (!f2 && t >= 2270u) { f2 = true; gh_fdd_sfx("fdd_read.wav"); }
        if (!f3 && t >= 3370u) { f3 = true; gh_fdd_sfx("fdd_seek1.wav"); }
        if (!f4 && t >= 4270u) { f4 = true; gh_fdd_sfx("fdd_read.wav"); }
        if (!f5 && t >= 6070u) { f5 = true; gh_fdd_sfx("fdd_seek2.wav"); }
        if (!f6 && t >= 7170u) { f6 = true; gh_fdd_sfx("fdd_read.wav"); }
        c.rect(0, 0, 640, 400, ov::K);
        if (t < t_scr) {
            fr++; unsigned h = fr * 2654435761u; h ^= h >> 13; const int v = ((h % 23u) == 0u) ? 180 : 232 + (int)((h >> 5) % 24u);
            const uint32_t wh = 0xFF000000u | ((uint32_t)v << 16) | ((uint32_t)v << 8) | (uint32_t)v;
            const uint32_t bx = 0xFFE6D4E6u, bt = 0xFF303030u;
            for (int i = 0; i < 10; i++) { const int x = 48 + (i < 5 ? i * 48 : 280 + (i - 5) * 48); c.rect(x, 384, 40, 16, bx); if (FK[i][0]) ov::text(c, x + 20 - (int)strlen(FK[i]) * 4, 384, std::string(FK[i]), bt, bx); }
            if (t >= t_txt) {
                ov::text(c, 8, 16, std::string("NEC PC-9800 " "\xE3\x82\xB7\xE3\x83\xAA\xE3\x83\xBC\xE3\x82\xBA" " " "\xE3\x83\x91\xE3\x83\xBC\xE3\x82\xBD\xE3\x83\x8A\xE3\x83\xAB" " " "\xE3\x82\xB3\xE3\x83\xB3\xE3\x83\x94\xE3\x83\xA5\xE3\x83\xBC\xE3\x82\xBF"), wh, ov::K);
                ov::text(c, 8, 48, std::string("\xE3\x83\x9E\xE3\x82\xA4\xE3\x82\xAF\xE3\x83\xAD\xE3\x82\xBD\xE3\x83\x95\xE3\x83\x88" " MS-DOS " "\xE3\x83\x90\xE3\x83\xBC\xE3\x82\xB8\xE3\x83\xA7\xE3\x83\xB3" " 6.20"), wh, ov::K);
                ov::text(c, 8, 64, std::string("Copyright (C) 1981,1994 Microsoft Corp. / NEC Corporation"), wh, ov::K);
                int cr = 5; if (t >= t_xms) { ov::text(c, 24, 6 * 16, std::string("HIMEM: XMS" "\xE3\x83\x87\xE3\x83\x90\xE3\x82\xA4\xE3\x82\xB9\xE3\x83\x89\xE3\x83\xA9\xE3\x82\xA4\xE3\x83\x90" " Ver 2.03W " "\xE3\x81\x8C\xE4\xBD\xBF\xE7\x94\xA8\xE5\x8F\xAF\xE8\x83\xBD\xE3\x81\xA7\xE3\x81\x99"), wh, ov::K); ov::text(c, 24, 7 * 16, std::string(x2), wh, ov::K); cr = 8; }
                if (t >= t_type0) { int n = (int)((t - t_type0) / t_ch); if (n > 8) n = 8; if (n > typed) { typed = n; gh_vibe(4, 60); gh_key_tick(false); } std::string pr = std::string("A>") + std::string("KYUHACHI", (size_t)n); ov::text(c, 8, cr * 16, pr, wh, ov::K); if (t >= t_enter) { if (!ent) { ent = true; gh_vibe(10, 100); gh_key_tick(true); } } else if (((t / 300u) & 1u) == 0u) c.rect(8 + (int)pr.size() * 8, cr * 16, 8, 16, wh); } else if (((t / 380u) & 1u) == 0u) c.rect(8, cr * 16, 8, 16, wh);   /* t802: KYUHACHI 타이핑 */
            }
        }
        gh_publish(buf, 640, 400); SDL_Delay(16);
    }
    return skipped;
}
static bool gh_boot_seq(bool full) {
    if (gApp.headless || !gApp.use_gl) return false;
    bool kh_post_beeped = false;   /* t800: 새 부팅 연출이 이미 삑을 냈으면 로고 단계 삑 생략 */
    if (full && gh_crt_enabled()) { gh_crt_sfx("crt_on.wav"); gh_vibe(35, 160); gh_crt_run(true, NULL); if (gApp.ui_.effective(gApp.cur_game).pwrseq >= 2) { static const bool KH_BOOT_OLD = false; if (KH_BOOT_OLD) { if (gh_memcount_seq()) return true; if (gh_dos_seq()) return true; } else { kh_post_beeped = true; if (gh_memcount_seq2()) return true; if (gh_dos_seq2()) return true; } } }   /* v0.6.8: 켜짐 연출 먼저, 그다음 로고·부팅 */
    static const bool KH_LOGO_ON = false;    /* t601 (#550·#551): 시작 로고 그리기 기본 끔(코드·그림은 남김). t148 개명 때 새 로고와 함께 true 로 */
    if (full && KH_LOGO_ON) {   /* t52d (#129): 시작 로고 1.4초 — 아무 키·클릭이면 부팅 연출 전체를 건너뜀 */
        unsigned tl = SDL_GetTicks(); bool sk = false;
        for (;;) { if (SDL_GetTicks() - tl >= 1400u) break; SDL_Event e; while (SDL_PollEvent(&e)) { bool ak = false; kh_stk_ev(e); gh_ev2uk(e, &ak); if (e.type == SDL_QUIT) { GH_QUITSET(); sk = true; } if (ak) sk = true; } if (sk) return true; gh_logo_frame(); SDL_Delay(16); }
    }
    unsigned t0 = SDL_GetTicks(); bool beeped = kh_post_beeped; bool skipped = false; unsigned total = full ? 1800u : 500u;
    for (;;) {
        unsigned t = SDL_GetTicks() - t0; if (t >= total) break;
        SDL_Event e; while (SDL_PollEvent(&e)) { bool ak = false; kh_stk_ev(e); gh_ev2uk(e, &ak); if (e.type == SDL_QUIT) { GH_QUITSET(); skipped = true; } if (ak) skipped = true; }
        if (skipped) break;
        if (full) {
            if (t >= 300 && !beeped) { beeped = true; gh_beep(gApp.ui_.effective(gApp.cur_game).se_gain()); }
            int stage = t < 300 ? 0 : t < 700 ? 1 : t < 1000 ? 2 : t < 1300 ? 3 : 4; gh_boot_frame(stage, t >= 1300);
        } else gh_boot_frame(t < 150 ? 1 : 4, false);
        SDL_Delay(16);
    }
    if (g_beep_id && skipped) gau::stop(g_beep_id);   /* #B4 */
    if (full && beeped && !skipped) SDL_Delay(0);
    gh_beep_close();
    return skipped;
}
static void gh_spawn_menu(void) {
    std::string exe = plat::exe_path();
    std::string cmd = "\"" + exe + "\" --noboot"; unsigned long perr = 0;
    if (plat::spawn(exe, cmd, gApp.exedir, &perr)) { gh_log("spawn menu OK"); }
    else gh_log("spawn menu FAIL err=%lu", (unsigned long)perr);
}
/* ---- t49a (#845): 실행 순서(단계) — 항목의 next= 사슬. 「처음부터」 는 사슬의 첫 단계부터, 「이어하기」 는 지금 고른(마지막) 단계로.
   다음 단계로는 아무 입력(키·클릭·패드) 또는 일시정지 메뉴 「다음 단계」 — 프로세스를 새로 띄워(--entry <다음>) 디스크 구성을 바꾼다. 게임 안 화면 감지·자동 전환 없음 */
static const ui::UiEnt * gh_find_ent(int gi, const std::string & name) { if (gi < 0 || gi >= (int)gApp.ui_.games.size() || name.empty()) return NULL; for (auto & e : gApp.ui_.games[gi].ents) if (e.name == name && e.vgi < 0 && !e.pseudo) return &e; return NULL; }
static std::string gh_ent_next(int gi, const std::string & name) {   /* 다음 단계 이름("-" = 기본 실행). 없거나 가리키는 항목이 없으면 빈 문자열 */
    const ui::UiEnt * e = gh_find_ent(gi, name); if (!e) return std::string(); auto it = e->kv.find("next"); if (it == e->kv.end() || it->second.empty() || it->second == "none") return std::string();
    if (it->second == "-" || gh_find_ent(gi, it->second)) return it->second; return std::string(); }
static std::string gh_stage_label(int gi, const std::string & nx) { if (nx == "-") return phr_("게임"); const ui::UiEnt * e = gh_find_ent(gi, nx); return e ? (e->label.empty() ? e->name : e->label) : nx; }
static std::string gh_chain_head(int gi, const std::string & req) {   /* req(고른 단계)로 이어지는 사슬의 맨 앞 단계 이름. 앞선 단계가 없으면 req 그대로 */
    if (gi < 0 || gi >= (int)gApp.ui_.games.size()) return req; const ui::UiGame & g = gApp.ui_.games[gi];
    std::set<std::string> T; if (req.empty() || req == "-") { T.insert("-"); std::string d = g.defent; if (d.empty()) for (auto & e : g.ents) if (!e.pseudo && !e.hidden && e.vgi < 0) { d = e.name; break; } if (!d.empty()) T.insert(d); } else T.insert(req);
    std::string head = req; std::set<std::string> seen;
    for (int depth = 0; depth < 8; depth++) {
        std::string found; for (auto & e : g.ents) { if (e.pseudo || e.vgi >= 0 || e.name.empty() || seen.count(e.name) || T.count(e.name)) continue; std::string nx = gh_ent_next(gi, e.name); if (!nx.empty() && T.count(nx)) { found = e.name; break; } }
        if (found.empty()) break; seen.insert(found); head = found; T.clear(); T.insert(found);
    }
    return head; }
static void gh_chain_start(ui::Ui & U, int gi) {   /* 메뉴에서 「처음부터」(g_resume_slot == 0) 로 시작할 때 사슬의 첫 단계로 바꾼다 */
    if (gApp.nostage || gApp.resume_slot != 0) return; std::string h = gh_chain_head(gi, prof::g_entry_req); if (h != prof::g_entry_req) { gh_log("STAGE 처음부터: %s -> 첫 단계 %s", prof::g_entry_req.c_str(), h.c_str()); prof::g_entry_req = h; } (void)U; }
static void gh_stage_init(void) {   /* 게임 시작 때: 지금 단계의 다음 단계가 있으면 안내·메뉴 항목을 켠다 */
    gApp.stage_next.clear(); gApp.ui_.stage_next_label.clear(); if (gApp.nostage) return;
    std::string nm = (!prof::g_entry_req.empty() && prof::g_entry_req != "-") ? prof::g_entry_req : std::string(); if (nm.empty()) { auto it = gApp.prof_.kv.find("entry_name"); if (it != gApp.prof_.kv.end()) nm = it->second; }
    gApp.stage_entry = nm;
    std::string nx = gh_ent_next(gApp.cur_game, nm); if (nx.empty()) { gh_log("STAGE entry='%s' next=(없음)", nm.c_str()); return; }
    gApp.stage_next = nx; const ui::UiEnt * ce = gh_find_ent(gApp.cur_game, nm); std::string nl = phr_("다음 단계로 넘어가기"); if (ce) { auto li = ce->kv.find("next_label"); if (li != ce->kv.end() && !li->second.empty()) nl = li->second; } /* 속성 「다음 단계로」: 메뉴 이름 = next_label (확장 자리: 속성 종류를 더하려면 여기에 항목을 추가) */
    gApp.ui_.stage_next_label = nl; gh_log("STAGE entry='%s' next='%s' (%s)", nm.c_str(), nx.c_str(), nl.c_str()); }
static void gh_spawn_stage(const std::string & entry) {   /* 같은 실행 인자에서 --demo/--entry 를 빼고 --entry <다음> 을 붙여 새 프로세스로 */
    std::string exe = plat::exe_path(); std::string cmd = "\"" + exe + "\"";
    for (size_t i = 1; i < gApp.argv_saved.size(); i++) { const std::string & a = gApp.argv_saved[i]; if (a == "--demo") continue; if (a == "--entry") { i++; continue; } cmd += " \"" + a + "\""; }
    if (!gApp.id_given) { cmd += " \""; cmd += (gApp.cur_game >= 0 && gApp.cur_game < (int)gApp.ui_.games.size()) ? gApp.ui_.games[gApp.cur_game].id : std::string(gApp.prof_.id ? gApp.prof_.id : ""); cmd += "\" --noboot"; }
    cmd += " --entry \"" + aw::u8_to_acp(entry) + "\"";
    unsigned long perr = 0;
    if (plat::spawn(exe, cmd, gApp.exedir, &perr)) { gh_log("STAGE 다음 단계 실행: %s", cmd.c_str()); }
    else gh_log("STAGE spawn FAIL err=%lu", (unsigned long)perr);
}
static void gh_sendkey(int arg) {
    static const KBD_KEYS K9[9] = { KBD_f10, KBD_1, KBD_2, KBD_3, KBD_4, KBD_5, KBD_6, KBD_7, KBD_kana };
    if (arg < 0 || arg > 8) return;
    if ((int)K9[arg] < 256 && kh_never[(int)K9[arg]]) { gh_log("sendkey #%d blocked (never_map)", arg); gApp.osd = phr_("이 작품에서는 보낼 수 없는 키입니다"); gApp.osd_until = SDL_GetTicks() + 2000; return; }
    gApp.key_down = K9[arg]; gApp.key_release_at = SDL_GetTicks() + 100; KEYBOARD_AddKey(gApp.key_down, true);
    gh_log("sendkey #%d", arg);
}
static bool gh_play_commit(void) { unsigned s = gApp.play_ms / 1000; if (!s || gApp.cur_game < 0 || gApp.cur_game >= (int)gApp.ui_.games.size()) return false; gApp.ui_.games[gApp.cur_game].play_s += (int)s; gApp.play_ms -= s * 1000; return true; }
/* ---- 게임 중 메뉴(에뮬 스레드에서 도는 중첩 루프 = 게임이 멈춘다) */
static void gh_bk_pause_set(bool on, const char * why) {   /* t47a (#559): 자동 입력 타이머만 멈춘다(게임은 계속 돈다) */
    if (on) { if (gApp.prof_.bootkeys.empty()) { gh_osd(phr_("자동 입력 없음"), 1500); return; } if (gApp.next_bk >= gApp.prof_.bootkeys.size()) { gh_osd(phr_("자동 입력이 이미 끝났습니다"), 1500); return; } }
    if (gApp.bk_paused == on) return;
    gApp.bk_paused = on; gh_log("BKPAUSE %s (%s) bk_t=%u next=#%u wall=%u", on ? "정지" : "재개", why, gApp.bk_t, (unsigned)gApp.next_bk, gApp.boot_tick ? SDL_GetTicks() - gApp.boot_tick : 0u);
    gh_osd(on ? phr_("자동 입력 일시정지 (Alt+P 로 재개)") : phr_("자동 입력 재개"), 1500);
}
#include "menu_sq.h"
static void gh_menu_run(void) {

    if (gApp.headless || !gApp.use_gl) return;
    kc::presave = gh_autosave;   /* kx-cheat: auto slot is saved before a cheat write */
    kbov::release_all(); gApp.ui_.kb_vis = kbov::g_vis ? 1 : 0; gApp.ui_.kb_btn = kbov::g_btn_on ? 1 : 0; gApp.ui_.kb_sh = gApp.kbshader ? 1 : 0; gApp.menu_open = true; gh_boost_update();
    static uint32_t snap[640 * 400], buf[640 * 400], dimb[640 * 400]; ov::Canvas sc = { snap, 640, 400 }, c = { buf, 640, 400 };
    sc.rect(0, 0, 640, 400, ov::K); if (!gApp.fb.empty()) ov::blit_scaled(sc, gApp.fb.data(), gApp.fbw, gApp.fbh, 0, 0, 640, 400); msq::dim_of(dimb, snap);
    ui::Ui & U = gApp.ui_; U.in_game = true; U.cur = gApp.cur_game; U.scr = ui::S_GAME; U.sel = 0;
    gh_refresh_game_slots(gApp.cur_game); U.slotsel = U.games[gApp.cur_game].lastslot >= 0 ? U.games[gApp.cur_game].lastslot : 0;
    bool done = false; bool sendk = false; int sendarg = 0; g_rep_k = ui::UK_NONE;
    gh_diag_enter(1);
    const unsigned mo_t0 = SDL_GetTicks(); unsigned mo_fr = 0;   /* t797: 메뉴 열기 계측 */
    while (!done && !gApp.quit) {
        const unsigned mo_f0 = SDL_GetTicks();
        gh_inj_poll_menu();   /* t49a: 하니스 inject 를 일시정지 메뉴에서도 */
        gh_pad_rep();   /* v0.6.8: 패드 누르고 있기 반복 */
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (gh_evpre(e, 1)) continue;
            if (e.type == SDL_QUIT) { GH_QUITSET(); done = true; break; }
            if (gh_wm_hotkey(e)) continue;
#if defined(__ANDROID__)
            if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION || e.type == SDL_FINGERUP) { if (e.type == SDL_FINGERDOWN) gh_vibe_menu(); gh_touch_feed(e); continue; }   /* t55a: menu touch */
#endif
            ui::UKey k = gh_ev2uk(e, NULL); if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) gh_log("DIAG btn -> ev2uk=%d none=%d", (int)k, k == ui::UK_NONE ? 1 : 0); if (k == ui::UK_NONE) continue; if (e.type != SDL_KEYUP && e.type != SDL_CONTROLLERBUTTONUP && e.type != SDL_JOYBUTTONUP && e.type != SDL_MOUSEBUTTONUP) gh_vibe_menu();
            ui::Act a = U.key(k);
            switch (a.t) {
            case ui::A_RESUME: done = true; break;
            case ui::A_NEXTSTAGE: if (!gApp.stage_next.empty()) { gh_log("STAGE F12 menu '%s' -> %s", U.stage_next_label.c_str(), gApp.stage_next.c_str()); if (gApp.inwin_enabled && !gApp.headless && gApp.use_gl) { gApp.inwin_req = 1; gApp.inwin_target = gApp.stage_next; } else gApp.stage_go = true; GH_QUITSET(); done = true; } break;   /* t49a (#847) · t51a: 창 안 전환 먼저, 안 되면 gh_inwin_cycle 이 g_stage_go 로 폴백 */
            case ui::A_SAVE: { std::string d = gApp.savedir + "\\" + gApp.prof_.id; plat::mkdir(gApp.savedir.c_str()); plat::mkdir(d.c_str());
                bool ok = gh_do_save(gh_slot_n(gApp.cur_game, a.slot), false); gh_osd(ok ? phr_("슬롯 ") + ui::Ui::slot_lbl(a.slot) + phr_(" 에 저장했습니다") : std::string(phr_("저장에 실패했습니다")), 2000); done = true; break; }
            case ui::A_LOAD: { if (!U.games[gApp.cur_game].slot[a.slot].used) { gh_osd(phr_("비어 있는 슬롯입니다"), 1500); break; }
                bool ok = gh_do_load(gh_slot_n(gApp.cur_game, a.slot)); gh_osd(ok ? phr_("슬롯 ") + ui::Ui::slot_lbl(a.slot) + phr_(" 을 불러왔습니다") : std::string(phr_("불러오기에 실패했습니다")), 2000); done = true; break; }
            case ui::A_DELSLOT: gh_delete_slot(gApp.cur_game, a.slot); break;
            case ui::A_APPLY: gh_apply_settings(U.eff(), true); kh_western = U.eff().pad == 1; gh_apply_profile_input(gApp.prof_, true); break;   /* t48e (#646): 조작 설정 변경을 바로 입력 층에 */
            case ui::A_SHOT: gApp.scr_req = true; break;   /* t41: 메뉴가 닫힌 뒤 다음 프레임에서 찍는다 */
            case ui::A_WINMULT: gh_win_apply(a.arg, false); break;
            case ui::A_FULLSCR: gh_fs_toggle(); break;
            case ui::A_KBD: kbov::set_visible(!kbov::g_vis); gh_kb_cursor(); gh_osd(kbov::g_vis ? phr_("화면 키보드 켜짐 (Alt+K 로 끔)") : phr_("화면 키보드 꺼짐"), 2000); done = true; break;
            case ui::A_KBSH: { int v = gApp.kbshader ? 0 : 1; gApp.kbshader = v; U.g.kbshader = v; if (gApp.cur_game >= 0 && gApp.cur_game < (int)ui::Ui::MAXG && U.has_t[gApp.cur_game]) U.t[gApp.cur_game].kbshader = v; U.kb_sh = v; U.dirty = true; gh_osd(v ? phr_("키보드·배속 표시에 셰이더를 적용합니다") : phr_("키보드·배속 표시를 셰이더 없이 그립니다"), 2500); break; }
            case ui::A_KBBTN: {
 int v = kbov::g_btn_on ? 0 : 1; kbov::g_btn_on = v != 0; U.g.kbbtn = v; if (gApp.cur_game >= 0 && gApp.cur_game < (int)ui::Ui::MAXG && U.has_t[gApp.cur_game]) U.t[gApp.cur_game].kbbtn = v; U.kb_btn = v; U.dirty = true; kbov::g_dirty = true; gh_osd(v ? phr_("키보드 버튼을 표시합니다") : phr_("키보드 버튼을 숨깁니다 (Alt+K · 메뉴로 키보드 켜기)"), 2500); break; }
            case ui::A_SENDKEY: sendk = true; sendarg = a.arg; done = true; break;
            case ui::A_DISKSWAP: { auto fit_ = gApp.fdfiles.find(U.games[gApp.cur_game].id); bool ok = false;
                if (fit_ != gApp.fdfiles.end() && a.arg >= 0 && a.arg < (int)fit_->second.size() && a.slot >= 0 && a.slot < 2) { ok = gh_attach(a.slot, fit_->second[a.arg].c_str(), true); if (ok) U.games[gApp.cur_game].cur_fd[a.slot] = U.games[gApp.cur_game].fds[a.arg]; }
                gh_log("DISKSWAP drive=%c idx=%d %s", a.slot ? 'B' : 'A', a.arg, ok ? "OK" : "FAIL");
                gh_osd(ok ? std::string(a.slot ? "B" : "A") + phr_(" 드라이브 : ") + U.games[gApp.cur_game].cur_fd[a.slot] : std::string(phr_("디스크 교체에 실패했습니다")), 3000); done = true; break; }
            case ui::A_MAINMENU: gh_autosave(); gh_play_commit(); ui::settings_save(U, gApp.cfgpath); if (gh_crt_enabled()) { gh_crt_sfx("crt_off.wav"); gh_crt_run(false, buf); } if (gApp.inwin_enabled && !gApp.headless && gApp.use_gl) gApp.inwin_req = 2; else gh_spawn_menu(); GH_QUITSET(); done = true; break;   /* t51a: 창 안 메뉴 */
            case ui::A_QUIT: gh_autosave(); gh_play_commit(); if (a.arg) ui::settings_save(U, gApp.cfgpath); if (gh_crt_enabled()) { gh_crt_sfx("crt_off.wav"); gh_crt_run(false, buf); } GH_QUITSET(); done = true; break;
            default: break; }
            if (done) break;
        }
        gh_wm_poll();
        const bool snapscr = U.scr == ui::S_GAME || U.scr == ui::S_KEYS || U.scr == ui::S_QUIT || U.scr == ui::S_SWAP || U.scr == ui::S_SWAPI;
        gh_stem_step(); U.fs_now = gh_fs_now(); U.ts_ms = (int)SDL_GetTicks();
        if (gh_menu_sq()) msq::compose(U, buf, dimb, snapscr); else { if (snapscr) memcpy(buf, snap, sizeof buf); U.draw(c); } { std::string bw = gh_builtin_warn(); if (!bw.empty()) ov::osd_draw(c, bw, false, 336); } if (gApp.osd_until && SDL_GetTicks() < gApp.osd_until) ov::osd_draw(c, gApp.osd, true);
        gh_publish(buf, 640, 400); if (mo_fr < 12u) { gh_log("MENU-OPEN frame %u +%ums work=%ums scr=%d", mo_fr, SDL_GetTicks() - mo_t0, SDL_GetTicks() - mo_f0, (int)U.scr); } mo_fr++; SDL_Delay(16);
    }
    if (U.dirty) { ui::settings_save(U, gApp.cfgpath); U.dirty = false; }
    U.in_game = false; gApp.menu_open = false; gApp.bk_last = SDL_GetTicks();   /* t47a: 메뉴에 있던 시간은 자동 입력 시계에서 뺀다 */
    { int cg_ = gApp.cur_game; std::string o_; if (cg_ >= 0 && cg_ < (int)U.games.size()) for (auto & kv_ : U.games[cg_].ov) o_ += kv_.first + "=" + kv_.second + " "; gh_log("APPLYDIAG id=%s has_t=%d ov[%s] g.scan=%d glow=%d curve=%d", (cg_ >= 0 && cg_ < (int)U.games.size()) ? U.games[cg_].id.c_str() : "-", (int)(cg_ >= 0 && cg_ < (int)ui::Ui::MAXG && U.has_t[cg_]), o_.c_str(), U.g.scan, U.g.glow, U.g.curve); } gh_apply_settings(U.effective(gApp.cur_game), true); gh_boost_update(); kbov::wake();
    if (gApp.osd_until && !gApp.boost_on && SDL_GetTicks() < gApp.osd_until && gApp.osd.find("▶▶") == 0) gApp.osd_until = 0;
    if (sendk) gh_sendkey(sendarg);
    if (!gApp.fb.empty()) gh_publish(gApp.fb.data(), gApp.fbw, gApp.fbh);
}

