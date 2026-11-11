#pragma once
/* app/gh_audio.h -- v0.6.8 #B4: 소리 장치 하나를 앱 전체가 공유한다.
   안드로이드 SDL2 는 오디오 장치를 한 번에 하나만 열 수 있는데, 삑(gh_beep)·CRT 소리·부팅 소음(pwr_seq)·메뉴 클릭(mclk) 이
   각자 SDL_OpenAudioDevice 를 불러서 먼저 연 쪽만 소리가 났다. 이제 장치는 여기서 한 번만 열고(44100Hz 16비트 모노),
   소리는 모두 gau::play() 로 넣는다. 콜백이 소프트웨어로 합친다(동시 재생 가능). 샘플레이트가 다르면 넣을 때 선형 보간으로 맞춘다.
   SDL 과 표준 라이브러리만 쓴다(로그 함수에 기대지 않음 — 실패 문구는 gau::err). */
namespace gau {
static const int RATE = 44100;
struct Voice { std::vector<int16_t> pcm; size_t pos; int id; };
static SDL_AudioDeviceID dev = 0;
static std::vector<Voice> vs;
static int next_id = 1;
static char err[160] = "";
static void cb(void *, Uint8 * st, int len) {   /* SDL 이 장치 잠금을 잡은 채로 부른다 */
    int16_t * o = (int16_t *)st; int n = len / 2; static int32_t acc[8192];
    while (n > 0) {
        const int m = n > 8192 ? 8192 : n; for (int i = 0; i < m; i++) acc[i] = 0;
        for (size_t k = 0; k < vs.size(); k++) { Voice & v = vs[k]; const size_t left = v.pcm.size() - v.pos; const int c = (size_t)m < left ? m : (int)left; for (int i = 0; i < c; i++) acc[i] += v.pcm[v.pos + (size_t)i]; v.pos += (size_t)c; }
        for (int i = 0; i < m; i++) { int32_t s = acc[i]; o[i] = (int16_t)(s > 32767 ? 32767 : s < -32768 ? -32768 : s); }
        o += m; n -= m;
    }
}
static bool open_dev(void) {
    if (dev) return true;
    SDL_AudioSpec w; SDL_zero(w); w.freq = RATE; w.format = AUDIO_S16SYS; w.channels = 1; w.samples = 1024; w.callback = cb;
    SDL_AudioSpec h; SDL_zero(h);
    dev = SDL_OpenAudioDevice(NULL, 0, &w, &h, 0);   /* 허용 변경 0 = 다른 형식이면 SDL 이 변환 */
    if (!dev) { snprintf(err, sizeof err, "%s", SDL_GetError()); return false; }
    SDL_PauseAudioDevice(dev, 0); return true;
}
/* 재생 끝난 소리를 치운다(장치 잠금을 잡은 채로 부를 것) */
static void sweep(void) { for (size_t k = 0; k < vs.size();) { if (vs[k].pos >= vs[k].pcm.size()) vs.erase(vs.begin() + (long)k); else k++; } }
/* 소리 하나 시작. 돌려주는 값 = 소리 번호(0 = 실패). 16비트 모노 pcm, rate = 그 샘플레이트 */
static int play(const int16_t * p, size_t n, int rate) {
    if (!p || n == 0 || rate < 4000) return 0;
    if (!open_dev()) return 0;
    Voice v; v.pos = 0; v.id = 0;
    if (rate == RATE) v.pcm.assign(p, p + n);
    else { const size_t m = (size_t)((uint64_t)n * RATE / (uint64_t)rate); v.pcm.resize(m); for (size_t i = 0; i < m; i++) { const double x = (double)i * rate / RATE; size_t a = (size_t)x; const double f = x - (double)a; const size_t b = a + 1 < n ? a + 1 : n - 1; if (a >= n) a = n - 1; v.pcm[i] = (int16_t)((double)p[a] * (1.0 - f) + (double)p[b] * f); } }
    SDL_LockAudioDevice(dev); sweep(); if (vs.size() >= 24) vs.erase(vs.begin()); v.id = next_id++; if (next_id <= 0) next_id = 1; const int id = v.id; vs.push_back(v); SDL_UnlockAudioDevice(dev);
    return id;
}
static void stop(int id) { if (!dev || id <= 0) return; SDL_LockAudioDevice(dev); for (size_t k = 0; k < vs.size(); k++) if (vs[k].id == id) { vs.erase(vs.begin() + (long)k); break; } SDL_UnlockAudioDevice(dev); }
static bool playing(int id) { if (!dev || id <= 0) return false; bool r = false; SDL_LockAudioDevice(dev); for (size_t k = 0; k < vs.size(); k++) if (vs[k].id == id && vs[k].pos < vs[k].pcm.size()) { r = true; break; } SDL_UnlockAudioDevice(dev); return r; }
}   /* namespace gau */
