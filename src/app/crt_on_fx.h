#pragma once
/* app/crt_on_fx.h -- #S3d/#S3e: CRT 켜짐 화면 연출(PC-98 RGB 모니터에서 실제로 보이는 것만 · 개미/점 잡음 없음).
 *   e 서서히 밝아짐(형광체 예열) · a 과밝기 번짐 · b 상하 크기/위치 출렁임(2~3번) · d 소자(디가우스) 색 일렁임(한 번)
 *   단계: 1 약 = e / 2 중 = a+b+e / 3 강 = a+b+d+e.  0 = 예전 연출 그대로(이 파일을 쓰지 않음).
 * #S3e: 네모가 늘어나는 모양을 버리고 가장자리를 모두 부드럽게 — 가로선은 끝이 가늘어지며 사라지고 위아래로 빛이 번지며,
 *   세로로 펼쳐질 때 위아래 경계는 그라데이션, 가운데가 가장 밝고 네 귀가 둥글게 늦게 차오른다(가운데 밝고 가장자리 어두운 번짐).
 * 꺼짐 연출·소리·길이는 그대로. 순수 계산(SDL 없음). 켜짐 길이 = on_ms(lvl): 640 / 900 / 1000 ms (예전 520). */
#include <stdint.h>
#include <math.h>
namespace crtfx {
static inline unsigned on_ms(int lvl) { return lvl <= 0 ? 520u : lvl == 1 ? 640u : lvl == 2 ? 900u : 1000u; }
static inline float ease(float q) { q = q < 0 ? 0 : q > 1 ? 1 : q; return q * q * (3 - 2 * q); }
static inline float clamp01(float q) { return q < 0 ? 0 : q > 1 ? 1 : q; }
static inline float sstep(float a, float b, float x) { return ease((x - a) / (b - a)); }
static inline uint32_t px(float r, float g, float b) { int R = r > 255 ? 255 : r < 0 ? 0 : (int)r, G = g > 255 ? 255 : g < 0 ? 0 : (int)g, B = b > 255 ? 255 : b < 0 ? 0 : (int)b; return 0xFF000000u | ((uint32_t)R << 16) | ((uint32_t)G << 8) | (uint32_t)B; }
/* o = 640x400 ARGB. p = 0..1 (경과 / on_ms(lvl)), lvl = 1..3. 마지막 프레임(p=1)은 예전처럼 검정 — 뒤의 부팅 연출이 검정에서 이어진다. */
static void on_frame(uint32_t * o, float p, int lvl) {
    const int W = 640, H = 400, cx = W / 2, cy0 = H / 2; for (int i = 0; i < W * H; i++) o[i] = 0xFF000000u;
    p = clamp01(p);
    const float GR = 205.0f, GG = 255.0f, GB = 215.0f;
    const bool fa = lvl >= 2, fb = lvl >= 2, fd = lvl >= 3;
    const float tms = p * (float)on_ms(lvl);
    const float warm = 0.30f + 0.70f * ease(tms / 500.0f);   /* e: 어둡게 시작해 약 0.5초에 걸쳐 제 밝기로 */
    if (p < 0.4f) {   /* 가로선: 양옆으로 퍼지되 끝이 뚝 끊기지 않고 가늘어지며 사라진다 + 선 위아래 빛 번짐 */
        const float q = p / 0.4f; const float hw = ease(q) * 330.0f + 1.0f; const float k = (0.55f + 0.45f * q) * warm;
        for (int y = cy0 - 14; y <= cy0 + 14; y++) { const float dy = (float)(y - cy0) + 0.5f;
            for (int x = cx - (int)hw - 2; x <= cx + (int)hw + 2; x++) { if (x < 0 || x >= W) continue;
                const float u = fabsf((float)(x - cx) + 0.5f) / hw; if (u >= 1.0f) continue;
                const float taper = 1.0f - sstep(0.35f, 1.0f, u);            /* 끝으로 갈수록 어두워지고 */
                const float sg = 0.9f + 0.9f * taper;                         /* 얇아짐 */
                const float core = expf(-(dy * dy) / (sg * sg));
                const float glow = 0.30f * expf(-(dy * dy) / (7.0f * 7.0f)) * (0.4f + 0.6f * taper);   /* 선 위아래 번짐 */
                const float v = k * taper * (core + glow); if (v <= 0.002f) continue;
                o[y * W + x] = px(GR * v, GG * v, GB * v); } }
        return;
    }
    const float q = (p - 0.4f) / 0.6f;   /* 0..1 : 선이 위아래로 번짐 → 가라앉음 */
    float br, haze = 0.0f, fill, cy = (float)cy0;
    if (!fa) { br = (1.0f - q) * (1.0f - q) * 1.35f; fill = ease(q); }                     /* 약: 예전 밝기 곡선 + e */
    else {
        fill = ease(q / 0.55f);                                                            /* 중·강: 세로 펼침을 앞 55% 에 끝내고 */
        br = powf(1.0f - q, 1.5f) * 1.8f; haze = 26.0f * (1.0f - q) * (1.0f - q);          /* a: 더 밝게 · 뿌연 막 */
    }
    float hhs = 1.0f;   /* b: 크기·위치 2.5번 출렁 */
    if (fb && q > 0.55f) { const float q2 = (q - 0.55f) / 0.45f, wob = (1.0f - q2) * sinf(6.2831853f * 2.5f * q2); hhs = 1.0f + 0.09f * wob; cy = (float)cy0 + 7.0f * wob; }
    const float hh0 = (fill * 198.0f + 2.0f) * hhs;
    float env = 0.0f; if (fd && q > 0.2f && q < 0.8f) env = sinf(3.1415927f * (q - 0.2f) / 0.6f);   /* d: 한 번 일었다 가라앉는 색 일렁임 */
    const float ew = 16.0f + 0.10f * hh0;      /* 위아래 경계의 부드러운 폭(칼선 아님) */
    const float edge_x = 10.0f + 0.35f * (float)W * 0.1f;   /* 좌우 경계 부드러움 */
    for (int y = 0; y < H; y++) {
        const float dy = (float)y - cy + 0.5f, ay = fabsf(dy), ry = ay / 200.0f;
        for (int x = 0; x < W; x++) {
            const float dx = (float)(x - cx) + 0.5f, rx = fabsf(dx) / 320.0f;
            /* 네 귀는 늦게 차오른다: 가장자리(rx 큼)일수록 세로 펼침 높이가 작다 */
            const float hh = hh0 * (1.0f - 0.45f * rx * rx * (1.0f - fill) - 0.20f * rx * rx);
            const float yv = 1.0f - sstep(hh - ew, hh + ew * 0.6f, ay);            /* 위아래 그라데이션 경계 */
            const float xv = 1.0f - sstep(320.0f - edge_x * 3.0f * (1.0f - fill * 0.5f), 330.0f, fabsf(dx));   /* 좌우 끝도 부드럽게 */
            const float cen = 1.0f - 0.55f * (rx * rx * 0.7f + ry * ry * 0.5f);     /* 가운데 밝고 가장자리 어두운 번짐 */
            float m = yv * xv; if (m <= 0.001f) continue;
            float k = (br * (1.0f - 0.45f * ay / (hh0 + 1.0f)) * cen + (1.0f - q) * 0.03f) * warm * m;
            float hz = haze * warm * m * (0.35f + 0.65f * cen);
            float r = GR * k + hz, g = GG * k + hz, b = GB * k + hz;
            if (env > 0.0f) { const float ph = 6.2831853f * ((float)x / 480.0f + (float)y / 320.0f - 1.5f * q); r *= 1.0f + 0.20f * env * sinf(ph); b *= 1.0f + 0.20f * env * sinf(ph + 2.1f); g *= 1.0f - 0.08f * env * sinf(ph + 4.2f); }
            o[y * W + x] = px(r, g, b);
        } }
}
}
