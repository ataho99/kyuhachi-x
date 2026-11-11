#pragma once
/* app/pwr_seq.h -- patch C1: boot演出 into gh_boot_seq + game fade-only */
/* R1: kyuhachi boot seq (gh_boot_seq) plays click+spinup+noise via SDL */
/*     direct audio (mixer not up yet); video is black veil 1->0 only. */
/*     Beep stays gh_beep (boot_beep.wav never played). */
/* R2: game start is silent + black alpha 1->0 over 1500 ms linear. No hold, */
/*     no noise dots, no click/spinup/beep. Emu picture runs underneath. */
/* R3: noboot skips gh_boot_seq only; game fade runs regardless of noboot. */
/*     skipped only on pwrseq==0, uitest, frametest, resume/load, noload. */
/* R4: first input is NOT swallowed; it shortens a running fade to 200 ms. */
/* R5: hum (hddhum) 0=off, 1=game-only, 2=menu too. Game hum needs an HDD */
/*     slot in imageDiskList[2..5]. Menu with no game hums if hddhum==2. */
/*     Boot end hands over to hum with the old crossfade (no cut). Same */
/*     process menu<->game keeps hum level/pos (no restart); a new process */
/*     may cut (allowed). */
/* R6: kyuhachi logo is off in gh_boot_seq (KH_LOGO_ON false); the loading */
/*     cover never drew a logo. Info screens and title bar are untouched. */
/* pwrseq==0 turns off both R1 and R2. apply() signature kept; hddhum is */
/* an int 0/1/2 (settings clamp is C2's job). Mixer channel PWRNOISE kept. */
#include <vector>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>   /* C5: boot_logf needs FILE/va_list (gh_log unseen) */
namespace pwr {
static const unsigned BOOT_MS = 8000;   /* C8: 8s boot seq (was 4000) */
static const unsigned GAME_MS = 300;   /* C10: 0.3s fade (was R2 1500) */
static const unsigned NOISE_MS = 500;   /* C8: CRT raster at 500ms */
static const unsigned NOISE_END = 6700;   /* C8: hum joins at load rows */
static const unsigned SKIP_FADE = 200;   /* R4 shortened fade */
static const float SEQ_GAIN = 0.5f;   /* one-shot fixed gain */
static const float HUM_DB = 0.2512f;   /* hum extra -12 dB after norm */
static const int M_NONE = 0;
static const int M_BOOT = 1;   /* R1: inside gh_boot_seq */
static const int M_GAME = 2;   /* R2: game start fade */
/* C10: prompt + transitions (all ms). gaps sum 865, screen+snd share. */
static const unsigned C10_TOTAL = 9600;   /* C10: text end, black 250 */
static const unsigned C10_PROMPT = 7800;   /* C10: A> emergence */
static const unsigned C10_GAP[8] = { 120, 90, 140, 110, 80, 130, 100, 95 };
static const unsigned C10_ENTER = 8820;   /* C10: last char 8665+155 */
static const unsigned C10_SEEK2 = 9220;   /* C10: hdd_seek2 replay */
static const unsigned C10_BLACK = 9350;   /* C10: black till TOTAL */
static const unsigned C10_MENU_MS = 300;   /* C10: menu fade-in */
static const unsigned C10_POST_MS = 250;   /* C10: black after 1st frame */
static const unsigned C10_END_MS = 20000;   /* C10: game fade watchdog */
static unsigned c10_char_ms(unsigned i) {   /* C10: k-th char time */
    unsigned t = C10_PROMPT;
    for (unsigned k = 0; k < i && k < 8; k++) t += C10_GAP[k];
    return t;
}
static unsigned c10_nchars(unsigned t) {   /* C10: chars shown at t */
    unsigned n = 0;
    while (n < 8 && t >= c10_char_ms(n)) n++;
    return n;
}
static void c10_tick_add(std::vector<int16_t> & mx, size_t off,
        uint32_t seed, size_t n, bool low, float g) {   /* C10 */
    if (off >= mx.size() || n == 0) return;
    uint32_t s = seed; int32_t prev = 0;
    for (size_t j = 0; j < n && off + j < mx.size(); j++) {
        s = s * 1664525u + 1013904223u;   /* C10: fixed seed LCG */
        int32_t r = (int32_t)((s >> 9) & 0xFFFFu) - 32768;
        int32_t v = r;
        if (low) { v = (r + 3 * prev) / 4; prev = r; }   /* enter lower */
        float env = 1.0f - (float)j / (float)n;
        int32_t cur = (int32_t)mx[off + j] + (int32_t)(v * env * 0.35f * g);
        if (cur > 32767) cur = 32767; else if (cur < -32768) cur = -32768;
        mx[off + j] = (int16_t)cur;
    }
}
struct St {
    bool inited = false; void * ch = NULL; uint64_t t = 0;
    SDL_AudioDeviceID boot_dev = 0;   /* C4: direct-audio dev, not beep_dev */
    int pwrseq = 1, hddhum = 1, hvol = 3;
    bool seq_on = false, finishing = false;
    bool want_hum = false;
    int mode = 0;
    unsigned t0 = 0, t_skip = 0; float skip_a = 0.0f;
    uint64_t t0smp = 0, skip_smp = 0;
    unsigned frames = 0, calls = 0; unsigned long long nz = 0;
    std::vector<int16_t> click, spinup, noise, hum;
    bool h_click = false, h_spinup = false, h_noise = false, h_hum = false;
    uint64_t o_click = 0, o_spinup = 0, o_noise = 0;
    float hum_g = 0.0f; size_t hum_pos = 0;
};
static St S;
static inline float one_shot(const std::vector<int16_t> & b, bool have,
        uint64_t start, uint64_t k) {
    if (!have || k < start || k - start >= (uint64_t)b.size()) return 0.0f;
    return (float)b[(size_t)(k - start)] * (1.0f / 32768.0f);
}
static void load_all(const std::string & resdir) {
    if (!resdir.empty()) {
        S.h_click = dsn::load_wav(resdir + "\\boot_click.wav", S.click);
        S.h_spinup = dsn::load_wav(resdir + "\\boot_spinup.wav", S.spinup);
        S.h_noise = dsn::load_wav(resdir + "\\boot_noise.wav", S.noise);
        S.h_hum = dsn::load_wav(resdir + "\\hum_loop.wav", S.hum);
    }
    if (S.h_click) dsn::norm(S.click, 0.7f);
    if (S.h_spinup) dsn::norm(S.spinup, 0.7f);
    if (S.h_noise) dsn::norm(S.noise, 0.7f);
    if (S.h_hum) dsn::norm(S.hum, 0.25f);
}
static void boot_logf(const char * fmt, ...) {   /* C5: PWRBOOT file+stderr */
    char b[240]; va_list ap; va_start(ap, fmt);   /* gh_log lives below */
    vsnprintf(b, sizeof b, fmt, ap); va_end(ap);   /* (opts_log.h), unseen */
    if (gApp.log) { fprintf(gApp.log, "PWRBOOT %s\n", b); fflush(gApp.log); }
    /* p680: no stderr here (null FILE* aborts on Android) */
}
static void boot_cut(const char * why) {   /* C5: why logged at each site */
    boot_logf("cut why=%s dev=%u", why, (unsigned)S.boot_dev);
    if (S.boot_dev) { gau::stop((int)S.boot_dev); S.boot_dev = 0; }   /* #B4: boot_dev = 공유 장치(gau)의 소리 번호 */
}
static void boot_mix_play(void) {   /* C4: click+spinup+noise, SDL direct */
    if (gApp.headless || S.pwrseq == 0) {   /* C5: entry logged */
        boot_logf("play enter skip headless=%d pwrseq=%d", (int)gApp.headless, S.pwrseq);
        return;   /* pwrseq==0: no snd */
    }
    boot_logf("play enter se_gain=%.3f", gApp.ui_.effective(gApp.cur_game).se_gain());
    boot_cut("mix-reopen");
    const std::string rd = gApp.exedir + "\\res\\disk-noises";
    std::vector<int16_t> ck = S.click, sp = S.spinup, nz = S.noise;
    bool hk = S.h_click, hs = S.h_spinup, hn = S.h_noise;
    if (!hk) { hk = dsn::load_wav(rd + "\\boot_click.wav", ck);   /* pre-mixer */
        if (hk) dsn::norm(ck, 0.7f); }   /* same peak as mixer path */
    if (!hs) { hs = dsn::load_wav(rd + "\\boot_spinup.wav", sp);
        if (hs) dsn::norm(sp, 0.7f); }
    if (!hn) { hn = dsn::load_wav(rd + "\\boot_noise.wav", nz);
        if (hn) dsn::norm(nz, 0.7f); }
    std::vector<int16_t> sk1, sk2, hh1, hh2, fr;   /* C8: seeks+read */
    bool h1 = dsn::load_wav(rd + "\\fdd_seek1.wav", sk1);   /* C8: miss=>skip */
    bool h2 = dsn::load_wav(rd + "\\fdd_seek2.wav", sk2);   /* C8 */
    bool h3 = dsn::load_wav(rd + "\\hdd_seek1.wav", hh1);   /* C8 */
    bool h4 = dsn::load_wav(rd + "\\hdd_seek2.wav", hh2);   /* C8 */
    bool h5 = dsn::load_wav(rd + "\\fdd_read.wav", fr);   /* C8: 3 hits */
    if (h1) dsn::norm(sk1, 0.7f);   /* C8: same peak as mixer path */
    if (h2) dsn::norm(sk2, 0.7f);   /* C8 */
    if (h3) dsn::norm(hh1, 0.7f);   /* C8 */
    if (h4) dsn::norm(hh2, 0.7f);   /* C8 */
    if (h5) dsn::norm(fr, 0.7f);   /* C8 */
    boot_logf("play load ck=%u sp=%u nz=%u hk=%d hs=%d hn=%d", (unsigned)ck.size(),
        (unsigned)sp.size(), (unsigned)nz.size(), (int)hk, (int)hs, (int)hn);
    boot_logf("play load2 s1=%u s2=%u h1=%u h2=%u fr=%u",   /* C8 */
        (unsigned)sk1.size(), (unsigned)sk2.size(), (unsigned)hh1.size(),
        (unsigned)hh2.size(), (unsigned)fr.size());
    if (!hk && !hs && !hn && !h1 && !h2 && !h3 && !h4 && !h5) {   /* C8 */
        boot_logf("play skip nowav"); return; }
    const float g = gApp.ui_.effective(gApp.cur_game).se_gain() * SEQ_GAIN;
    boot_logf("play gain g=%.3f", g);
    if (g <= 0.0f) { boot_logf("play skip se0"); return; }   /* like beep amp<=0 */
    const size_t oc = (size_t)S.o_click, os = (size_t)S.o_spinup;
    const size_t on_ = (size_t)S.o_noise;   /* C4: current start offsets */
    const size_t o1 = (size_t)dsn::ms(4600), o2 = (size_t)dsn::ms(5000);   /* C8 */
    const size_t o3 = (size_t)dsn::ms(5700), o4 = (size_t)dsn::ms(6100);   /* C8 */
    const size_t o5a = (size_t)dsn::ms(6400), o5b = (size_t)dsn::ms(6900);   /* C8 */
    const size_t o5c = (size_t)dsn::ms(7400);   /* C8: read 3rd hit */
    const size_t o6 = (size_t)dsn::ms((int)C10_ENTER);   /* C10: seek1 */
    const size_t o7 = (size_t)dsn::ms((int)C10_SEEK2);   /* C10: seek2 */
    const size_t cap = (size_t)dsn::ms((int)C10_TOTAL);   /* C10: mix fits TOTAL */
    size_t n = 0;
    if (hk && oc + ck.size() > n) n = oc + ck.size();
    if (hs && os + sp.size() > n) n = os + sp.size();
    if (hn && on_ + nz.size() > n) n = on_ + nz.size();
    if (h1 && o1 + sk1.size() > n) n = o1 + sk1.size();   /* C8 */
    if (h2 && o2 + sk2.size() > n) n = o2 + sk2.size();   /* C8 */
    if (h3 && o3 + hh1.size() > n) n = o3 + hh1.size();   /* C8 */
    if (h4 && o4 + hh2.size() > n) n = o4 + hh2.size();   /* C8 */
    if (h5 && o5a + fr.size() > n) n = o5a + fr.size();   /* C8: read x3 */
    if (h5 && o5b + fr.size() > n) n = o5b + fr.size();   /* C8 */
    if (h5 && o5c + fr.size() > n) n = o5c + fr.size();   /* C8 */
    if (h3 && o6 + hh1.size() > n) n = o6 + hh1.size();   /* C10: enter seek1 */
    if (h4 && o7 + hh2.size() > n) n = o7 + hh2.size();   /* C10: enter seek2 */
    {   /* C10: ticks need buffer till ENTER+88 */
        const size_t need = (size_t)dsn::ms((int)C10_ENTER) + 88;
        if (n != 0 && n < need && need <= cap) n = need;
    }
    if (n > cap) n = cap;   /* C10: cut at TOTAL */
    boot_logf("play mix n=%u", (unsigned)n);
    if (n == 0) { boot_logf("play skip empty"); return; }
    std::vector<int16_t> mx(n, 0);
    for (size_t i = 0; i < n; i++) {
        float o = 0.0f;
        if (hk && i >= oc && i - oc < ck.size())
            o += (float)ck[i - oc] * (1.0f / 32768.0f);
        if (hs && i >= os && i - os < sp.size())
            o += (float)sp[i - os] * (1.0f / 32768.0f);
        if (hn && i >= on_ && i - on_ < nz.size())
            o += (float)nz[i - on_] * (1.0f / 32768.0f);
        if (h1 && i >= o1 && i - o1 < sk1.size())   /* C8 */
            o += (float)sk1[i - o1] * (1.0f / 32768.0f);
        if (h2 && i >= o2 && i - o2 < sk2.size())   /* C8 */
            o += (float)sk2[i - o2] * (1.0f / 32768.0f);
        if (h3 && i >= o3 && i - o3 < hh1.size())   /* C8 */
            o += (float)hh1[i - o3] * (1.0f / 32768.0f);
        if (h4 && i >= o4 && i - o4 < hh2.size())   /* C8 */
            o += (float)hh2[i - o4] * (1.0f / 32768.0f);
        if (h5 && i >= o5a && i - o5a < fr.size())   /* C8: read x3 */
            o += (float)fr[i - o5a] * (1.0f / 32768.0f);
        if (h5 && i >= o5b && i - o5b < fr.size())   /* C8 */
            o += (float)fr[i - o5b] * (1.0f / 32768.0f);
        if (h5 && i >= o5c && i - o5c < fr.size())   /* C8 */
            o += (float)fr[i - o5c] * (1.0f / 32768.0f);
        if (h3 && i >= o6 && i - o6 < hh1.size())   /* C10: seek1 */
            o += (float)hh1[i - o6] * (1.0f / 32768.0f);
        if (h4 && i >= o7 && i - o7 < hh2.size())   /* C10: seek2 */
            o += (float)hh2[i - o7] * (1.0f / 32768.0f);
        o *= g;
        if (o > 1.0f) o = 1.0f; else if (o < -1.0f) o = -1.0f;   /* sum clip */
        mx[i] = dsn::to16(o);
    }
    {   /* C10: key ticks share c10_char_ms with screen; enter lower */
        for (unsigned k = 0; k < 8; k++)
            c10_tick_add(mx, (size_t)dsn::ms((int)c10_char_ms(k)),
                0xC10u + k * 7919u, 55, false, g);
        c10_tick_add(mx, (size_t)dsn::ms((int)C10_ENTER), 0xE77u, 88, true, g);
    }
    S.boot_dev = (SDL_AudioDeviceID)gau::play(mx.data(), mx.size(), dsn::RATE);   /* #B4: 공유 장치에 넣는다(직접 열지 않음) */
    boot_logf("play open id=%u err=%s", (unsigned)S.boot_dev, gau::err);
    if (!S.boot_dev) { boot_logf("play skip openfail"); return; }   /* beep stays */
    boot_logf("play queued bytes=%u n=%u", (unsigned)(mx.size() * 2), (unsigned)n);
}
static void gen(int16_t * out, int n) {
    S.calls++;
    const float hg = S.hvol < 4 ? S.hvol * S.hvol / 16.0f : S.hvol / 4.0f;
    const float up = 1.0f / (0.30f * (float)dsn::RATE);
    const float dn = 1.0f / (0.50f * (float)dsn::RATE);
    const uint64_t fl = (uint64_t)(SKIP_FADE * dsn::RATE / 1000);
    for (int i = 0; i < n; i++) {
        const uint64_t k = S.t + (uint64_t)i;
        const uint64_t el = k >= S.t0smp ?
            (k - S.t0smp) * 1000ULL / (uint64_t)dsn::RATE : 0ULL;
        float o = 0.0f;
        if (S.seq_on && S.mode == M_BOOT) {   /* R1 one-shots; R2 is silent */
            float sg = SEQ_GAIN;
            if (S.finishing) {
                const uint64_t fs = k >= S.skip_smp ? k - S.skip_smp : 0ULL;
                sg *= fs >= fl ? 0.0f : 1.0f - (float)fs / (float)fl;
            }
            o += one_shot(S.click, S.h_click, S.t0smp + S.o_click, k) * sg;
            o += one_shot(S.spinup, S.h_spinup, S.t0smp + S.o_spinup, k) * sg;
            o += one_shot(S.noise, S.h_noise, S.t0smp + S.o_noise, k) * sg;
        }
        bool gate = true;   /* hum joins at NOISE_END during boot (old xfade) */
        if (S.seq_on && S.mode == M_BOOT) gate = (el >= NOISE_END);
        const bool ha = S.h_hum && S.want_hum && gate;
        if (ha) { S.hum_g += up; if (S.hum_g > 1.0f) S.hum_g = 1.0f; }
        else { S.hum_g -= dn; if (S.hum_g < 0.0f) S.hum_g = 0.0f; }
        if (S.hum_g > 0.0f && !S.hum.empty()) {
            o += (float)S.hum[S.hum_pos % S.hum.size()] *
                (1.0f / 32768.0f) * S.hum_g * hg * HUM_DB;
            S.hum_pos++;
        }
        float y = o; const float ay = y < 0.0f ? -y : y;
        if (ay > 0.8f) {
            const float c = 0.8f + 0.2f * tanhf((ay - 0.8f) * 5.0f);
            y = y < 0.0f ? -c : c;
        }
        const int16_t v = dsn::to16(y); out[i] = v; if (v) S.nz++;
    }
    S.t += (uint64_t)n;
}
static void handler(Bitu len) {
    if (!S.ch) return;
    int16_t buf[2048];
    while (len > 0) {
        const int n = len > 2048 ? 2048 : (int)len; gen(buf, n);
        ((MixerChannel *)S.ch)->AddSamples_m16((Bitu)n, buf); len -= (Bitu)n;
    }
}
static void apply(int pwrseq, int hddhum, int hvol, const std::string & rd) {
    if (pwrseq < 0) pwrseq = 0; if (pwrseq > 1) pwrseq = 1;
    if (hddhum < 0) hddhum = 0; if (hddhum > 2) hddhum = 2;   /* C1R5: 3-state */
    if (hvol < 0) hvol = 0; if (hvol > 8) hvol = 8;
    S.pwrseq = pwrseq; S.hddhum = hddhum; S.hvol = hvol;
    if (!S.inited) {
        S.inited = true; load_all(rd);
        S.ch = MIXER_AddChannel(handler, dsn::RATE, "PWRNOISE");
    }
    if (S.ch) ((MixerChannel *)S.ch)->Enable((S.pwrseq || S.hddhum) ? true : false);
}
static void to_hum(void) { S.seq_on = false; S.finishing = false; S.mode = M_NONE; }
static bool active(void) { return S.seq_on; }
static unsigned ff_t = 0;   /* C10: 1st emu frame tick, 0 = not yet */
static void note_first_frame(void) { if (!ff_t) ff_t = SDL_GetTicks(); }
static bool mf_on = false;   /* C10: menu fade active */
static unsigned mf_t0 = 0;
static void menu_fade_enter(void) {   /* C10: boot->menu 0.3s fade */
    if (S.pwrseq == 0 || S.seq_on) return;
    mf_on = true; mf_t0 = SDL_GetTicks();
}
static void menu_fade_apply(uint32_t * px, int w, int h) {   /* C10 */
    if (!mf_on || !px || w <= 0 || h <= 0) return;
    unsigned el = SDL_GetTicks() - mf_t0;
    if (el >= C10_MENU_MS) { mf_on = false; return; }
    float inv = (float)el / (float)C10_MENU_MS;
    const size_t n = (size_t)w * h;
    for (size_t i = 0; i < n; i++) {
        const uint32_t v = px[i];
        const unsigned r = (unsigned)(((v >> 16) & 255u) * inv);
        const unsigned g = (unsigned)(((v >> 8) & 255u) * inv);
        const unsigned b = (unsigned)((v & 255u) * inv);
        px[i] = (v & 0xFF000000u) | ((r & 255u) << 16) | ((g & 255u) << 8) | (b & 255u);
    }
}
static void boot_enter(bool full) {
    if (S.pwrseq == 0 || S.seq_on) return;   /* C1R1; pwrseq==0 kills R1+R2 */
    if (!gApp.uitest_dir.empty() || !gApp.ft_dir.empty()) return;
    (void)full;
    S.seq_on = true; S.finishing = false; S.mode = M_BOOT;
    boot_logf("enter full=%d", (int)full);   /* C5: which branch called */
    S.t0 = SDL_GetTicks(); S.t0smp = S.t;
    S.o_click = 0; S.o_spinup = (uint64_t)(100 * dsn::RATE / 1000);   /* C8 */
    S.o_noise = (uint64_t)(NOISE_MS * dsn::RATE / 1000);
    boot_mix_play();   /* C4: direct-audio one-shots (mixer not ready yet) */
}
static void boot_exit(void) {
    boot_cut("exit");   /* C5: stop direct audio */
    if (S.mode != M_BOOT) return;
    S.seq_on = false; S.finishing = false; S.mode = M_NONE;
}
static void on_game_start(void) {
    { const auto e_ = gApp.ui_.effective(gApp.cur_game);   /* p671: settings of this game at first start */
      apply(e_.pwrseq, e_.hddhum, e_.hvol, gApp.exedir + "\\res\\disk-noises"); }
    ff_t = 0;   /* C10: wait 1st emu frame (LOADING end sets it) */
    if (S.pwrseq == 0 || gApp.noload) return;   /* C1R3: noboot NOT checked */
    if (!gApp.uitest_dir.empty() || !gApp.ft_dir.empty()) return;
    if (gApp.resume_slot || gApp.load_ms) return;
    S.seq_on = true; S.finishing = false; S.mode = M_GAME;   /* silent fade */
    S.t0 = SDL_GetTicks(); S.t0smp = S.t;
    S.o_click = 0; S.o_spinup = 0; S.o_noise = 0;
}
static void tick(void) {
    { const bool in_game = (gApp.boot_tick != 0);   /* C1R5: 3-state hum */
      if (S.hddhum <= 0) S.want_hum = false;
      else if (!in_game) S.want_hum = (S.hddhum >= 2);
      else { bool eq = false;
        for (int d = 2; d < 6; d++) if (imageDiskList[d]) eq = true;
        S.want_hum = eq; } }
    if (!S.seq_on) return;
    if (gApp.menu_open) { to_hum(); return; }
    const unsigned now = SDL_GetTicks();
    if (S.finishing) {
        if (now - S.t_skip >= SKIP_FADE) {
            S.seq_on = false; S.finishing = false; S.mode = M_NONE; }
        return; }
    if (S.mode == M_GAME) {   /* C10: black till ff+POST, 0.3s fade */
        if (ff_t && now >= ff_t + C10_POST_MS + GAME_MS)
        { S.seq_on = false; S.finishing = false; S.mode = M_NONE; }
        else if (now - S.t0 >= C10_END_MS + C10_POST_MS + GAME_MS)
        { S.seq_on = false; S.finishing = false; S.mode = M_NONE; }
        return;
    }
    if (now - S.t0 >= BOOT_MS) { S.seq_on = false; S.finishing = false; S.mode = M_NONE; }
}
static bool overlay_on(void) { return S.seq_on; }
static float alpha_now(void) {
    if (!S.seq_on) return 0.0f;
    if (S.finishing) {
        const unsigned d = SDL_GetTicks() - S.t_skip;
        if (d >= SKIP_FADE) return 0.0f;
        return S.skip_a * (1.0f - (float)d / (float)SKIP_FADE);
    }
    const unsigned el = SDL_GetTicks() - S.t0;
    if (S.mode == M_GAME) {   /* C10: black till ff+POST, then fade */
        if (!ff_t) return 1.0f;
        const unsigned now = SDL_GetTicks(), base = ff_t + C10_POST_MS;
        if (now < base) return 1.0f;
        const unsigned d = now - base;
        if (d >= GAME_MS) return 0.0f;
        return 1.0f - (float)d / (float)GAME_MS;
    }
    if (el >= BOOT_MS) return 0.0f;   /* C4: black veil 1->0 linear */
    return 1.0f - (float)el / (float)BOOT_MS;
}
static void draw_overlay(uint32_t * px, int w, int h) {   /* C4: veil only */
    if (!S.seq_on || !px || w <= 0 || h <= 0) return;
    S.frames++;
    const float inv = 1.0f - alpha_now();
    const size_t n = (size_t)w * h;
    for (size_t i = 0; i < n; i++) {
        const uint32_t v = px[i];
        const unsigned r = (unsigned)(((v >> 16) & 255u) * inv);
        const unsigned g = (unsigned)(((v >> 8) & 255u) * inv);
        const unsigned b = (unsigned)((v & 255u) * inv);
        px[i] = (v & 0xFF000000u) | ((r & 255u) << 16) | ((g & 255u) << 8) | (b & 255u);
    }
}
static void boot_draw(uint32_t * px, int w, int h) {
    if (!S.seq_on || S.mode != M_BOOT) return;
    draw_overlay(px, w, h);   /* C1R1: black noise->fade on boot canvas */
}
static void draw_pub(uint32_t * px, int w, int h) {
    if (S.seq_on) draw_overlay(px, w, h);
}
static void shorten(SDL_Event & e) {
    if (S.mode == M_BOOT && S.boot_dev) boot_cut("shorten");   /* C5: cut now */
    if (!S.seq_on || S.mode != M_GAME) return;   /* C1R4: never swallowed */
    if (e.type == SDL_QUIT || e.type == SDL_WINDOWEVENT) return;
    if (!S.finishing) {
        S.skip_a = alpha_now();
        S.finishing = true; S.t_skip = SDL_GetTicks();
        S.skip_smp = S.t;
    }
}
/* E1R: menu click (move/confirm/hotkey tick, direct audio, no samples) */
namespace mclk {
static unsigned last_ms = 0;   /* E1R: 60ms repeat gate */
static unsigned mute_until = 0;   /* E1R: touch input stays silent */
static uint32_t lcg = 0xE1C4u;   /* E1R: fixed seed, 3 variants */
static void note_touch(void) { mute_until = SDL_GetTicks() + 350u; }   /* E1R */
static void play(bool enter) {   /* E1R */
    if (gApp.headless) return;   /* E1R: no audio in tests */
    if (gApp.boot_tick != 0 && !gApp.menu_open) return;   /* E1R */
    const ui::Settings st = gApp.ui_.effective(gApp.cur_game);   /* E1R */
    if (!st.mclick) return;   /* E1R: setting off */
    unsigned now = SDL_GetTicks();   /* E1R */
    if (now < mute_until) return;   /* E1R: touch silent */
    if (now - last_ms < 60u) return;   /* E1R: hold repeat gate */
    last_ms = now;   /* E1R */
    float g = st.se_gain() * 0.25f;   /* E1R: half of boot tick */
    if (g <= 0.0f) return;   /* E1R: se=0 silent */
    lcg = lcg * 1664525u + 1013904223u;   /* E1R: fixed LCG */
    uint32_t seed = 0xE1C4u + (uint32_t)((lcg >> 16) % 3u) * 7919u;   /* E1R */
    size_t n = enter ? 88u : 55u;   /* E1R: enter lower+longer */
    std::vector<int16_t> pcm(n, 0);   /* E1R */
    uint32_t s = seed; int32_t prev = 0;   /* E1R */
    for (size_t j = 0; j < n; j++) {   /* E1R */
        s = s * 1664525u + 1013904223u;   /* E1R */
        int32_t r = (int32_t)((s >> 9) & 0xFFFFu) - 32768;   /* E1R */
        int32_t v = r;   /* E1R */
        if (enter) { v = (r + 3 * prev) / 4; prev = r; }   /* E1R */
        float env = 1.0f - (float)j / (float)n;   /* E1R */
        float y = (float)v * (1.0f / 32768.0f) * env * 0.35f * g;   /* E1R */
        if (y > 1.0f) y = 1.0f; else if (y < -1.0f) y = -1.0f;   /* E1R */
        pcm[j] = dsn::to16(y);   /* E1R */
    }   /* E1R */
    gau::play(pcm.data(), pcm.size(), dsn::RATE);   /* #B4: 공유 장치에 넣는다 */
}   /* E1R: play */
static void key(ui::UKey k) {   /* E1R: menu input to tick kind */
    int kind = -1;   /* E1R: 0 light tick, 1 low enter tick */
    switch (k) {   /* E1R */
    case ui::UK_OK: kind = 1; break;   /* E1R */
    case ui::UK_UP: case ui::UK_DOWN: kind = 0; break;   /* E1R */
    case ui::UK_LEFT: case ui::UK_RIGHT: kind = 0; break;   /* E1R */
    case ui::UK_CANCEL: kind = 0; break;   /* E1R */
    case ui::UK_SQ: case ui::UK_TRI: kind = 0; break;   /* E1R */
    case ui::UK_L: case ui::UK_R: kind = 0; break;   /* E1R */
    case ui::UK_SELECT: kind = 0; break;   /* E1R */
    default: break;   /* E1R */
    }   /* E1R */
    if (kind < 0) return;   /* E1R: unmapped input silent */
    play(kind == 1);   /* E1R: nav/back/hotkey light, OK low */
}   /* E1R: key */
}   /* E1R: namespace mclk */
}   /* namespace pwr */
