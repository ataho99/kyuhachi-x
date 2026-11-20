#pragma once
/* kh_tlay.h -- t862: 터치 배치 엔진(창 좌표, 짧은 변 % 크기, 세트 배치 + 세트 이동·키 추가·해상도별 저장·내보내기/가져오기).
 * kh_vpad.h 와 같은 구조: 순수 판정·파일 입출력만, 호스트가 fire 와 경로를 잇는다.
 * kid 0..7 = ctlzone::KeyId 와 같은 번호(LEFT RIGHT UP DOWN ENTER ESC SPACE TAB), 8 L 9 R, 10.. = 추가 키(슬롯). */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
namespace tlay {
enum { T_LEFT = 0, T_RIGHT, T_UP, T_DOWN, T_ENTER, T_ESC, T_SPACE, T_TAB, T_L, T_R, NK };
enum { MAXX = 12, NSET = 3, NKX = NK + MAXX };
struct Rect { int x, y, w, h; };
struct Off { int dx, dy; };                    /* 창 너비·높이의 1/1000 */
struct Extra { int idx; int dx, dy; };         /* idx = 호스트 키 표 번호, dx/dy = 기본 자리(화면 위쪽 가운데)에서의 이동 */
struct Lay { Off o[NSET]; int nx; Extra x[MAXX]; int hid; };   /* t863: hid 비트0 = 십자 숨김, 비트1 = 아날로그 숨김 */
struct Geo { bool on; Rect r[NK]; Rect ir; int ks, stick_r, idle_x, idle_y, nx; Rect xr[MAXX]; };
typedef void (*FireFn)(int kid, bool down);
static FireFn g_fire = 0, g_firex = 0;         /* g_firex(표 번호, 눌림) */
static volatile int g_show = 0;
static void (*g_kbshow)(bool) = 0;             /* t863: 호스트가 채움 — 화면 키보드 켜기/끄기(키 고르기용) */
static int g_aa = 256;                         /* t863: 그림 진하기 배율(256 = 그대로) — 호스트가 오버레이 진하기 설정에서 채움 */
static int g_ldw = 0, g_ldh = 0, g_lks = 0;    /* t863: 마지막 배치 창 크기·키 한 변(키 고르기에서 기본 자리 계산) */
static volatile unsigned g_mask = 0;           /* 눌린 키 비트(그리기용, 0..21) */
static volatile int g_stk_on = 0, g_stk_x = 0, g_stk_y = 0, g_stk_dx = 0, g_stk_dy = 0;
static volatile int g_stk_mouse = 0; static volatile float g_stk_fx = 0.0f, g_stk_fy = 0.0f;   /* t862f: 스틱 = 마우스 커서 속도(기울기 -1..1) */
static int g_cnt[NKX];
static Lay g_lay[2];
static int g_ver = 0, g_edit = 0, g_pick = 0, g_nlab = 0;
static const char * const * g_lab = 0;         /* 호스트 키 표 라벨 */
static char g_path[512] = "", g_expath[512] = "", g_expath2[512] = "";
static unsigned g_now = 0, g_msg_until = 0; static char g_msg[48] = "";
static int g_curw = 0, g_curh = 0, g_curmode = 0;
struct Ptr { bool act; long long id; int kind; int key; int cx, cy; int mv; int ox, oy; };   /* kind 0 키, 1 스틱, 2 세트 끌기(편집), 3 막대 버튼, 4 고르기 칸 */
static Ptr g_p[8];

static inline bool in_r(const Rect & r, int x, int y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int iabs(int a) { return a < 0 ? -a : a; }

/* ---- 세트 이동: set 0 방향, 1 4키, 2 L·R(키보드+마우스), 3+i 추가 키 i. 창 안으로 가둔다. 실제 이동 px 를 되돌려준다 */
static void set_members(Geo & g, int set, int dir, Rect ** v, int & n) {
    n = 0;
    if (set == 0) { for (int k = 0; k < 4; k++) if (g.r[k].w > 0) v[n++] = &g.r[k]; if (dir == 1 && g.ir.w > 0) v[n++] = &g.ir; }
    else if (set == 1) { for (int k = 4; k < 8; k++) if (g.r[k].w > 0) v[n++] = &g.r[k]; }
    else if (set == 2) { for (int k = 8; k < 10; k++) if (g.r[k].w > 0) v[n++] = &g.r[k]; }
    else if (set - 3 < g.nx) v[n++] = &g.xr[set - 3];
}
static void set_shift(Geo & g, int set, int dir, int dw, int dh, int & pdx, int & pdy) {
    Rect * v[12]; int n; set_members(g, set, dir, v, n); if (n == 0) { pdx = pdy = 0; return; }
    int x0 = 1 << 30, y0 = 1 << 30, x1 = -(1 << 30), y1 = -(1 << 30);
    for (int i = 0; i < n; i++) { if (v[i]->x < x0) x0 = v[i]->x; if (v[i]->y < y0) y0 = v[i]->y; if (v[i]->x + v[i]->w > x1) x1 = v[i]->x + v[i]->w; if (v[i]->y + v[i]->h > y1) y1 = v[i]->y + v[i]->h; }
    if (x0 + pdx < 0) pdx = -x0; if (x1 + pdx > dw) pdx = dw - x1; if (y0 + pdy < 0) pdy = -y0; if (y1 + pdy > dh) pdy = dh - y1;
    for (int i = 0; i < n; i++) { v[i]->x += pdx; v[i]->y += pdy; }
    if (set == 0 && dir == 1 && g.ir.w > 0) { g.idle_x = g.ir.x + g.ir.w / 2; g.idle_y = g.ir.y + g.ir.h / 2; }
}

/* ---- 배치. mode 0 키보드: 키 한 변 = 짧은 변의 15%. mode 1 키보드+마우스: 11%. dir: 0 십자, 1 아날로그 */
static void layout(int dw, int dh, int mode, int dir, Geo & g) {
    g.on = false; if (dw < dh || dh < 240) return;
    for (int k = 0; k < NK; k++) g.r[k] = Rect{ 0, 0, 0, 0 };
    int sh = imin(dw, dh), m = sh * 34 / 1000, s;
    if (mode == 0) {
        s = sh * 15 / 100; if (s < 36) return;
        int cyc = dh - m - s * 3 / 2, cxl = m + s * 3 / 2, cxr = dw - m - s * 3 / 2;
        if (dir == 0) {
            g.r[T_UP] = Rect{ cxl - s / 2, cyc - s * 3 / 2, s, s }; g.r[T_DOWN] = Rect{ cxl - s / 2, cyc + s / 2, s, s };
            g.r[T_LEFT] = Rect{ cxl - s * 3 / 2, cyc - s / 2, s, s }; g.r[T_RIGHT] = Rect{ cxl + s / 2, cyc - s / 2, s, s };
        }
        g.r[T_SPACE] = Rect{ cxr - s / 2, cyc - s * 3 / 2, s, s }; g.r[T_ESC] = Rect{ cxr - s / 2, cyc + s / 2, s, s };
        g.r[T_TAB] = Rect{ cxr - s * 3 / 2, cyc - s / 2, s, s }; g.r[T_ENTER] = Rect{ cxr + s / 2, cyc - s / 2, s, s };
    } else {
        s = sh * 11 / 100; int pitch = s + sh * 214 / 10000; if (s < 28) return;
        int cyc = dh * 62 / 100, cxl = m + s * 3 / 2;
        if (dir == 0) {
            g.r[T_UP] = Rect{ cxl - s / 2, cyc - s * 3 / 2, s, s }; g.r[T_DOWN] = Rect{ cxl - s / 2, cyc + s / 2, s, s };
            g.r[T_LEFT] = Rect{ cxl - s * 3 / 2, cyc - s / 2, s, s }; g.r[T_RIGHT] = Rect{ cxl + s / 2, cyc - s / 2, s, s };
        }
        int ccx = dw / 2 - sh * 15 / 100, ccy = dh * 742 / 1000, x0 = ccx - pitch / 2 - s / 2, x1 = ccx + pitch / 2 - s / 2, y0 = ccy - pitch / 2 - s / 2, y1 = ccy + pitch / 2 - s / 2;
        g.r[T_SPACE] = Rect{ x0, y0, s, s }; g.r[T_TAB] = Rect{ x1, y0, s, s }; g.r[T_ESC] = Rect{ x0, y1, s, s }; g.r[T_ENTER] = Rect{ x1, y1, s, s };
        int ry = dh * 915 / 1000 - s / 2, rx = dw - m - s;
        g.r[T_R] = Rect{ rx, ry, s, s }; g.r[T_L] = Rect{ rx - s - sh * 214 / 10000, ry, s, s };
    }
    g.ks = s; g.stick_r = dh * 13 / 100; g.idle_x = dw * 11 / 100; g.idle_y = dh * 62 / 100;
    g.ir = Rect{ g.idle_x - g.stick_r, g.idle_y - g.stick_r, g.stick_r * 2, g.stick_r * 2 };
    Lay & L = g_lay[mode ? 1 : 0];
    if (L.hid & 1) for (int k = 0; k < 4; k++) g.r[k] = Rect{ 0, 0, 0, 0 };   /* t863: 숨김 */
    if (L.hid & 2) g.ir = Rect{ 0, 0, 0, 0 };
    g_ldw = dw; g_ldh = dh; g_lks = s;
    g.nx = L.nx; for (int i = 0; i < L.nx; i++) g.xr[i] = Rect{ dw / 2 - s / 2, dh * 22 / 100 - s / 2, s, s };
    for (int set = 0; set < NSET + L.nx; set++) {
        Off * o = set < NSET ? &L.o[set] : (Off *)&L.x[set - NSET].dx;
        int pdx = o->dx * dw / 1000, pdy = o->dy * dh / 1000;
        set_shift(g, set, dir, dw, dh, pdx, pdy);
        o->dx = pdx * 1000 / dw; o->dy = pdy * 1000 / dh;   /* 가둔 만큼 되돌려 저장 */
    }
    g.on = true;
}

/* ---- 해상도별 저장 파일: "L WxH mode set dx dy" / "X WxH mode 라벨 dx dy" (비율이라 해상도가 달라도 쓸 수 있다) */
static char lc_(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static bool seq_(const char * a, const char * b, bool ci) { while (*a && *b) { char x = *a++, y = *b++; if (ci) { x = lc_(x); y = lc_(y); } if (x != y) return false; } return *a == *b; }
static int lab_index(const char * s) {   /* t863: "라벨" 또는 "라벨#n"(같은 라벨이 여러 개일 때 n 번째). 옛 대소문자 라벨도 읽는다 */
    char b[32]; snprintf(b, sizeof b, "%s", s); int nth = 1; char * h = strrchr(b, '#');
    if (h && h != b && h[1] >= '0' && h[1] <= '9') { nth = atoi(h + 1); *h = 0; }
    for (int ci = 0; ci < 2; ci++) { int c = nth; for (int i = 0; i < g_nlab; i++) if (seq_(g_lab[i], b, ci != 0) && --c == 0) return i; }
    return -1;
}
static void lab_name(int ix, char * out, int n) { int c = 0; for (int i = 0; i <= ix; i++) if (!strcmp(g_lab[i], g_lab[ix])) c++; if (c > 1) snprintf(out, n, "%s#%d", g_lab[ix], c); else snprintf(out, n, "%s", g_lab[ix]); }
static void parse_file(const char * path, int w, int h, bool fallback, bool & found) {
    found = false; FILE * f = fopen(path, "r"); if (!f) return;
    char line[160]; int fw = 0, fh = 0;
    for (int pass = 0; pass < 2 && !found; pass++) {   /* pass 0: 현재 해상도, pass 1: 파일의 첫 해상도(내보낸 다른 기기) */
        if (pass == 1 && !fallback) break; rewind(f); int lw = 0, lh = 0; fw = fh = 0;
        for (int m = 0; m < 2; m++) { g_lay[m].nx = 0; g_lay[m].hid = 0; for (int s = 0; s < NSET; s++) g_lay[m].o[s] = Off{ 0, 0 }; }
        while (fgets(line, sizeof line, f)) {
            char t; int a, b, mo, st, dx, dy; char lb[24];
            if (sscanf(line, "L %dx%d %d %d %d %d", &a, &b, &mo, &st, &dx, &dy) == 6) {
                if (pass == 0 && (a != w || b != h)) continue; if (pass == 1) { if (!fw) { fw = a; fh = b; } if (a != fw || b != fh) continue; }
                if (mo >= 0 && mo < 2 && st >= 0 && st < NSET) { g_lay[mo].o[st] = Off{ dx, dy }; found = true; }
            } else if (sscanf(line, "X %dx%d %d %23s %d %d", &a, &b, &mo, lb, &dx, &dy) == 6) {
                if (pass == 0 && (a != w || b != h)) continue; if (pass == 1) { if (!fw) { fw = a; fh = b; } if (a != fw || b != fh) continue; }
                int ix = lab_index(lb); if (mo >= 0 && mo < 2 && ix >= 0 && g_lay[mo].nx < MAXX) { g_lay[mo].x[g_lay[mo].nx++] = Extra{ ix, dx, dy }; found = true; }
            }
            if (line[0] == 'H' && sscanf(line, "H %dx%d %d %d", &a, &b, &mo, &st) == 4) {   /* t863: 숨김 줄 */
                if (pass == 0 && (a != w || b != h)) continue; if (pass == 1) { if (!fw) { fw = a; fh = b; } if (a != fw || b != fh) continue; }
                if (mo >= 0 && mo < 2) { g_lay[mo].hid = st & 3; found = true; }
            }
            (void)t; (void)lw; (void)lh;
        }
    }
    fclose(f);
}
static void load_layout(int w, int h) {   /* 현재 해상도 배치를 파일에서 읽는다(없으면 기본) */
    bool fd = false; for (int m = 0; m < 2; m++) { g_lay[m].nx = 0; g_lay[m].hid = 0; for (int s = 0; s < NSET; s++) g_lay[m].o[s] = Off{ 0, 0 }; }
    if (g_path[0]) parse_file(g_path, w, h, false, fd);
    g_curw = w; g_curh = h; g_ver++;
}
static bool save_layout(int w, int h) {   /* 다른 해상도 줄은 그대로 두고 현재 해상도 줄만 바꿔 쓴다 */
    if (!g_path[0]) return false;
    static char keep[24000]; int kn = 0; keep[0] = 0;
    FILE * f = fopen(g_path, "r");
    if (f) { char line[160]; while (fgets(line, sizeof line, f)) { int a, b; char c = line[0]; if ((c == 'L' || c == 'X' || c == 'H') && sscanf(line + 2, "%dx%d", &a, &b) == 2 && a == w && b == h) continue; int n = (int)strlen(line); if (kn + n < (int)sizeof keep - 1) { memcpy(keep + kn, line, n); kn += n; keep[kn] = 0; } } fclose(f); }
    f = fopen(g_path, "w"); if (!f) return false;
    fputs(keep, f);
    for (int m = 0; m < 2; m++) {
        for (int s = 0; s < NSET; s++) fprintf(f, "L %dx%d %d %d %d %d\n", w, h, m, s, g_lay[m].o[s].dx, g_lay[m].o[s].dy);
        if (g_lay[m].hid) fprintf(f, "H %dx%d %d %d\n", w, h, m, g_lay[m].hid);
        for (int i = 0; i < g_lay[m].nx; i++) { int ix = g_lay[m].x[i].idx; if (ix >= 0 && ix < g_nlab) { char nm[32]; lab_name(ix, nm, sizeof nm); fprintf(f, "X %dx%d %d %s %d %d\n", w, h, m, nm, g_lay[m].x[i].dx, g_lay[m].x[i].dy); } }
    }
    fclose(f); return true;
}
static void say(const char * s) { snprintf(g_msg, sizeof g_msg, "%s", s); g_msg_until = g_now + 2200; g_ver++; }
static void do_export(int w, int h) {
    save_layout(w, h); FILE * a = fopen(g_path, "rb"); if (!a) { say("EXPORT FAIL"); return; }
    FILE * b = g_expath[0] ? fopen(g_expath, "wb") : 0; if (!b && g_expath2[0]) b = fopen(g_expath2, "wb"); if (!b) { fclose(a); say("EXPORT FAIL"); return; }
    char buf[2048]; size_t n; while ((n = fread(buf, 1, sizeof buf, a)) > 0) fwrite(buf, 1, n, b); fclose(a); fclose(b); say("EXPORTED");
}
static void do_import(int w, int h) {
    bool fd = false; const char * ps[2] = { g_expath, g_expath2 };
    for (int i = 0; i < 2 && !fd; i++) if (ps[i][0]) parse_file(ps[i], w, h, true, fd);
    if (!fd) { load_layout(w, h); say("NO FILE"); return; }
    save_layout(w, h); g_ver++; say("IMPORTED");
}
static void reset_layout(int mode) { g_lay[mode ? 1 : 0].nx = 0; g_lay[mode ? 1 : 0].hid = 0; for (int s = 0; s < NSET; s++) g_lay[mode ? 1 : 0].o[s] = Off{ 0, 0 }; g_ver++; }

/* ---- 눌림 ---- */
static void fire_any(int k, bool d) {
    if (k < NK) { if (g_fire) g_fire(k, d); return; }
    int slot = k - NK; Lay & L = g_lay[g_curmode ? 1 : 0]; if (slot >= 0 && slot < L.nx && g_firex) g_firex(L.x[slot].idx, d);
}
static void press(int k) { if (g_cnt[k]++ == 0) { g_mask = g_mask | (1u << k); fire_any(k, true); } }
static void release(int k) { if (g_cnt[k] > 0 && --g_cnt[k] == 0) { g_mask = g_mask & ~(1u << k); fire_any(k, false); } }
static int hit(const Geo & g, int x, int y) {
    for (int k = 0; k < NK; k++) if (g.r[k].w > 0 && in_r(g.r[k], x, y)) return k;
    for (int i = 0; i < g.nx; i++) if (in_r(g.xr[i], x, y)) return NK + i;
    return -1;
}
static bool owned(void) { for (int i = 0; i < 8; i++) if (g_p[i].act) return true; return false; }
static unsigned stick_mask(int dx, int dy, int r) {
    int t = r * 35 / 100; unsigned m = 0;
    if (dx < -t) m |= 1u << T_LEFT; if (dx > t) m |= 1u << T_RIGHT; if (dy < -t) m |= 1u << T_UP; if (dy > t) m |= 1u << T_DOWN;
    return m;
}
static void stick_apply(Ptr & p, unsigned nm) {
    unsigned om = (unsigned)p.key;
    for (int k = 0; k < 4; k++) { bool a = (om >> k) & 1u, b = (nm >> k) & 1u; if (a && !b) release(k); else if (!a && b) press(k); }
    p.key = (int)nm;
}
static void release_all(void) {
    for (int i = 0; i < 8; i++) if (g_p[i].act) { if (g_p[i].kind == 0 && g_p[i].key >= 0) release(g_p[i].key); else if (g_p[i].kind == 1) stick_apply(g_p[i], 0); g_p[i].act = false; }
    g_stk_on = 0;
}

/* ---- 편집 화면 기하: 위 막대 버튼 5개(SAVE RESET ADD EXPORT IMPORT), 고르기 판 */
enum { NB = 5 };
static Rect bar_rect(int i, int dw, int dh) { int bw = imin(dw / 6, dh * 30 / 100), bh = dh * 8 / 100, gap = bw / 10, tot = NB * bw + (NB - 1) * gap; return Rect{ (dw - tot) / 2 + i * (bw + gap), dh / 100, bw, bh }; }
static Rect menu_rect(int i, int dw, int dh) { int bw = imin(dw / 4, dh * 45 / 100), bh = dh * 10 / 100, gap = bh / 3, tot = 3 * bh + 2 * gap; return Rect{ (dw - bw) / 2, (dh - tot) / 2 + i * (bh + gap), bw, bh }; }   /* t863: 추가 메뉴 3칸 */
static int find_set(const Geo & g, int dir, int x, int y) {   /* 손가락 아래 세트(추가 키 먼저) */
    Geo gg = g; Rect * v[12]; int n;
    for (int set = NSET + g.nx - 1; set >= 0; set--) { set_members(gg, set, dir, v, n); for (int i = 0; i < n; i++) if (in_r(*v[i], x, y)) return set; }
    return -1;
}
static bool pick_key(int ix) {   /* t863: 화면 키보드에서 누른 키를 새 버튼으로 추가(호스트 kbov 가 부름). true = 소비 */
    if (!g_edit || g_pick != 2) return false;
    Lay & L = g_lay[g_curmode ? 1 : 0];
    if (ix >= 0 && ix < g_nlab && L.nx < MAXX && g_ldw > 0 && g_ldh > 0) { int st = g_lks * 115 / 100; L.x[L.nx] = Extra{ ix, ((L.nx % 5) - 2) * st * 1000 / g_ldw, (L.nx / 5) * st * 1000 / g_ldh }; L.nx++; save_layout(g_ldw, g_ldh); say("KEY ADDED"); }
    g_pick = 0; g_ver++; if (g_kbshow) g_kbshow(false);
    return true;
}
static bool pointer_edit(const Geo & g, int mode, int dir, int dw, int dh, long long id, int phase, int x, int y) {
    g_curmode = mode;
    int si = -1; for (int i = 0; i < 8; i++) if (g_p[i].act && g_p[i].id == id) si = i;
    if (phase == 0) {
        if (si >= 0) g_p[si].act = false;
        int kind = -1, key = -1;
        if (g_pick == 2) kind = 5;   /* 화면 키보드로 고르는 중: 키보드 밖을 누르면 취소 */
        else if (g_pick == 1) { kind = 4; for (int i = 0; i < 3; i++) if (in_r(menu_rect(i, dw, dh), x, y)) key = i; }
        else { for (int b = 0; b < NB; b++) if (in_r(bar_rect(b, dw, dh), x, y)) { kind = 3; key = b; }
               if (kind < 0) { int st = find_set(g, dir, x, y); if (st >= 0) { kind = 2; key = st; } } }
        for (int i = 0; i < 8; i++) if (!g_p[i].act) { int ox = 0, oy = 0; if (kind == 2) { Lay & L0 = g_lay[mode ? 1 : 0]; Off * o0 = key < NSET ? &L0.o[key] : (key - NSET < L0.nx ? (Off *)&L0.x[key - NSET].dx : 0); if (o0) { ox = o0->dx; oy = o0->dy; } } g_p[i] = Ptr{ true, id, kind, key, x, y, 0, ox, oy }; return true; }
        return true;
    }
    if (si < 0) return true;
    Ptr & p = g_p[si];
    if (phase == 2) {
        if (p.kind == 2 && p.key >= 0) {
            int ddx = x - p.cx, ddy = y - p.cy; int mvn = iabs(ddx) + iabs(ddy); if (mvn > p.mv) p.mv = mvn;
            Lay & L = g_lay[mode ? 1 : 0]; Off * o = p.key < NSET ? &L.o[p.key] : (p.key - NSET < L.nx ? (Off *)&L.x[p.key - NSET].dx : 0);
            if (o) { o->dx = p.ox + ddx * 1000 / dw; o->dy = p.oy + ddy * 1000 / dh; g_ver++; }
        }
        return true;
    }
    int kind = p.kind, key = p.key, mv = p.mv; p.act = false;
    if (kind == 3 && key >= 0 && in_r(bar_rect(key, dw, dh), x, y)) {
        if (key == 0) { save_layout(dw, dh); g_edit = 0; g_pick = 0; g_ver++; say("SAVED"); }
        else if (key == 1) { reset_layout(mode); save_layout(dw, dh); say("RESET"); }
        else if (key == 2) { g_pick = 1; g_ver++; }   /* t863: +ADD 메뉴(키보드 · 아날로그 · 십자) */
        else if (key == 3) do_export(dw, dh);
        else if (key == 4) do_import(dw, dh);
    } else if (kind == 5) {   /* 키 고르기 취소 */
        g_pick = 0; g_ver++; if (g_kbshow) g_kbshow(false);
    } else if (kind == 4) {
        Lay & L = g_lay[mode ? 1 : 0];
        if (key >= 0 && in_r(menu_rect(key, dw, dh), x, y)) {
            if (key == 0) { if (L.nx >= MAXX) { say("MAX KEYS"); g_pick = 0; } else { g_pick = 2; if (g_kbshow) g_kbshow(true); } }
            else { L.hid ^= (key == 1 ? 2 : 1); save_layout(dw, dh); g_pick = 0; say(key == 1 ? ((L.hid & 2) ? "ANALOG OFF" : "ANALOG ON") : ((L.hid & 1) ? "DPAD OFF" : "DPAD ON")); }
        } else g_pick = 0;
        g_ver++;
    } else if (kind == 2 && mv >= 12) { save_layout(dw, dh);
    } else if (kind == 2 && key >= NSET && mv < 12) {   /* 추가 키를 끌지 않고 짧게 누르면 지운다 */
        Lay & L = g_lay[mode ? 1 : 0]; int slot = key - NSET; if (slot >= 0 && slot < L.nx) { for (int i = slot; i + 1 < L.nx; i++) L.x[i] = L.x[i + 1]; L.nx--; g_ver++; say("KEY REMOVED"); save_layout(dw, dh); }
    }
    return true;
}

/* phase 0 down, 1 up, 2 move. dir 1 이면 화면 왼쪽 절반의 첫 터치가 스틱 중심. wall = 오버레이 위에서 시작 = 안 받음. true = 소비 */
static bool pointer(const Geo & g, int mode, int dir, int dw, int dh, long long id, int phase, int x, int y, bool wall) {
    g_curmode = mode;
    if (g_edit) return pointer_edit(g, mode, dir, dw, dh, id, phase, x, y);
    int si = -1; for (int i = 0; i < 8; i++) if (g_p[i].act && g_p[i].id == id) si = i;
    if (phase == 0) {
        if (wall) return false;
        int k = hit(g, x, y); int kind = 0;
        if (k < 0) { if (dir == 1 && !(g_lay[mode ? 1 : 0].hid & 2) && x < dw / 2 && y > dh / 5 && !g_stk_on) kind = 1; else return false; }
        if (si >= 0) { if (g_p[si].kind == 0 && g_p[si].key >= 0) release(g_p[si].key); else if (g_p[si].kind == 1) stick_apply(g_p[si], 0); g_p[si].act = false; }
        for (int i = 0; i < 8; i++) if (!g_p[i].act) {
            g_p[i] = Ptr{ true, id, kind, kind == 0 ? k : 0, x, y, 0 };
            if (kind == 0) press(k); else { g_stk_on = 1; g_stk_x = x; g_stk_y = y; g_stk_dx = x; g_stk_dy = y; }
            return true;
        }
        return false;
    }
    if (si < 0) return false;
    Ptr & p = g_p[si];
    if (p.kind == 1) {
        if (phase == 2) { int dx = x - p.cx, dy = y - p.cy; g_stk_dx = x; g_stk_dy = y; if (g_stk_mouse) { stick_apply(p, 0); float fx = (float)dx / (float)g.stick_r, fy = (float)dy / (float)g.stick_r; if (fx > 1.0f) fx = 1.0f; if (fx < -1.0f) fx = -1.0f; if (fy > 1.0f) fy = 1.0f; if (fy < -1.0f) fy = -1.0f; g_stk_fx = fx; g_stk_fy = fy; }   /* t862f */
            else stick_apply(p, stick_mask(dx, dy, g.stick_r)); }
        else { stick_apply(p, 0); g_stk_fx = 0.0f; g_stk_fy = 0.0f; g_stk_on = 0; p.act = false; }
        return true;
    }
    if (phase == 2) { int nk = hit(g, x, y); if (nk != p.key) { if (p.key >= 0) release(p.key); p.key = nk; if (nk >= 0) press(nk); } return true; }
    if (p.key >= 0) release(p.key);
    p.act = false; p.key = -1; return true;
}
static void tick(unsigned now) { g_now = now; if (g_msg[0] && (int)(now - g_msg_until) >= 0) { g_msg[0] = 0; g_ver++; } }
}   /* namespace tlay */
/* t862c: 실제 쓰는 모드 = 설정이 키보드+마우스 이고 게임이 마우스를 쓸 때만 1. 마우스를 안 쓰는 게임은 자동으로 키보드 모드 */
#ifndef TLAY_MODE
#define TLAY_MODE() (gApp.ui_.g.touchmode == 1 && gApp.mouse_game ? 1 : 0)
#define TLAY_DIR() (gApp.ui_.g.touchmode ? gApp.ui_.g.touchdir2 : gApp.ui_.g.touchdir)
#endif

