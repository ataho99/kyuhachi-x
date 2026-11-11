#pragma once
/* screens/phos.h -- t662 (#635): phosphor option page (beam width / persistence / breathing / glow), a sub-page of S_SCREEN. */
/* One packed Settings::phos (2 bits per option: beam | pers << 2 | breath << 4 | glow << 6), own-value bit 22 for the whole bundle. */
inline void Ui::d_phos(Canvas & c) {
    titlebar(c, phr_("ph_title"));
    Settings & s = eff();
    const bool own = scope == 1 && cur >= 0 && cur < MAXG;
    const bool ind = own && (ovm[cur] & (1u << 22));
    const bool offp = s.ratio == 2;   /* integer scaling uses the plain shader: the bundle has no effect there */
    static const char * const nk[4] = { "ph_beam", "ph_pers", "ph_breath", "ph_glow" };
    static const char * const dk[4] = { "desc_ph_beam", "desc_ph_pers", "desc_ph_breath", "desc_ph_glow" };
    rbox(c, 16, 24, 288, 220, WH, phr_("ph_title"));
    std::vector<std::string> n; std::vector<bool> dm;
    for (int i = 0; i < 4; i++) {
        n.push_back((ind ? std::string("● ") : std::string()) + phr_(nk[i])); dm.push_back(offp);
    }
    items(c, 40, 44, 256, n, sel, 16, &dm);
    rbox(c, 320, 24, 304, 220, WH, phr_("ph_choices"));
    const char * lv[4] = { phr_("ph_lv0"), phr_("ph_lv1"), phr_("ph_lv2"), phr_("ph_lv3") };
    const char * pv[6] = { phr_("ph_lv0"), "8ms", "12ms", "18ms", "27ms", "40ms" };   /* #S1: 잔광 끔 + 5단. 공백 없는 표기 = 한 줄 264px(오른쪽 끝 x=600 < 상자 안쪽 616). 「8 ms」 공백형은 304px 로 넘침 */
    for (int i = 0; i < 4; i++) {
        choices(c, 344, 44 + i * 16, i == 1 ? pv : lv, i == 1 ? 6 : 4, i == 1 ? s.ph_pers() : (s.phos >> (2 * i)) & 3, offp ? GY : WH, sel == i);
    }
    rbox(c, 16, 248, 608, 82, WH, phr_("ph_prev"));
    text(c, 40, 270, std::string(phr_(offp ? "ph_off_note" : "ph_live")), offp ? YL : WH, K);
    desc_mq(c, 324, 44, dkey(phr_(dk[sel >= 0 && sel < 4 ? sel : 0])));
    helpline(c, phr_("ph_help"));
}
inline Act Ui::k_phos(UKey k, Act & a) {
    const int gi = cur; const bool own = scope == 1 && gi >= 0 && gi < MAXG; const unsigned PB = 1u << 22;
    Settings pre; if (own) { sync_t(gi); pre = t[gi]; }
    Settings & s = eff();
    if (k == UK_LEFT || k == UK_RIGHT) {
        const int ix = sel < 0 ? 0 : sel > 3 ? 3 : sel;
        const int sh = ix == 1 ? 8 : 2 * ix, mk = ix == 1 ? 7 : 3, mx = ix == 1 ? 5 : 3;   /* #S1: 잔광(1번)만 8~10비트 0..5, 나머지 2비트 0..3 */
        const int v = clampi(((s.phos >> sh) & mk) + (k == UK_RIGHT ? 1 : -1), 0, mx);
        s.phos = (s.phos & ~(mk << sh)) | (v << sh); a.t = A_APPLY; dirty = true;
    } else if (k == UK_TRI) {
        if (own && (ovm[gi] & PB)) {   /* marked line: back to global */
            ov_set(gi, ovm[gi] & ~PB); sync_t(gi); a.t = A_APPLY; dirty = true; return a;
        }
        s.phos = 0; a.t = A_APPLY; dirty = true;
    } else if (k == UK_OK || k == UK_CANCEL) { phos_pg = false; sel = rowidx(S_SCREEN, 13); return a; }
    if (own) { const unsigned d = diff_bits(pre, t[gi]); if (d) ov_set(gi, ovm[gi] | d); sync_t(gi); }
    return a;
}
