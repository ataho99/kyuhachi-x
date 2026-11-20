#pragma once
/* screens/tags.h -- t822: 줄 꼬리표 도우미. 출하값 판정(값이 Settings 기본값과 같은가) · 예외 N작품(전역 줄을 따로 쓰는 작품 수). */
inline bool kh_factory_row(bool screen_page, int id, const Settings & s) {
    const Settings d;
    if (screen_page) {
        switch (id) { case 1: return s.ratio == d.ratio; case 2: return s.shader == d.shader; case 3: return s.scan == d.scan; case 4: return s.mask == d.mask;
                      case 5: return s.bleed == d.bleed; case 6: return s.glow == d.glow; case 7: return s.curve == d.curve; case 16: return s.vpos == d.vpos; default: return false; }
    }
    switch (id) {
    case 1: return s.bgm == d.bgm;   case 2: return s.se == d.se;   case 36: return s.vo == d.vo;
    case 5: return s.bmult == d.bmult;   case 6: return s.bmode == d.bmode;   case 7: return s.bsound == d.bsound;
    case 8: return s.boot == d.boot;   case 9: return s.bosd == d.bosd;   case 10: return s.ovbtn == d.ovbtn;
    case 16: return s.dsnd == d.dsnd;   case 17: return s.dvol == d.dvol;   case 18: return s.cpuspd == d.cpuspd;
    case 19: return s.pwrseq == d.pwrseq;   case 20: return s.hddhum == d.hddhum;   case 21: return s.hvol == d.hvol;
    case 22: return s.ovpos_mode == d.ovpos_mode;   case 23: return s.ovpos_edge == d.ovpos_edge;
    case 24: return s.ovpos_align == d.ovpos_align;   case 25: return s.ovpos_free == d.ovpos_free;
    case 26: return s.mclick == d.mclick;   case 48: return s.ovsize == d.ovsize;   case 49: return s.ovalpha == d.ovalpha;   case 50: return s.ovauto == d.ovauto;   case 52: return s.touchbar == d.touchbar;   case 53: return s.touchmode == d.touchmode;   case 54: return s.touchdir == d.touchdir && s.touchdir2 == d.touchdir2;
    case 29: return s.touchfx == d.touchfx;   case 30: return s.tfxsz == d.tfxsz;   case 31: return s.tfxmouse == d.tfxmouse;
    case 32: return s.tmouse == d.tmouse;   case 33: return s.backpos == d.backpos;
    case 34: return s.vibe == d.vibe;   case 35: return s.vibe_pct == d.vibe_pct;
    default: return false; }
}
