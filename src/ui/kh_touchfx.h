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
    for (int i = 0; i < 256; i++) b[i] = 0;
    int a = 235; if (rel >= 0) { a = 235 - rel * 235 / 200; if (a < 0) a = 0; }
    if (a > 0) {
        for (int d = 2; d <= 5; d++) { const uint32_t c = pm((uint32_t)a, 0, 238, 238);
            put(b, 8 + d, 8, c); put(b, 8 - d, 8, c); put(b, 8, 8 + d, c); put(b, 8, 8 - d, c); }
        put(b, 8, 8, pm((uint32_t)a, 255, 255, 255));
    }
    int ra = (rel < 0 && age < 220) ? 255 - age * 255 / 220 : 0;
    if (ra > 0) {
        const double R = 3.0 + 4.0 * (age < 180 ? age / 180.0 : 1.0);
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
            const double dx = x - 8, dy = y - 8, d = sqrt(dx * dx + dy * dy);
            if (fabs(d - R) < 0.55) put(b, x, y, pm((uint32_t)ra, 255, 255, 255));
        }
    }
}
}   /* namespace tfx */
