#pragma once
#include <vector>
/* kh_tlay_draw.h -- t862a: 터치 배치 그리기(창 크기 미리곱 ARGB 버퍼, 렌더 스레드). vpad::shape/icon 재사용 */
namespace tlay {
/* t862b2: 라벨을 키 높이의 약 30% 글자 높이로 키운다 — 기본 16px 글꼴을 임시 캔버스에 찍고 최근접으로 확대 */
static void bigtext(uint32_t * buf, int bw, int bh, const Rect & r, const char * t, uint32_t ink) {
    int tw = ov::tw(t); if (tw <= 0) return;
    std::vector<uint32_t> tmp((size_t)tw * 16, 0); ov::Canvas tc = { tmp.data(), tw, 16 }; ov::text(tc, 0, 0, t, ink);
    int th = r.h * 30 / 100; if (th < 16) th = 16; int ow = tw * th / 16;
    int x0 = r.x + (r.w - ow) / 2, y0 = r.y + (r.h - th) / 2;
    for (int y = 0; y < th; y++) { int sy = y * 16 / th; for (int x = 0; x < ow; x++) { int sx = x * tw / ow; uint32_t p = tmp[(size_t)sy * tw + sx]; if ((p >> 24) == 0) continue;
        int px = x0 + x, py = y0 + y; if (px < 0 || py < 0 || px >= bw || py >= bh) continue; buf[(size_t)py * bw + px] = p; } }
}
/* t863: 글자·화살표 그림 + 어두운 외곽 그림자(밝은 화면에서도 보이게). k<4 = 방향 화살표 */
static void glyph(uint32_t * buf, int dw, int dh, int k, const Rect & r, const char * txt, uint32_t ink, bool shadow) {
    static const int IC[4] = { vpad::B_LEFT, vpad::B_RIGHT, vpad::B_UP, vpad::B_DOWN };
    if (shadow) {
        static const int OX[8] = { -1, 0, 1, -1, 1, -1, 0, 1 }, OY[8] = { -1, -1, -1, 0, 0, 1, 1, 1 }; int sh = r.w / 36 > 2 ? r.w / 36 : 2;
        for (int i = 0; i < 8; i++) { Rect q = { r.x + OX[i] * sh, r.y + OY[i] * sh, r.w, r.h }; vpad::Rect vq = { q.x, q.y, q.w, q.h };
            if (k >= 0 && k < 4) vpad::icon(buf, dw, dh, IC[k], vq, vpad::pm(225, 0, 0, 0)); else bigtext(buf, dw, dh, q, txt, vpad::pm(225, 0, 0, 0)); }
    }
    vpad::Rect vr = { r.x, r.y, r.w, r.h };
    if (k >= 0 && k < 4) vpad::icon(buf, dw, dh, IC[k], vr, ink); else bigtext(buf, dw, dh, r, txt, ink);
}
static void scale_buf(uint32_t * buf, size_t n, int k) {   /* t863: 오버레이 진하기 설정 배율(premultiplied) */
    for (size_t i = 0; i < n; i++) { uint32_t p = buf[i], a = p >> 24; if (!a) continue; uint32_t na = a * (uint32_t)k / 256u; if (na > 255u) na = 255u;
        uint32_t r = ((p >> 16) & 255u) * na / a, gg = ((p >> 8) & 255u) * na / a, b = (p & 255u) * na / a; if (r > na) r = na; if (gg > na) gg = na; if (b > na) b = na;
        buf[i] = (na << 24) | (r << 16) | (gg << 8) | b; }
}
static void render(uint32_t * buf, int dw, int dh, const Geo & g, unsigned mask, int dir, int stk_on, int sx, int sy, int dx, int dy) {
    memset(buf, 0, (size_t)dw * dh * 4);
    if (g_edit && g_pick == 2) return;   /* t863: 화면 키보드로 키를 고르는 중 — 키보드를 가리지 않게 비움 */
    const bool edit = g_edit != 0;
    static const char * LB[NK] = { "", "", "", "", "RET", "ESC", "SPC", "TAB", "L", "R" };
    for (int k = 0; k < NK; k++) {
        const Rect & r = g.r[k]; if (r.w <= 0) continue;
        bool on = ((mask >> k) & 1u) != 0;
        uint32_t edge = on ? vpad::pm(235, 255, 255, 255) : vpad::pm(150, 200, 208, 230), face = on ? vpad::pm(220, 255, 192, 0) : vpad::pm(90, 32, 40, 64);
        uint32_t ink = on ? vpad::pm(255, 0, 0, 0) : vpad::pm(235, 255, 255, 255);
        vpad::Rect vr = { r.x, r.y, r.w, r.h };
        if (edit) { vpad::shape(buf, dw, dh, vr, false, 0, edge); vpad::shape(buf, dw, dh, vr, false, r.w / 24 > 2 ? r.w / 24 : 2, face); glyph(buf, dw, dh, k, r, LB[k], ink, false); }
        else {   /* 게임 중: 박스 없이 그림만(눌린 동안만 밝은 바탕) */
            if (on) { vpad::shape(buf, dw, dh, vr, false, 0, edge); vpad::shape(buf, dw, dh, vr, false, r.w / 24 > 2 ? r.w / 24 : 2, face); }
            glyph(buf, dw, dh, k, r, LB[k], ink, !on);
        }
    }
    for (int i = 0; i < g.nx; i++) {   /* 추가 키 */
        const Rect & r = g.xr[i]; bool on = ((mask >> (NK + i)) & 1u) != 0; int ix = g_lay[g_curmode ? 1 : 0].x[i].idx;
        uint32_t edge = on ? vpad::pm(235, 255, 255, 255) : vpad::pm(150, 200, 208, 230), face = on ? vpad::pm(220, 255, 192, 0) : vpad::pm(90, 32, 40, 64), ink = on ? vpad::pm(255, 0, 0, 0) : vpad::pm(235, 255, 255, 255);
        vpad::Rect vr = { r.x, r.y, r.w, r.h };
        if (edit || on) { vpad::shape(buf, dw, dh, vr, false, 0, edge); vpad::shape(buf, dw, dh, vr, false, r.w / 24 > 2 ? r.w / 24 : 2, face); }
        if (ix >= 0 && ix < g_nlab) glyph(buf, dw, dh, -1, r, g_lab[ix], ink, !edit && !on);
    }
    if (dir == 1 && g.ir.w > 0) {   /* 떠다니는 스틱: 평소 = 흐린 원, 누르는 동안 = 중심 원 + 손가락 위치. 바깥에 어두운 테두리 */
        int cx = stk_on ? sx : g.idle_x, cy = stk_on ? sy : g.idle_y, R = g.stick_r;
        vpad::Rect a = { cx - R, cy - R, R * 2, R * 2 }, o = { cx - R - 3, cy - R - 3, R * 2 + 6, R * 2 + 6 };
        vpad::shape(buf, dw, dh, o, true, 0, vpad::pm(150, 0, 0, 0));
        vpad::shape(buf, dw, dh, a, true, 0, vpad::pm(stk_on ? 150 : 90, 200, 208, 230)); vpad::shape(buf, dw, dh, a, true, R / 16 > 2 ? R / 16 : 2, vpad::pm(stk_on ? 60 : 24, 32, 40, 64));
        if (stk_on) { int kr = R * 4 / 10; int ex = dx - cx, ey = dy - cy; int L2 = ex * ex + ey * ey; if (L2 > R * R) { int L = 1; while (L * L < L2) L++; ex = ex * R / L; ey = ey * R / L; }
            vpad::Rect b = { cx + ex - kr, cy + ey - kr, kr * 2, kr * 2 }, bo = { cx + ex - kr - 3, cy + ey - kr - 3, kr * 2 + 6, kr * 2 + 6 };
            vpad::shape(buf, dw, dh, bo, true, 0, vpad::pm(170, 0, 0, 0));
            vpad::shape(buf, dw, dh, b, true, 0, vpad::pm(230, 255, 255, 255)); vpad::shape(buf, dw, dh, b, true, 2, vpad::pm(200, 255, 192, 0)); }
    }
    if (!edit && g_aa != 256) scale_buf(buf, (size_t)dw * dh, g_aa);
}
/* t863: 편집 화면 겹침 — 위 막대 5개(+ADD 가 메뉴) + 추가 메뉴(키보드·아날로그·십자) + 키 고르기(화면 키보드) 안내 + 메시지 */
static void render_edit(uint32_t * buf, int dw, int dh, const Geo & g) {
    (void)g;
    static const char * BL[NB] = { "SAVE", "DEFAULT", "+ADD", "EXPORT", "IMPORT" };
    if (g_pick == 2) {   /* 화면 키보드에서 키를 누르면 추가됨 */
        Rect h = { dw / 2 - dw / 3, dh * 2 / 100, dw * 2 / 3, dh * 9 / 100 };
        for (int y = h.y - 4; y < h.y + h.h + 4; y++) for (int x = h.x - 8; x < h.x + h.w + 8; x++) if (x >= 0 && y >= 0 && x < dw && y < dh) buf[(size_t)y * dw + x] = vpad::pm(210, 8, 10, 20);
        bigtext(buf, dw, dh, h, "TAP A KEY ON THE KEYBOARD  (TAP ELSEWHERE = CANCEL)", vpad::pm(235, 255, 255, 255));
        if (g_msg[0]) { Rect m = { dw / 2 - dw / 4, dh * 12 / 100, dw / 2, dh * 8 / 100 }; bigtext(buf, dw, dh, m, g_msg, vpad::pm(255, 255, 224, 96)); }
        return;
    }
    if (g_pick == 1) { for (size_t i = 0, n = (size_t)dw * dh; i < n; i++) buf[i] = vpad::pm(200, 8, 10, 20); }
    for (int b = 0; b < NB; b++) { Rect r = bar_rect(b, dw, dh); vpad::Rect vr = { r.x, r.y, r.w, r.h };
        vpad::shape(buf, dw, dh, vr, false, 0, vpad::pm(235, 255, 192, 0)); vpad::shape(buf, dw, dh, vr, false, 3, vpad::pm(235, 40, 48, 72)); bigtext(buf, dw, dh, r, BL[b], vpad::pm(255, 255, 255, 255)); }
    if (g_pick == 1) {
        const int hid = g_lay[g_curmode ? 1 : 0].hid;
        const char * ML[3] = { "KEYBOARD", (hid & 2) ? "ANALOG OFF" : "ANALOG ON", (hid & 1) ? "DPAD OFF" : "DPAD ON" };
        for (int i = 0; i < 3; i++) { Rect r = menu_rect(i, dw, dh); vpad::Rect vr = { r.x, r.y, r.w, r.h };
            vpad::shape(buf, dw, dh, vr, false, 0, vpad::pm(200, 200, 208, 230)); vpad::shape(buf, dw, dh, vr, false, 3, vpad::pm(235, 32, 40, 64)); bigtext(buf, dw, dh, r, ML[i], vpad::pm(255, 255, 255, 255)); }
    }
    else { Rect h = { dw / 2 - dw / 4, dh * 10 / 100, dw / 2, dh * 8 / 100 }; bigtext(buf, dw, dh, h, "DRAG = MOVE   TAP EXTRA KEY = DELETE", vpad::pm(235, 255, 255, 255)); }
    if (g_msg[0]) { Rect h = { dw / 2 - dw / 4, dh * 45 / 100, dw / 2, dh * 10 / 100 }; bigtext(buf, dw, dh, h, g_msg, vpad::pm(255, 255, 224, 96)); }
}
}   /* namespace tlay */
