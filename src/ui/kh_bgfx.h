#pragma once
/* ui/kh_bgfx.h -- menu background effect (kx-bg). Drawn right after the black fill, BEHIND every menu element, only on
 * Kyuhachi's own screens (never in the in-game pause menu or the game picture). Pure function of time: no state, no allocation.
 * kx-nova 2: visible on Nova -- 2-3 px dots, normal level ~59% of white (far layer 70% of that), 100-150 dots, normal speed moves
 * within a 3 s look. Text and selection bars stay opaque, so dots never cover letters; boxes dim the dots behind them to 70%. */
namespace bgx {
static const int MAXN = 160, NOISEN = 300;
static inline uint32_t hash(uint32_t x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return x; }
static inline uint32_t sc(uint32_t col, int lv) {   /* menu colour scaled to lv/255 */
    return ((((col >> 16) & 255u) * lv / 255u) << 16) | ((((col >> 8) & 255u) * lv / 255u) << 8) | ((col & 255u) * lv / 255u);
}
static inline int cl3(int v, int hi) { return v < 0 ? 0 : v > hi ? hi : v; }
static inline void dot(Canvas & c, int x, int y, int sz, uint32_t col) { if (sz > 1) c.rect(x, y, sz, sz, col); else c.set(x, y, col); }
static inline void draw(Canvas & c, const Settings & s, int ms) {
    if (s.bgfx <= 0) return;
    static const int LV[3] = { 0x80, 0xB4, 0xF0 };   /* dim / normal / bright: 50 / 70 / 94 % of white (CRT filter dims ~1/3) */
    const int L = LV[cl3(s.bgbri, 2)], sp = cl3(s.bgspd, 2), fx = s.bgfx; const double t = (double)ms / 1000.0;
    const int W = c.w, H = c.h;
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
    } else {   /* noise: ~300 dots per frame at <= 30 % brightness, re-drawn every period (not a CRT overlay) */
        static const unsigned PER[3] = { 250u, 120u, 60u };
        const uint32_t step = (uint32_t)ms / PER[sp]; const int NL = L < 0x4C ? L : 0x4C;
        for (int i = 0; i < NOISEN; i++) {
            const uint32_t h = hash(step * 1315423911u + (uint32_t)i * 2654435761u);
            const int lv = (int)(NL * (0.4 + 0.6 * (double)((h >> 20) & 255u) / 255.0));
            dot(c, (int)(h % (uint32_t)W), (int)((h >> 10) % (uint32_t)H), 2, sc(WH, lv));
        }
    }
}
}
