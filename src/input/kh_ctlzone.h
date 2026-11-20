#pragma once
/* kh_ctlzone.h -- E(지시 #E): 가로 고정 · 화면 전체 터치패드(kbm) 오버레이 판정 앞단.
 * 설계: 설계_터치조종_1714.md §0(슨상 결정: 가로 강제·우클릭=버튼) + §7(화면 전체 터치패드) + §8(참고 사례).
 * E2(지시 #E2): 12번째 칸 K_TAB 추가(E 문서는 12칸이나 코드가 11칸이어서 맞춤) · 칸 너비
 * 조정(화살표 44·중간 52·L/R 60 유지, 합 608+간격 22=630 ≤ 640, x0=5) · 그리기용 라벨 표 ·
 * 그리기 가시 훅(show_fn)·눌림 미러(g_down). 그 외 E 판정 로직 무변경.
 * 선행 해법(재사용, 이 파일에서 새로 만들지 않음):
 *   - 좌표: gh_kb_win2virt(창→가상 640x400, 곡면 warp 포함) — 이 파일의 win2virt 는 시험용 순수 쌍둥이
 *   - 키 전송: gh_kb_send/KEYBOARD_AddKey, 마우스: kh_vmouse_move_rel/button, 길게-불러오기: kbov::tick
 *   - 메뉴 터치: gh_touch_feed(메뉴 열림時は 게임 경로가 안 탐 — E-6 구조적 우선)
 *   - 기존 트랙패드: gh_tpad_finger(탭=좌·0.2초유지+끌기=드래그)
 * 이 파일 = 순수 판정만(SDL·KEYBOARD 모름). 호스트(app/kbd_event.h)가 콜백·좌표를 잇는다.
 * 터치 외 기능·기본 입력(키보드·게임패드) 경로는 건드리지 않는다. */
#include <stdint.h>

namespace ctlzone {

/* ---- E-1 가로 고정. manifest sensorLandscape 가 강제하므로 세로 분기는 두지 않는다.
 * 혹시 세로 지오메트리가 들어오면 kbm 을 끄는 가드(geom_ok)만 둔다. */
static inline bool landscape_locked(void) { return true; }
static inline bool geom_ok(int dw, int dh) { return dw >= dh && dw > 0 && dh > 0; }

/* ---- E-2/E-3/E-4 상수 (§8 제안 기반. 가정 A1: 방 실측 전까지 이 기본값) */
static const unsigned TAP_MS = 250;    /* 탭 판정 시간(ms). RetroArch식 0.2초 계열 */
static const float SLOP_PX = 16.0f;    /* 탭 이동 허용(가상 640x400 px). 넘으면 드래그/무시 */
static const unsigned HOLD_MS = 200;   /* 누른 채 0.2초 뒤 끌기 = 왼쪽 드래그 유지(기존 gh_tpad_finger 와 같은 값) */
static const float PAD_GAIN = 1.5f;    /* 상대 이동 게인(기존 1.5*640/ww 계열) */
static const int VW = 640, VH = 400;   /* 가상 캔버스(기존 kbov 와 동일) */
static const int KEY_H = 44;           /* 하단 키 줄 높이 */
static int g_key_y = VH - 4 - KEY_H;   /* t860: 오버레이 줄이 아래에 있으면 그 위로 올림(kbov::sync_ky) */
static const int KEY_Y = VH - 4 - KEY_H; /* 하단 키 줄 윗변(ov_btn 줄 자리와 겹치지 않게 4px 띄움) */

/* ---- E-4/E-5 키+버튼 배치 (가상 640x400, 하단 한 줄. 그 밖 화면 전체 = 터치패드) */
/* E2: K_TAB 을 SPACE 다음에 추가해 문서(12칸)와 맞춤. kid 순서는 그리기·전송이 함께 쓴다. */
enum KeyId { K_LEFT = 0, K_RIGHT, K_UP, K_DOWN, K_ENTER, K_ESC, K_SPACE, K_TAB, K_MENU, K_KBD, BTN_L, BTN_R, NKEYS };
struct Rect { int x, y, w, h; };
static inline bool in_rect(const Rect & r, int x, int y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

/* E2: 칸 너비: 화살표 44·중간(Enter·Esc·Spc·Tab·Menu·Kbd) 52·L/R 60, 간격 2.
 * 합 608+22=630 ≤ 640. 가운데 정렬(x0=(640-630)/2=5). L/R 60px 유지(지시서). */
static inline void layout(Rect out[NKEYS]) {
    static const int W[NKEYS] = { 44, 44, 44, 44, 52, 52, 52, 52, 52, 52, 60, 60 };
    static const int GAP = 2;
    int tot = 0; for (int i = 0; i < NKEYS; i++) tot += W[i]; tot += GAP * (NKEYS - 1);
    int x = (VW - tot) / 2;
    for (int i = 0; i < NKEYS; i++) { out[i].x = x; out[i].y = g_key_y; out[i].w = W[i]; out[i].h = KEY_H; x += W[i] + GAP; }
}
static inline int key_at(int vx, int vy) { Rect r[NKEYS]; layout(r); for (int i = 0; i < NKEYS; i++) if (in_rect(r[i], vx, vy)) return i; return -1; }
static inline bool is_mouse_btn(int k) { return k == BTN_L || k == BTN_R; }

/* ---- E2: 그리기용 라벨 (기존 kbov 키 표처럼 ASCII. 화살표는 LT/RT/UP/DN). */
static const char * const LABELS[NKEYS] = {
    "LT", "RT", "UP", "DN", "RET", "ESC", "SPC", "TAB", "MENU", "KBD", "L", "R"
};

/* ---- E-5/E-6 라우터: 메뉴 열림 → 메뉴(게임 경로는 손대지 않음). 키 사각형 먼저, 나머지 전부 터치패드. */
enum Route { R_MENU, R_KEY, R_PAD, R_IGNORE };
static inline Route route(bool menu_open, bool kbm_on, int keyhit) {
    if (menu_open) return R_MENU;
    if (!kbm_on) return R_IGNORE;
    return keyhit >= 0 ? R_KEY : R_PAD;
}

/* ---- E-2/E-3 kbm 활성 조건 (순수). 패드 연결 시 터치 숨김(기존 vpad auto 와 같은 규칙). */
static inline bool want_kbm(bool mouse_game, bool boot_tick, bool menu_open, bool hidden_by_option, int dw, int dh) {   /* t860: 패드 연결 분기 제거 — 4번째 인자 = 설정 「터치 키 표시」 꺼짐 */
    if (!mouse_game || !boot_tick || menu_open || hidden_by_option) return false;
    return geom_ok(dw, dh);
}

/* ---- E2: 그리기 가시 훅. 호스트(app/kbd_event.h)가 gh_ctl_visible 로 채운다.
 * 그리기 파일(ui/kh_kbov_draw.h)은 gApp 을 모르므로 이 함수 포인터로만 묻는다. */
typedef bool (*ShowFn)(void);
static ShowFn g_show_fn = 0;
/* E2: 눌림 미러. 호스트 gh_ctl_fire 가 kid 단위로 쓰고 그리기 파일이 읽는다(눌림 반전용). */
static int g_down[NKEYS];
static inline bool show(void) { return g_show_fn && g_show_fn(); }

/* ---- 좌표: 창(px) → 가상(640x400). 호스트는 gh_kb_win2virt 를 쓰고, 이 함수는 PC 시험용 쌍둥이. */
static inline bool win2virt(int mx, int my, int ww, int wh, int dw, int dh, int rx, int ry, int rw, int rh, int & vx, int & vy) {
    if (ww <= 0 || wh <= 0 || dw <= 0 || dh <= 0 || rw <= 0 || rh <= 0) return false;
    double u = ((double)mx * dw / ww - rx) / rw, v = ((double)my * dh / wh - ry) / rh;
    vx = (int)(u * VW + (u < 0 ? -0.5 : 0.5)); vy = (int)(v * VH + (v < 0 ? -0.5 : 0.5));
    return true;
}

/* ---- E-2/E-3 한 손가락 분류기 (순수. 멀티손가락 수는 호스트가 셈). 기존 gh_tpad_finger 규칙을 값으로 옮김:
 * 탭(시간≤TAP_MS·이동≤SLOP)=좌클릭 · 길게(≥HOLD_MS) 누른 채 끌기=좌 드래그 유지 · 두 손가락 이상 탭=없음(슨상 결정: 1차 제외). */
enum TEv { EV_NONE, EV_MOVE, EV_TAP, EV_HOLD_START, EV_HOLD_END };
struct Finger {
    bool on; unsigned t0; float x0, y0, lx, ly, dist; bool hold; int peak;
    void reset(void) { on = false; t0 = 0; x0 = y0 = lx = ly = 0; dist = 0; hold = false; peak = 0; }
};
static inline TEv down(Finger & f, unsigned now, float x, float y, int fingers) {
    f.on = true; f.t0 = now; f.x0 = x; f.y0 = y; f.lx = x; f.ly = y; f.dist = 0; f.hold = false;
    f.peak = fingers > f.peak ? fingers : f.peak;
    return EV_NONE;
}
static inline TEv move(Finger & f, unsigned now, float x, float y, float & dx, float & dy) {
    dx = 0; dy = 0; if (!f.on) return EV_NONE;
    float px = x - f.lx, py = y - f.ly; f.lx = x; f.ly = y;
    float ax = px < 0 ? -px : px, ay = py < 0 ? -py : py; f.dist += ax + ay;
    if (f.peak > 1) return EV_NONE;   /* 멀티터치는 패드로 안 보냄(1차 제외) */
    if (!f.hold && now - f.t0 >= HOLD_MS && f.dist <= SLOP_PX) { f.hold = true; return EV_HOLD_START; }
    if (f.hold || f.dist > SLOP_PX) { dx = px * PAD_GAIN; dy = py * PAD_GAIN; return EV_MOVE; }
    return EV_NONE;
}
static inline TEv up(Finger & f, unsigned now, int fingers_now) {
    if (!f.on) return EV_NONE;
    f.on = false;
    if (f.hold) { f.hold = false; return EV_HOLD_END; }
    unsigned dt = now - f.t0;
    if (f.peak <= 1 && fingers_now == 0 && dt <= TAP_MS && f.dist <= SLOP_PX) return EV_TAP;  /* E-3 탭=좌클릭 */
    return EV_NONE;   /* 두 손가락 이상 탭 포함: 1차에서 우클릭 없음(§0 슨상 결정) */
}

}   /* namespace ctlzone */
