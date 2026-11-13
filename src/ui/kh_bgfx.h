#pragma once
/* ui/kh_bgfx.h -- menu background effect (kx-bg). Drawn right after the black fill, BEHIND every menu element, only on
 * Kyuhachi's own screens (never in the in-game pause menu or the game picture). Pure function of time: no state, no allocation.
 * kx-nova 2: visible on Nova -- 2-3 px dots, normal level ~59% of white (far layer 70% of that), 100-150 dots, normal speed moves
 * within a 3 s look. Text and selection bars stay opaque, so dots never cover letters; boxes dim the dots behind them to 70%.
 * kx-bg 2: 13 entries (0 off, 1-4 dot effects, 5-12 dot/line effects of the old demo scene). The effect page previews the cursor row. */
namespace bgx {
static const int MAXN = 160, NOISEN = 300, NFX = 13;
static inline uint32_t hash(uint32_t x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return x; }
static inline uint32_t sc(uint32_t col, int lv) {   /* menu colour scaled to lv/255 */
    return ((((col >> 16) & 255u) * lv / 255u) << 16) | ((((col >> 8) & 255u) * lv / 255u) << 8) | ((col & 255u) * lv / 255u);
}
static inline int cl3(int v, int hi) { return v < 0 ? 0 : v > hi ? hi : v; }
static inline void dot(Canvas & c, int x, int y, int sz, uint32_t col) { if (sz > 1) c.rect(x, y, sz, sz, col); else c.set(x, y, col); }
static inline void hln(Canvas & c, int x, int y, int w, int th, uint32_t col) { c.rect(x, y, w, th, col); }
static inline void vln(Canvas & c, int x, int y, int h, uint32_t col) { c.rect(x, y, 1, h, col); }
static inline void dln(Canvas & c, int x0, int y0, int x1, int y1, int step, uint32_t col) {   /* dotted line */
    const double dx = (double)(x1 - x0), dy = (double)(y1 - y0); const int n = (int)(std::sqrt(dx * dx + dy * dy) / (double)step);
    for (int i = 0; i <= n; i++) { const double u = n ? (double)i / (double)n : 0.0; c.set(x0 + (int)(dx * u), y0 + (int)(dy * u), col); }
}
static inline void fx_bars(Canvas & c, int LC, double k, double t) {   /* raster bars: soft horizontal bands waving up and down */
    for (int b = 0; b < 5; b++) {
        const int yc = (int)((double)c.h * 0.5 + std::sin(t * k * 0.9 + (double)b * 0.8) * (double)c.h * 0.38);
        for (int r = -4; r <= 4; r++) hln(c, 0, yc + r, c.w, 1, sc((b & 1) ? CY : WH, LC * (5 - (r < 0 ? -r : r)) / 5));
    }
}
static inline void fx_sine(Canvas & c, int L, double k, double t) {   /* three rows of dots running as waves */
    for (int row = 0; row < 3; row++) {
        const double base = (double)c.h * (0.25 + 0.25 * row), amp = 28.0 + 8.0 * row;
        for (int x = 0; x < c.w; x += 8)
            { const int y = (int)(base + std::sin((double)x * 0.035 + t * k * 1.6 + row * 1.3) * amp);
              dot(c, x, y, 2, sc(row == 1 ? CY : WH, L * (85 - row * 10) / 100)); }
    }
}
static inline void fx_liss(Canvas & c, int L, double k, double t) {   /* two Lissajous curves of dots, tail fading */
    for (int cv = 0; cv < 2; cv++)
        for (int i = 0; i < 200; i++) {
            const double f = (double)i / 200.0, u = f * 6.2831853;
            const int x = c.w / 2 + (int)(std::sin(3.0 * u + t * k * 0.5 + cv * 1.0) * (double)c.w * 0.42);
            const int y = c.h / 2 + (int)(std::sin(2.0 * u + t * k * 0.37 + cv * 0.7) * (double)c.h * 0.42);
            dot(c, x, y, 2, sc(cv ? CY : WH, (int)(L * (0.15 + 0.85 * f))));
        }
}
static inline void fx_tunnel(Canvas & c, int LC, double k, double t) {   /* rectangles rushing out of the centre */
    for (int i = 0; i < 12; i++) {
        const double u = std::fmod(t * k * 0.35 + (double)i / 12.0, 1.0);
        const int hw = 6 + (int)(u * u * (double)c.w * 0.75), hh = hw * 5 / 8;
        const int x0 = c.w / 2 - hw, y0 = c.h / 2 - hh; const uint32_t col = sc((i & 1) ? CY : WH, (int)(LC * (0.15 + 0.85 * u)));
        hln(c, x0, y0, 2 * hw + 1, 1, col); hln(c, x0, y0 + 2 * hh, 2 * hw + 1, 1, col);
        vln(c, x0, y0, 2 * hh + 1, col); vln(c, x0 + 2 * hw, y0, 2 * hh + 1, col);
    }
}
static inline void fx_rain(Canvas & c, int LC, double k, double t) {   /* falling drops with a short fading tail */
    const int span = c.h + 40;
    for (int i = 0; i < 110; i++) {
        const uint32_t h = hash((uint32_t)i * 2971u + 17u); const int x = (int)(h % (uint32_t)c.w);
        const double v = (60.0 + (double)((h >> 8) % 90u)) * k;
        const double y = std::fmod((double)((h >> 16) % (uint32_t)span) + t * v, (double)span) - 20.0;
        for (int s = 0; s < 4; s++) dot(c, x, (int)y - s * 3, 2, sc(i % 4 == 0 ? CY : WH, LC * (4 - s) / 4));
    }
}
static inline void fx_speed(Canvas & c, int LC, double k, double t) {   /* horizontal speed lines in three depth layers */
    static const double V[3] = { 30.0, 60.0, 110.0 }; static const int LN[3] = { 14, 24, 40 };
    for (int ly = 0; ly < 3; ly++)
        for (int i = 0; i < 14; i++) {
            const uint32_t h = hash((uint32_t)i * 3331u + (uint32_t)ly * 977u + 3u); const int len = LN[ly], span = c.w + len;
            double x = std::fmod((double)(h % (uint32_t)span) - V[ly] * k * t, (double)span); if (x < 0) x += (double)span;
            hln(c, (int)x - len, (int)((h >> 8) % (uint32_t)c.h), len, ly == 2 ? 2 : 1, sc(ly == 2 ? CY : WH, LC * (50 + 25 * ly) / 100));
        }
}
static inline void fx_plasma(Canvas & c, int LC, double k, double t) {   /* a grid of dots whose brightness ripples */
    for (int gy = 4; gy < c.h; gy += 16)
        for (int gx = 4; gx < c.w; gx += 16) {
            const double v = std::sin(gx * 0.04 + t * k * 1.1) + std::sin(gy * 0.055 - t * k * 0.9)
                           + std::sin((gx + gy) * 0.03 + t * k * 0.7);
            const double n = (v / 3.0 + 1.0) * 0.5, a = n * n * 1.6; const int lv = (int)(LC * (a > 1.0 ? 1.0 : a));
            if (lv >= 10) dot(c, gx, gy, 2, sc(lv > LC * 3 / 4 ? CY : WH, lv));
        }
}
static inline void fx_moire(Canvas & c, int LC, double k, double t) {   /* two dotted fans turning against each other */
    for (int f = 0; f < 2; f++) {
        const int cx = (int)((double)c.w * (f ? 0.7 : 0.3)), cy = c.h / 2; const double a0 = t * k * (f ? -0.12 : 0.1);
        for (int s = 0; s < 20; s++) {
            const double a = a0 + (double)s * 0.3141593;
            dln(c, cx, cy, cx + (int)(std::cos(a) * 520.0), cy + (int)(std::sin(a) * 520.0), 4, sc(f ? CY : WH, LC * 4 / 5));
        }
    }
}
static inline void draw(Canvas & c, const Settings & s, int ms, int fxo = -1) {
    const int fx = fxo >= 0 ? fxo : s.bgfx;
    if (fx <= 0) return;
    static const int LV[3] = { 0x80, 0xB4, 0xF0 };   /* dim / normal / bright: 50 / 70 / 94 % of white (CRT filter dims ~1/3) */
    const int L = LV[cl3(s.bgbri, 2)], sp = cl3(s.bgspd, 2); const double t = (double)ms / 1000.0;
    const int W = c.w, H = c.h; static const double SK[3] = { 0.6, 1.0, 1.7 }; const int LC = L < 0x78 ? L : 0x78;
    if (fx == 1) {   /* star field: 3 layers flowing left, far layer 12 px/s, next x2, x4 (near 48 px/s) */
        static const double V[3] = { 12.0, 20.0, 32.0 }; static const int LY[3] = { 70, 85, 100 };
        for (int i = 0; i < 150; i++) {
            const int ly = i % 3; const uint32_t h = hash((uint32_t)i * 7919u + 13u);
            double x = (double)(h % (uint32_t)W) - V[sp] * (double)(1 << ly) * t; x = std::fmod(x, (double)W); if (x < 0) x += W;
            dot(c, (int)x, (int)((h >> 10) % (uint32_t)H), ly == 2 ? 3 : 2, sc(ly == 2 ? CY : WH, L * LY[ly] / 100));
        }
    } else if (fx == 2) {   /* twinkle in place */
        static const double P[3] = { 2.4, 1.4, 0.8 };
        for (int i = 0; i < 120; i++) {
            const uint32_t h = hash((uint32_t)i * 6271u + 5u); double u = std::fmod(t / P[sp] + (double)((h >> 20) & 1023u) / 1024.0, 1.0);
            const double tri = u < 0.5 ? u * 2.0 : 2.0 - u * 2.0;
            const int lv = (int)(L * (0.15 + 0.85 * tri));
            dot(c, (int)(h % (uint32_t)W), (int)((h >> 9) % (uint32_t)H), i % 5 == 0 ? 3 : 2, sc(i % 3 == 0 ? CY : WH, lv));
        }
    } else if (fx == 3) {   /* warp: from the centre to the edge in about 2 s */
        static const double CPS[3] = { 0.5, 0.8, 1.2 };
        for (int i = 0; i < 100; i++) {
            const uint32_t h = hash((uint32_t)i * 4099u + 7u); const double a = (double)(h & 0xFFFFu) / 65536.0 * 6.2831853;
            const double u = std::fmod(t * CPS[sp] + (double)((h >> 16) & 1023u) / 1024.0, 1.0), r = 4.0 + u * u * 380.0;
            const int x = W / 2 + (int)(std::cos(a) * r), y = H / 2 + (int)(std::sin(a) * r * 0.75);
            if (x < 0 || y < 0 || x >= W || y >= H) continue;
            dot(c, x, y, u > 0.6 ? 3 : 2, sc(i % 3 == 0 ? CY : WH, (int)(L * (0.2 + 0.8 * u))));
        }
    } else if (fx == 4) {   /* noise: 300 dots, each fades in and out smoothly (own phase) and jumps to a new spot when invisible */
        static const double CYC[3] = { 2.4, 1.5, 0.9 }; const int NL = L < 0x4C ? L : 0x4C;
        for (int i = 0; i < NOISEN; i++) {
            const uint32_t h0 = hash((uint32_t)i * 2654435761u + 3u);
            const double q = t / CYC[sp] + (double)(h0 & 1023u) / 1024.0;
            const uint32_t k = (uint32_t)q; const double u = q - (double)k;
            const uint32_t h = hash(k * 1315423911u + (uint32_t)i * 2246822519u + 11u);
            const double tri = u < 0.5 ? u * 2.0 : 2.0 - u * 2.0;
            dot(c, (int)(h % (uint32_t)W), (int)((h >> 10) % (uint32_t)H), 2, sc(WH, (int)(NL * tri)));
        }
    } else if (fx == 5) fx_bars(c, LC, SK[sp], t);
    else if (fx == 6) fx_sine(c, L, SK[sp], t);
    else if (fx == 7) fx_liss(c, L, SK[sp], t);
    else if (fx == 8) fx_tunnel(c, LC, SK[sp], t);
    else if (fx == 9) fx_rain(c, LC, SK[sp], t);
    else if (fx == 10) fx_speed(c, LC, SK[sp], t);
    else if (fx == 11) fx_plasma(c, LC, SK[sp], t);
    else fx_moire(c, LC, SK[sp], t);
}
}
