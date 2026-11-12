#pragma once
/* app/menu_sq.h -- in-game menu on a 16:10 display: the game picture behind stays 16:10 (dimmed, undistorted)
 * and the 640x400 menu canvas is squeezed horizontally into a centered 4:3 band (same look as the old 4:3 menu). */
namespace msq {
static const double BX = 53.333, SC = 1.2;   /* band starts at x=53.33, one band pixel covers 1.2 menu columns */
static inline int ux(int vx) { return (int)std::floor(((double)vx + 0.5 - BX) * SC); }   /* display x -> menu column */
static inline void dim_of(uint32_t * o, const uint32_t * s) {
    for (int i = 0; i < 640 * 400; i++) o[i] = 0xFF000000u | ((s[i] >> 1) & 0x7F7F7Fu);
}
static inline uint32_t mix(uint32_t a, uint32_t b, int wa) {   /* wa 0..256 */
    uint32_t r = 0xFF000000u;
    for (int sh = 0; sh <= 16; sh += 8) {
        int ca = (int)((a >> sh) & 255), cb = (int)((b >> sh) & 255);
        r |= (uint32_t)((ca * wa + cb * (256 - wa)) >> 8) << sh;
    }
    return r;
}
static inline void compose(ui::Ui & U, uint32_t * out, const uint32_t * dimb, bool snapscr) {
    static uint32_t A[640 * 400], B[640 * 400]; ov::Canvas ca = { A, 640, 400 }, cb = { B, 640, 400 };
    for (int i = 0; i < 640 * 400; i++) { A[i] = 0xFF010203u; B[i] = 0xFF040506u; }
    U.draw(ca); if (snapscr) U.draw(cb);
    memcpy(out, dimb, sizeof(uint32_t) * 640 * 400);
    for (int x = 54; x < 586; x++) {
        const double s0 = ((double)x - BX) * SC; const int i0 = (int)s0; const int i1 = i0 + 1 < 640 ? i0 + 1 : 639;
        const double w0 = (double)(i0 + 1) - s0; const int q0 = (int)(w0 / SC * 256.0 + 0.5);
        for (int y = 0; y < 400; y++) {
            const size_t k0 = (size_t)y * 640 + i0, k1 = (size_t)y * 640 + i1, ko = (size_t)y * 640 + x;
            const bool o0 = !snapscr || A[k0] == B[k0], o1 = !snapscr || A[k1] == B[k1];
            if (o0 && o1) out[ko] = mix(A[k0], A[k1], q0);
            else if (o0) out[ko] = A[k0];
            else if (o1) out[ko] = A[k1];
        }
    }
}
}
