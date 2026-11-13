#pragma once
/* kh_settings.h -- S5: ui::Settings 구조체·옛 형식 변환(fx_*)·settings.ini 한 구역 읽기/쓰기(ini_put·ini_set). kh_ui.h 에서 코드 그대로 옮김, 네임스페이스 ui:: 유지(이름 정리는 S7) */
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
namespace ui {
/* ---- 설정 (전역 + 작품별 덮어쓰기) */
struct Settings {
    int ratio = 0;      /* 0 4:3  1 16:10  2 정수배 */
    int shader = 0;     /* t48c: 필터 0 sharp 2 nearest. 1 = 옛 crt 프리셋: 파일에서 읽을 때만 나오며 「sharp + 효과 켬」으로 바뀐다 */
    int curve = 0;      /* 0..10 → 곡면 0.00..0.10 */
    int scan = 0;      /* t48c: 주사선 0..10 (0 = 끔, 1..10 → 옛 SCANLINE 2+n) */       /* 0..10 */
    int mask = 0;      /* t48c: 마스크 0..10 (0 = 끔, n → 옛 CGWG n*0.05) */       /* 0 꺼짐 1 약 2 중 3 강 */
    int glow = 0;       /* t48c (#625): 빛 번짐 0..10 (0 = 끔) */
    int bleed = 0;      /* t48c: 이제 「RGB 번짐」 0..10 (컨버전스 어긋남 + 가로 번짐). 옛 「번짐」은 RGB 번짐 + 빛 번짐 같은 값으로 변환 */      /* t42(#311): 0..10 → 번짐(실물 RGB 모니터 결함: 컨버전스·가로 번짐·형광체 번짐) 0.0..1.0, 0 = 끔 */
    int bgm = 8;        /* 0..8 */
    int se = 8;         /* 0..8 */
    int device = 0;     /* 0 기본 장치 */
    int pad = 0;        /* 0 일본식 ○결정 */
    int bmult = 1;      /* 0..4 = x2 x3 x4 x6 x8 */
    int bmode = 2;      /* 0 홀드 1 토글 2 둘 다 */
    int boot = 0;       /* 0 켬 1 끔 */
    int bosd = 1;       /* 배속 표시(오른쪽 위 ▶▶) 0 끔 1 켬 */
    int fpsd = 0;
    int cpuspd = 0;   /* t665: 0 표준 1 빠름 2 매우빠름 3 최대 */       /* FPS / speed display 0 off 1 on (ini key fpsdisp) */
    int stk_l = 0, stk_r = 2, dz_l = 0, dz_r = 0, stk_sp = 2;   /* #624: stick role 0 off 1 d-pad 2 mouse (left default off, right default mouse); deadzone level 0 default .. 3 largest; mouse speed 1 slow .. 3 fast */
    int bgfx = 1, bgspd = 0, bgbri = 1;   /* kx-bg: menu background effect (global only): 0 off 1 stars 2 twinkle 3 warp 4 noise 5-12 bars,sine,liss,tunnel,rain,speed,plasma,moire / speed / brightness */
    int phos = 0;       /* t662 (#635): phosphor bundle: beam width (bits 0-1) | breathing << 4 | glow << 6 | persistence << 8 (3 bits, 0..5 = off/8/12/18/27/40 ms; #S1). Old files kept persistence in bits 2-3 (0..3 = off/40/90/160 ms) — phos_norm() moves it. 0 = all off */
    static int phos_norm(int v) {   /* #S1: clamp + move an old-format persistence (bits 2-3) to bits 8-10: old 40 -> 27 ms, old 90/160 -> 40 ms */
        v = v < 0 ? 0 : v > 2047 ? 2047 : v; int lg = (v >> 2) & 3;
        if (lg) { v &= ~(3 << 2); if (((v >> 8) & 7) == 0) v |= (lg == 1 ? 4 : 5) << 8; }
        return v;
    }
    int kbshader = 0;   /* t46z (#548): 화면 키보드·버튼·배속·OSD 에 셰이더 적용 0 안 함(기본) 1 적용 */
    int kbbtn = 1;      /* t46x: 화면 키보드 토글 버튼 표시 0 숨김 1 표시 */
#if defined(__ANDROID__)
    int ovbtn = 1;      /* t52f (#132): 오버레이 버튼 줄(메뉴·저장·불러오기·배속·키보드) 표시 0 숨김 1 표시 */
#else
    int ovbtn = 0;      /* t52f (#132): 윈도 기본 숨김, 안드 표시 유지 0 숨김 1 표시 */
#endif
    int ovpos_mode = 0;   /* E2R: 0 align 1 free */
    int ovpos_edge = 0;   /* E2R: 0 bottom 1 top 2 left 3 right */
    int ovpos_align = 0;  /* E2R: horiz 0 left 1 mid 2 right, vert 0 top 1 mid 2 bot */
    int ovpos_free = 0;   /* E2R: free pos 0..100 */
    int padbtn = 2;     // virtual pad in the side bands: 0 off, 1 on, 2 auto (on with a touch screen and no real pad)
    int bsound = 2;     /* r017 배속 중 소리: 0 끔 1 빠르게(음높이 상승) 2 음높이 유지 */
    int dsnd = 0;       /* #641: disk drive sound 0 off 1 seek only 2 on (default off) */
    int dvol = 4;       /* #641: disk sound volume 0..8 */
    int pwrseq = 1;   /* t667: 0 끔 1 켬(기본 켬) */
    int hddhum = 1;   /* t673: 0 끔 1 게임 중만 2 메뉴 포함(기본 1) */
    int hvol = 3;     /* t667: 0..8(기본 3) */
    int mclick = 1;   /* E1R: 0 끔 1 켬(기본 켬, 전역 전용) */
    int touchfx = 1;   /* kx-touch: 터치 표시 0 끔 1 도트(기본 켬, 전역 전용) */
    int tfxsz = 0;     /* kx-touch: 터치 표시 크기 0 작게 1 크게 */
    int tfxmouse = 0;  /* kx-touch: 윈도 마우스 클릭에도 표시 0 끔(기본) 1 켬 */
    int tmouse = 0;    /* kx-touch: 게임 마우스 터치 방식 0 트랙패드(기본) 1 절대 위치 */
    int shim = 2;       /* build 7: menu text shimmer 0 off 1 per letter 2 per dot (global only) */
    int shim_amp = 4, shim_spd = 3, shim_glow = 6, shim_col = 0;   /* strength 1..10, speed 1..10, glow 0..10, color 0 green 1 amber 2 white 3 cyan */
    bool lmode = false, lseen = false; int l_sh = 0, l_scan = 5, l_mask = 1, l_bleed = 0, l_curve = 4;   /* t48c: 옛 형식(fx=1 표지 없음) 파일을 읽는 동안만 쓰는 칸. 끝나면 fx_finish 가 새 칸으로 옮긴다 */
    float curve_f() const { return curve * 0.01f; }
    float glow_f() const { return glow * 0.1f; }
    int ph_beam() const { return phos & 3; }  int ph_pers() const { return (phos >> 8) & 7; }  int ph_breath() const { return (phos >> 4) & 3; }  int ph_glow() const { return (phos >> 6) & 3; }
    float bleed_f() const { return bleed * 0.1f; }
    float scan_f() const { return scan / 10.0f; }
    float mask_f() const { return (mask < 0 ? 0 : mask > 10 ? 10 : mask) * 0.05f; }   /* t48c: 옛 마스크 1/2/3 = 0.15/0.3/0.5 = 새 3/6/10 단계 */
    int boost_x() const { static const int x[5] = { 2, 3, 4, 6, 8 }; return x[bmult < 0 ? 0 : bmult > 4 ? 4 : bmult]; }
    int bs_gain256() const { return boost_x() >= 3 ? 205 : 256; }   /* x3 이상은 조금 낮춤(-2 dB) */
    float se_gain() const { return se / 8.0f; }
    float bgm_gain() const { return bgm / 8.0f; }
};
/* t48c (#639 ①): 옛 설정(shader=1 crt 프리셋 + scan/mask/bleed) → 새 설정(필터 + 효과). 같은 그림이 나오게 옮긴다 */
static inline int fx_mask_leg(int m) { static const int t[4] = { 0, 3, 6, 10 }; return t[m < 0 ? 0 : m > 3 ? 3 : m]; }
inline void fx_begin(Settings & s) { s.lmode = true; s.lseen = false; s.l_sh = 0; s.l_scan = 5; s.l_mask = 1; s.l_bleed = 0; s.l_curve = 4; }
inline void fx_finish(Settings & s) {
    if (!s.lmode) return; s.lmode = false; if (!s.lseen) return; s.lseen = false;
    if (s.l_sh == 1) { s.shader = 0; s.scan = s.l_scan < 1 ? 1 : s.l_scan; s.mask = fx_mask_leg(s.l_mask); s.bleed = s.l_bleed; s.glow = s.l_bleed; s.curve = s.l_curve; }   /* 옛 번짐 = RGB 번짐 + 빛 번짐 같은 값(가장자리 가중 컨버전스 포함, 화소 동일) */
    else { s.shader = (s.l_sh == 2) ? 2 : 0; s.scan = 0; s.mask = 0; s.bleed = 0; s.glow = 0; s.curve = 0; }   /* 옛 sharp/nearest 에서는 효과가 안 먹었다 */
}

/* ---- 설정 파일 (settings.ini) -- 전역 [global] + 작품별 [id] */
static void ini_put(FILE * f, const Settings & s) {
    fprintf(f, "fx=1\nratio=%d\nshader=%d\ncurve=%d\nbleed=%d\nglow=%d\nscan=%d\nmask=%d\nbgm=%d\nse=%d\ndevice=%d\npad=%d\nbmult=%d\nbmode=%d\nboot=%d\nbosd=%d\nbsound=%d\nkbbtn=%d\nkbshader=%d\novbtn=%d\npadbtn=%d\n", s.ratio, (s.shader == 2 ? 2 : 0), s.curve, s.bleed, s.glow, s.scan, s.mask, s.bgm, s.se, s.device, s.pad, s.bmult, s.bmode, s.boot, s.bosd, s.bsound, s.kbbtn, s.kbshader, s.ovbtn, s.padbtn);
    fprintf(f, "shim=%d\nshim_amp=%d\nshim_spd=%d\nshim_glow=%d\nshim_col=%d\n", s.shim, s.shim_amp, s.shim_spd, s.shim_glow, s.shim_col);
    fprintf(f, "sk_l=%d\nsk_r=%d\ndz_l=%d\ndz_r=%d\nsk_sp=%d\n", s.stk_l, s.stk_r, s.dz_l, s.dz_r, s.stk_sp);
    fprintf(f, "phos=%d\n", s.phos);
    fprintf(f, "bgfx=%d\nbgspd=%d\nbgbri=%d\n", s.bgfx, s.bgspd, s.bgbri);
    fprintf(f, "dsnd=%d\ndvol=%d\n", s.dsnd, s.dvol);
    fprintf(f, "pwrseq=%d\nhddhum=%d\nhvol=%d\n", s.pwrseq, s.hddhum, s.hvol);   /* t667 */
    fprintf(f, "mclick=%d\n", s.mclick);   /* E1R */
    fprintf(f, "touchfx=%d\ntfxsz=%d\ntfxmouse=%d\ntmouse=%d\n", s.touchfx, s.tfxsz, s.tfxmouse, s.tmouse);   /* kx-touch */
    fprintf(f, "cpuspd=%d\n", s.cpuspd);   /* t665 */
    fprintf(f, "ovpos_mode=%d\novpos_edge=%d\n", s.ovpos_mode, s.ovpos_edge);   /* E2R */
    fprintf(f, "ovpos_align=%d\novpos_free=%d\n", s.ovpos_align, s.ovpos_free);   /* E2R */
    fprintf(f, "fpsdisp=%d\n", s.fpsd ? 1 : 0);   /* 0 도 쓴다: 작품 구역은 전역 값을 먼저 복사하므로 줄이 없으면 전역 켬이 새어 든다 */
}
static void ini_put_masked(FILE * f, const Settings & s, unsigned m) {   /* #587: 작품 구역에는 바꾼 줄만 쓴다 (비트 = Ui::fbit_of 순서) */
    if (m & (1u << 0)) fprintf(f, "ratio=%d\n", s.ratio);
    if (m & (1u << 1)) fprintf(f, "shader=%d\n", s.shader == 2 ? 2 : 0);
    if (m & (1u << 2)) fprintf(f, "curve=%d\n", s.curve);
    if (m & (1u << 3)) fprintf(f, "bleed=%d\n", s.bleed);
    if (m & (1u << 4)) fprintf(f, "glow=%d\n", s.glow);
    if (m & (1u << 5)) fprintf(f, "scan=%d\n", s.scan);
    if (m & (1u << 6)) fprintf(f, "mask=%d\n", s.mask);
    if (m & (1u << 7)) fprintf(f, "bgm=%d\n", s.bgm);
    if (m & (1u << 8)) fprintf(f, "se=%d\n", s.se);
    if (m & (1u << 9)) fprintf(f, "pad=%d\n", s.pad);
    if (m & (1u << 10)) fprintf(f, "bmult=%d\n", s.bmult);
    if (m & (1u << 11)) fprintf(f, "bmode=%d\n", s.bmode);
    if (m & (1u << 12)) fprintf(f, "boot=%d\n", s.boot);
    if (m & (1u << 13)) fprintf(f, "bosd=%d\n", s.bosd);
    if (m & (1u << 14)) fprintf(f, "bsound=%d\n", s.bsound);
    if (m & (1u << 15)) fprintf(f, "ovbtn=%d\n", s.ovbtn);
    if (m & (1u << 17)) fprintf(f, "sk_l=%d\n", s.stk_l);
    if (m & (1u << 18)) fprintf(f, "sk_r=%d\n", s.stk_r);
    if (m & (1u << 19)) fprintf(f, "dz_l=%d\n", s.dz_l);
    if (m & (1u << 20)) fprintf(f, "dz_r=%d\n", s.dz_r);
    if (m & (1u << 21)) fprintf(f, "sk_sp=%d\n", s.stk_sp);
    if (m & (1u << 22)) fprintf(f, "phos=%d\n", s.phos);   /* t662 (#635) */
    if (m & (1u << 23)) fprintf(f, "dsnd=%d\n", s.dsnd);   /* #641 */
    if (m & (1u << 24)) fprintf(f, "dvol=%d\n", s.dvol);
    if (m & (1u << 26)) fprintf(f, "pwrseq=%d\n", s.pwrseq);
    if (m & (1u << 27)) fprintf(f, "hddhum=%d\n", s.hddhum);
    if (m & (1u << 28)) fprintf(f, "hvol=%d\n", s.hvol);
    if (m & (1u << 16)) fprintf(f, "fpsdisp=%d\n", s.fpsd ? 1 : 0);
    if (m & (1u << 25)) fprintf(f, "cpuspd=%d\n", s.cpuspd);   /* 0 도 쓴다 (T252) */
}
static bool ini_set(Settings & s, const std::string & k, int v) {
    if (k == "bgfx") { s.bgfx = v < 0 ? 0 : v > 12 ? 12 : v; return true; }
    if (k == "bgspd") { s.bgspd = v < 0 ? 0 : v > 2 ? 2 : v; return true; }
    if (k == "bgbri") { s.bgbri = v < 0 ? 0 : v > 2 ? 2 : v; return true; }
    if (k == "phos") { s.phos = Settings::phos_norm(v); return true; }   /* t662 (#635), #S1: 옛 잔광 칸 이전 */
    if (k == "dsnd") { s.dsnd = v < 0 ? 0 : v > 2 ? 2 : v; return true; }   /* #641 */
    if (k == "dvol") { s.dvol = v < 0 ? 0 : v > 8 ? 8 : v; return true; }
    if (k == "pwrseq") { s.pwrseq = v ? 1 : 0; return true; }
    if (k == "hddhum") { s.hddhum = v < 0 ? 0 : v > 2 ? 2 : v; return true; }
    if (k == "hvol") { s.hvol = v < 0 ? 0 : v > 8 ? 8 : v; return true; }
    if (k == "mclick") { s.mclick = v ? 1 : 0; return true; }   /* E1R */
    if (k == "touchfx") { s.touchfx = v < 0 ? 0 : v > 3 ? 3 : v; return true; }   /* kx-touch */
    if (k == "tfxsz") { s.tfxsz = v < 0 ? 0 : v > 1 ? 1 : v; return true; }
    if (k == "tfxmouse") { s.tfxmouse = v < 0 ? 0 : v > 1 ? 1 : v; return true; }
    if (k == "tmouse") { s.tmouse = v < 0 ? 0 : v > 1 ? 1 : v; return true; }
    if (k == "shim") { s.shim = v < 0 ? 0 : v > 2 ? 2 : v; return true; }   /* build 7: menu text shimmer keys (before the long chain) */
    if (k == "shim_amp") { s.shim_amp = v < 1 ? 1 : v > 10 ? 10 : v; return true; }
    if (k == "shim_spd") { s.shim_spd = v < 1 ? 1 : v > 10 ? 10 : v; return true; }
    if (k == "shim_glow") { s.shim_glow = v < 0 ? 0 : v > 10 ? 10 : v; return true; }
    if (k == "shim_col") { s.shim_col = v < 0 ? 0 : v > 3 ? 3 : v; return true; }
    if (k == "sk_l") { s.stk_l = v < 0 ? 0 : v > 2 ? 2 : v; return true; }   /* #624 */
    if (k == "sk_r") { s.stk_r = v < 0 ? 0 : v > 2 ? 2 : v; return true; }
    if (k == "sk_sp") { s.stk_sp = v < 1 ? 1 : v > 3 ? 3 : v; return true; }
    if (k == "dz_l") { s.dz_l = v < 0 ? 0 : v > 3 ? 3 : v; return true; }
    if (k == "dz_r") { s.dz_r = v < 0 ? 0 : v > 3 ? 3 : v; return true; }
    if (k == "fx") { s.lmode = false; }   /* t48c: 새 형식 표지. 이 구역의 화면 키는 새 범위로 읽는다 */
    else if (s.lmode && (k == "shader" || k == "curve" || k == "bleed" || k == "scan" || k == "mask")) { s.lseen = true; if (k == "shader") s.l_sh = v; else if (k == "curve") s.l_curve = v < 0 ? 0 : v > 10 ? 10 : v; else if (k == "bleed") s.l_bleed = v < 0 ? 0 : v > 10 ? 10 : v; else if (k == "scan") s.l_scan = v < 0 ? 0 : v > 10 ? 10 : v; else s.l_mask = v < 0 ? 0 : v > 3 ? 3 : v; }   /* 옛 형식: 구역이 끝날 때 fx_finish 가 옮긴다 */
    else if (k == "ratio") s.ratio = v; else if (k == "shader") s.shader = (v == 2) ? 2 : 0; else if (k == "curve") s.curve = v < 0 ? 0 : v > 10 ? 10 : v; else if (k == "bleed") s.bleed = v < 0 ? 0 : v > 10 ? 10 : v; else if (k == "glow") s.glow = v < 0 ? 0 : v > 10 ? 10 : v; else if (k == "scan") s.scan = v < 0 ? 0 : v > 10 ? 10 : v; else if (k == "mask") s.mask = v < 0 ? 0 : v > 10 ? 10 : v;
    else if (k == "bgm") s.bgm = v; else if (k == "se") s.se = v; else if (k == "device") s.device = v; else if (k == "pad") s.pad = v; else if (k == "bmult") s.bmult = v; else if (k == "bmode") s.bmode = v; else if (k == "boot") s.boot = v; else if (k == "bosd") s.bosd = v; else if (k == "bsound") s.bsound = v; else if (k == "kbbtn") s.kbbtn = v < 0 ? 0 : v > 1 ? 1 : v; else if (k == "ovbtn") s.ovbtn = v < 0 ? 0 : v > 1 ? 1 : v; else if (k == "padbtn") s.padbtn = v < 0 ? 0 : v > 2 ? 2 : v; else if (k == "kbshader") s.kbshader = v < 0 ? 0 : v > 1 ? 1 : v; else if (k == "fpsdisp") s.fpsd = v ? 1 : 0;
    else if (k == "cpuspd") s.cpuspd = v < 0 ? 0 : v > 3 ? 3 : v;
    if (k == "ovpos_mode") { s.ovpos_mode = v ? 1 : 0; return true; }   /* E2R */
    if (k == "ovpos_edge") { s.ovpos_edge = v < 0 ? 0 : v > 3 ? 3 : v; return true; }   /* E2R */
    if (k == "ovpos_align") { s.ovpos_align = v < 0 ? 0 : v > 2 ? 2 : v; return true; }   /* E2R */
    if (k == "ovpos_free") { s.ovpos_free = v < 0 ? 0 : v > 100 ? 100 : v; return true; }   /* E2R */
    else return false;
    return true;
}

} /* namespace ui */
