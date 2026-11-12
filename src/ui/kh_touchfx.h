#pragma once
/* kh_touchfx.h -- 터치 위치 표시(큐하치 자체 효과, kx-touch). 손가락(과 옵션으로 마우스) 위치를 SDL 이벤트 감시로 모으고,
 * 렌더 스레드가 16x16 도트 스프라이트(십자 + 퍼지는 고리)를 창 좌표의 작은 사각형에 그린다. 스크린샷에는 안 찍힌다. */
#include <stdint.h>
#include <math.h>
#include <mutex>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <filesystem>
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"   /* PNG 전용(사용자 터치 모양) */
namespace tfx {
static const int NP = 6;
struct P { bool used; bool mouse; long long id; float x, y; uint32_t t0, t1, t2; };   /* x,y = 창 0..1 · t1 = 뗀 시각(0 = 누름) */
struct V { float x, y; int age, rel; };
static P g_p[NP];
static std::mutex g_mx;
static SDL_Window * g_win = 0;   /* 렌더 스레드가 매 프레임 채움 */
static bool g_reg = false;
static int slot_of(bool mouse, long long id) {
    for (int i = 0; i < NP; i++) if (g_p[i].used && g_p[i].mouse == mouse && g_p[i].id == id && g_p[i].t1 == 0) return i;
    return -1;
}
static int new_slot(void) {
    int old = 0;
    for (int i = 0; i < NP; i++) { if (!g_p[i].used) return i; if (g_p[i].t0 < g_p[old].t0) old = i; }
    return old;
}
static void feed(bool mouse, long long id, int ph, float x, float y, uint32_t now) {   /* ph 0 누름 1 뗌 2 이동 */
    std::lock_guard<std::mutex> lk(g_mx);
    int i = slot_of(mouse, id);
    if (ph == 0) { if (i >= 0) g_p[i].t1 = now; i = new_slot(); g_p[i] = P{ true, mouse, id, x, y, now, 0, now }; return; }
    if (i < 0) return;
    g_p[i].x = x; g_p[i].y = y; g_p[i].t2 = now; if (ph == 1) g_p[i].t1 = now;
}
static int SDLCALL watch(void *, SDL_Event * e) {
    const uint32_t now = SDL_GetTicks();
    if (e->type == SDL_FINGERDOWN || e->type == SDL_FINGERMOTION || e->type == SDL_FINGERUP) {
        feed(false, (long long)e->tfinger.fingerId, e->type == SDL_FINGERDOWN ? 0 : e->type == SDL_FINGERUP ? 1 : 2,
             e->tfinger.x, e->tfinger.y, now);
        return 0;
    }
    int ph = -1, mx = 0, my = 0;
    if (e->type == SDL_MOUSEBUTTONDOWN || e->type == SDL_MOUSEBUTTONUP) {
        if (e->button.which == SDL_TOUCH_MOUSEID || e->button.button != SDL_BUTTON_LEFT) return 0;
        ph = e->type == SDL_MOUSEBUTTONDOWN ? 0 : 1; mx = e->button.x; my = e->button.y;
    } else if (e->type == SDL_MOUSEMOTION) {
        if (e->motion.which == SDL_TOUCH_MOUSEID || !(e->motion.state & SDL_BUTTON_LMASK)) return 0;
        ph = 2; mx = e->motion.x; my = e->motion.y;
    } else return 0;
    int w = 0, h = 0; if (!g_win) return 0;
    SDL_GetWindowSize(g_win, &w, &h); if (w <= 0 || h <= 0) return 0;
    feed(true, 0, ph, (float)mx / (float)w, (float)my / (float)h, now);
    return 0;
}
static int collect(uint32_t now, bool mouse_ok, V * out) {   /* 보일 표시를 모으고 끝난 것은 치운다 */
    std::lock_guard<std::mutex> lk(g_mx); int n = 0;
    for (int i = 0; i < NP; i++) {
        P & p = g_p[i]; if (!p.used) continue;
        const int rel = p.t1 ? (int)(now - p.t1) : -1;
        if (rel > 220 || (!p.t1 && now - p.t2 > 15000u)) { p.used = false; continue; }
        if (p.mouse && !mouse_ok) continue;
        out[n].x = p.x; out[n].y = p.y; out[n].age = (int)(now - p.t0); out[n].rel = rel; n++;
    }
    return n;
}
static inline uint32_t pm(uint32_t a, uint32_t r, uint32_t g, uint32_t b) { return (a << 24) | ((r * a / 255) << 16) | ((g * a / 255) << 8) | (b * a / 255); }
static inline void put(uint32_t * b, int x, int y, uint32_t c) { if (x >= 0 && y >= 0 && x < 16 && y < 16) b[y * 16 + x] = c; }
/* 16x16 BGRA(곱한 알파). age = 누른 뒤 ms · rel = 뗀 뒤 ms(-1 = 누르는 중). 십자는 누르는 동안 유지, 뗄 때 200ms 안에 사라짐 */
static void sprite(uint32_t * b, int age, int rel) {
    uint8_t m[256], w[256]; for (int i = 0; i < 256; i++) { m[i] = 0; w[i] = 0; }   /* m: 1 시안 · 2 흰색 */
    const int a = rel >= 0 ? (235 - rel * 235 / 200 > 0 ? 235 - rel * 235 / 200 : 0) : 235;
    if (a > 0) {
        for (int d = 2; d <= 6; d++) for (int t = 7; t <= 8; t++) { m[t * 16 + 8 + d - (d > 0 ? 0 : 0)] = 1; m[t * 16 + 7 - d] = 1; m[(8 + d) * 16 + t] = 1; m[(7 - d) * 16 + t] = 1; }
        m[7 * 16 + 7] = m[7 * 16 + 8] = m[8 * 16 + 7] = m[8 * 16 + 8] = 2;
    }
    const int ra = (rel < 0 && age < 260) ? 255 - age * 255 / 260 : 0;
    if (ra > 0) {
        const double R = 3.5 + 3.5 * (age < 200 ? age / 200.0 : 1.0);
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
            const double dx = x - 7.5, dy = y - 7.5, d = sqrt(dx * dx + dy * dy);
            if (fabs(d - R) < 0.6 && !m[y * 16 + x]) w[y * 16 + x] = 1;
        }
    }
    for (int i = 0; i < 256; i++) b[i] = 0;
    const int aa = a > 0 ? a : 0;
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
        const int i = y * 16 + x;
        if (m[i]) b[i] = m[i] == 2 ? pm((uint32_t)aa, 255, 255, 255) : pm((uint32_t)aa, 0, 238, 238);
        else if (w[i]) b[i] = pm((uint32_t)ra, 255, 255, 255);
    }
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {   /* 어두운 남색 테두리: 비어 있는 칸이 이웃 4방향 중 하나라도 칠해졌으면 */
        const int i = y * 16 + x; if (b[i]) continue;
        const bool nb = (x > 0 && (m[i - 1] || w[i - 1])) || (x < 15 && (m[i + 1] || w[i + 1])) || (y > 0 && (m[i - 16] || w[i - 16])) || (y < 15 && (m[i + 16] || w[i + 16]));
        if (nb) { const int oa = ra > aa ? ra : aa; b[i] = pm((uint32_t)(oa * 200 / 255), 51, 0, 119); }
    }
}
/* 내장 나비(16x16, PC-98 16색 계열 · 왼쪽 8칸만 적고 거울) — 손가락을 따라다닌다. 누르는 동안 날갯짓, 뗄 때 반짝하며 사라진다 */
static const char * const BF_HEAD[2] = { "....i...", ".....i.y" };
static const char * const BF_A[11] = { "..kkk..o", "kWWCkk.o", "kWCCCCko", "kCCbCCko", ".kCCbCko", "..kCCkko", ".kPPkk.o", "kPPYPkko", "kPYPPk.o", ".kkkk..o", "........" };
static const char * const BF_B[11] = { "..kk...o", ".kWCk..o", ".kWCCkko", ".kCbCCko", "..kCbCko", "...kCkko", "..kPPk.o", ".kPYPkko", "..kPPk.o", "...kk..o", "........" };
static const char * const BF_C[11] = { ".......o", "...kk..o", "..kWCk.o", "..kCbkko", "..kCCk.o", "...kCk.o", "..kPPk.o", "..kPYk.o", "...kk..o", "........", "........" };
static uint32_t bf_col(char ch, uint32_t a) {
    switch (ch) { case 'k': return pm(a, 51, 0, 119); case 'i': return pm(a, 170, 170, 255); case 'o': return pm(a, 153, 85, 0);
        case 'y': case 'Y': return pm(a, 255, 255, 0); case 'W': return pm(a, 255, 255, 255); case 'C': return pm(a, 0, 238, 238);
        case 'b': return pm(a, 0, 102, 221); case 'P': return pm(a, 255, 136, 204); default: return 0; }
}
static void sprite_bf(uint32_t * b, int age, int rel) {
    const int step = (age / 90) & 3; const bool sp = rel >= 0; const char * const * body = (step == 0) ? BF_A : (step == 2) ? BF_C : BF_B;
    if (sp) body = BF_B;
    char g[16][17]; for (int y = 0; y < 16; y++) { for (int x = 0; x < 16; x++) g[y][x] = '.'; g[y][16] = 0; }
    for (int y = 0; y < 13; y++) { if (y < 2) continue; const char * r = body[y - 2]; for (int x = 0; x < 8; x++) { const char q = r[x] == 'o' ? '.' : r[x]; g[y][x] = q; g[y][15 - x] = q; } }   /* 날개만(몸통·머리 없음) */
    if (sp) { for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { if (g[y][x] == 'C') g[y][x] = 'W'; else if (g[y][x] == 'b') g[y][x] = 'C'; }
        const int px[6] = { 1, 14, 0, 15, 3, 12 }, py[6] = { 2, 2, 10, 10, 1, 1 }; for (int i = 0; i < 6; i++) if (g[py[i]][px[i]] == '.') g[py[i]][px[i]] = 'Y'; }
    int a = 255; if (sp) { a = 255 - rel * 255 / 220; if (a < 0) a = 0; }
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) b[y * 16 + x] = bf_col(g[y][x], (uint32_t)a);
}
/* 사용자 모양: 안드로이드 /sdcard/Kyuhachi/touch/<이름>/ · 윈도 <exe 폴더>\touch\<이름>\ 의 PNG(이름순, 최대 16장, 128x128 이하, 모두 같은 크기).
 * touch\_use.txt 첫 줄 = 쓸 폴더 이름(없으면 이름순 첫 폴더). 폴더 안 config.txt: frame_ms=90 · fade_ms=200 · scale=1 (모두 선택). 못 읽으면 도트로 돌아간다 */
struct UFrames { bool tried = false, ok = false; int w = 0, h = 0, ms = 90, fade = 200; float scale = 1.0f; std::vector<std::vector<uint32_t> > fr; };
static UFrames g_u;
static std::string trim_(const std::string & s) { size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n"); return a == std::string::npos ? std::string() : s.substr(a, b - a + 1); }
static void load_user(const std::string & base) {
    if (g_u.tried) return; g_u.tried = true;
    namespace fs = std::filesystem; std::error_code ec;
    const fs::path root = fs::u8path(base); if (!fs::is_directory(root, ec)) return;
    std::vector<std::string> names;
    for (fs::directory_iterator it(root, ec), en; !ec && it != en; it.increment(ec)) { std::error_code e2; if (it->is_directory(e2)) names.push_back(it->path().filename().u8string()); }
    if (names.empty()) return; std::sort(names.begin(), names.end());
    std::string use = names[0];
    { std::ifstream uf(root / "_use.txt"); std::string ln; if (uf && std::getline(uf, ln)) { ln = trim_(ln); for (size_t i = 0; i < names.size(); i++) if (names[i] == ln) use = ln; } }
    const fs::path dir = root / fs::u8path(use);
    { std::ifstream cf(dir / "config.txt"); std::string ln;
      while (cf && std::getline(cf, ln)) { const size_t q = ln.find('='); if (q == std::string::npos) continue; const std::string k = trim_(ln.substr(0, q)), v = trim_(ln.substr(q + 1));
          if (k == "frame_ms") g_u.ms = std::max(30, std::min(1000, atoi(v.c_str())));
          else if (k == "fade_ms") g_u.fade = std::max(0, std::min(220, atoi(v.c_str())));
          else if (k == "scale") g_u.scale = (float)std::max(0.25, std::min(8.0, atof(v.c_str()))); } }
    std::vector<std::string> pf;
    for (fs::directory_iterator it(dir, ec), en; !ec && it != en; it.increment(ec)) { std::error_code e2; if (!it->is_regular_file(e2)) continue;
        std::string fn = it->path().filename().u8string(), ex = it->path().extension().u8string(); for (size_t i = 0; i < ex.size(); i++) ex[i] = (char)tolower((unsigned char)ex[i]);
        if (ex == ".png") pf.push_back(fn); }
    std::sort(pf.begin(), pf.end(), [](const std::string & a, const std::string & b) { return a.size() != b.size() ? a.size() < b.size() : a < b; });   /* frame2 < frame10 */
    for (size_t i = 0; i < pf.size() && g_u.fr.size() < 16; i++) {
        std::ifstream f(dir / fs::u8path(pf[i]), std::ios::binary); if (!f) continue;
        std::vector<unsigned char> buf((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); if (buf.empty() || buf.size() > 2u * 1024u * 1024u) continue;
        int w = 0, h = 0, n = 0; unsigned char * px = stbi_load_from_memory(buf.data(), (int)buf.size(), &w, &h, &n, 4); if (!px) continue;
        if (w < 1 || h < 1 || w > 128 || h > 128 || (!g_u.fr.empty() && (w != g_u.w || h != g_u.h))) { stbi_image_free(px); continue; }
        g_u.w = w; g_u.h = h; std::vector<uint32_t> fr((size_t)w * h);
        for (size_t k = 0; k < fr.size(); k++) fr[k] = pm(px[k * 4 + 3], px[k * 4], px[k * 4 + 1], px[k * 4 + 2]);
        stbi_image_free(px); g_u.fr.push_back(fr); }
    g_u.ok = !g_u.fr.empty();
}
static void sprite_user(uint32_t * b, int age, int rel) {
    const int n = (int)g_u.fr.size(); int idx = (age / g_u.ms) % n; if (idx < 0) idx = 0;
    const std::vector<uint32_t> & f = g_u.fr[idx]; const size_t cnt = (size_t)g_u.w * g_u.h;
    int a = 256; if (rel >= 0) { a = g_u.fade > 0 ? 256 - rel * 256 / g_u.fade : 0; if (a < 0) a = 0; }
    for (size_t i = 0; i < cnt; i++) { const uint32_t p = f[i]; if (a >= 256) { b[i] = p; continue; }
        const uint32_t A = ((p >> 24) * (uint32_t)a) >> 8, Rr = (((p >> 16) & 255u) * (uint32_t)a) >> 8, G = (((p >> 8) & 255u) * (uint32_t)a) >> 8, B = ((p & 255u) * (uint32_t)a) >> 8;
        b[i] = (A << 24) | (Rr << 16) | (G << 8) | B; }
}
}   /* namespace tfx */
