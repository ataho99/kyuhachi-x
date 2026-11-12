#pragma once
/* kh_touchfx.h -- 터치 위치 표시(큐하치 자체 효과, kx-touch). 손가락(과 옵션으로 마우스) 위치를 SDL 이벤트 감시로 모으고,
 * 렌더 스레드가 16x16 도트 스프라이트(십자 + 퍼지는 고리)를 창 좌표의 작은 사각형에 그린다. 스크린샷에는 안 찍힌다. */
#include <stdint.h>
#include <math.h>
#include <mutex>
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
    for (int y = 0; y < 13; y++) { const char * r = y < 2 ? BF_HEAD[y] : body[y - 2]; for (int x = 0; x < 8; x++) { g[y][x] = r[x]; g[y][15 - x] = r[x]; } }
    if (sp) { for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { if (g[y][x] == 'C') g[y][x] = 'W'; else if (g[y][x] == 'b') g[y][x] = 'C'; }
        const int px[6] = { 1, 14, 0, 15, 7, 8 }, py[6] = { 2, 2, 10, 10, 0, 0 }; for (int i = 0; i < 6; i++) if (g[py[i]][px[i]] == '.') g[py[i]][px[i]] = 'Y'; }
    int a = 255; if (sp) { a = 255 - rel * 255 / 220; if (a < 0) a = 0; }
    for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) b[y * 16 + x] = bf_col(g[y][x], (uint32_t)a);
}
}   /* namespace tfx */
