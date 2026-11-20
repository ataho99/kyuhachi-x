/* app/wavdump.h -- t832 (#1010): 장치별 파형 덤프(디버그 전용, 평소 꺼짐).
 *
 * 무엇: 믹서 채널마다 WAV 하나씩 — 채널 볼륨·슬라이더가 곱해지기 전 값(덤프 중에는 모든 볼륨을 1.0 으로 고정), 믹서 출력 주파수 그대로, 스테레오 16비트.
 *   PC-98 보드(FM·SSG·리듬·ADPCM·PCM86)는 코어에서 「PC-98」 채널 하나로 합쳐져 나오므로, 부품별은 solo 로 따로 뜬다(아래).
 *   채널 이름 예: PC-98 · SPKR(비퍼) · CDAUDIO(CD-DA) · SB/PCM(SB 계열)  — 이름은 믹서 채널 이름 그대로 파일명에 쓴다.
 * 켜는 법(우선순위: 명령줄 > 환경변수 > 파일):
 *   --wavdump <폴더>            덤프 폴더(없으면 만든다). 이것이 있어야 켜짐
 *   --wavsolo fm,ssg,rhythm,adpcm,pcm86,beep,cdda,sb   나열한 것만 소리를 낸다(나머지는 볼륨 0). 비우면 전부. 장치 하나씩 여러 번 돌려 부품별 WAV 를 얻는다
 *   --wavfrom <초>              게임 시작(boot) 뒤 N 초부터 기록(기본 0)
 *   환경변수 KH_WAVDUMP=<폴더>  /  exe 폴더의 kh_wavdump.ini (줄: wavdump=<폴더> · wavsolo=… · wavfrom=…) — 안드로이드는 이 파일을 쓴다
 * 끝나면(--sec 도달·종료): <폴더>\<태그>_<채널>.wav 와 <폴더>\<태그>_summary.tsv(채널·주파수·프레임·초·peak·rms·dBFS), 로그 「WAVDUMP …」 줄.
 *   태그 = solo 목록을 +로 이은 것, 비면 all.
 * 믹서 쪽 훅 = core_patches/0009-wavdump.patch(MixerChannel::EndFrame 에서 kh_wavdump_feed 호출). 켜지 않으면 kh_wavdump_on=0 이라 비용 없음. */
#include <map>
#include <string>
extern "C" { void opngen_setvol(unsigned vol); void psggen_setvol(unsigned vol); void rhythm_setvol(unsigned vol); void rhythm_update(void * rhy);
             void adpcm_setvol(unsigned vol); void adpcm_update(void * ad); void pcm86gen_setvol(unsigned vol); extern char rhythm[]; extern char adpcm[]; }
int kh_wavdump_on = 0;   /* mixer.cpp(0009 패치)가 본다 */
namespace khwd {
struct Ch { FILE * f; unsigned long long n; unsigned freq; double sq; int peak; unsigned long long hdr_n; std::string path; };
static std::map<std::string, Ch> chs; static std::string dir, solo, tag; static unsigned from_ms = 0; static int cfg = -1; static bool done = false, ae = false; static SDL_mutex * mx = NULL;
static void le32(unsigned char * p, unsigned v) { p[0] = (unsigned char)(v & 255); p[1] = (unsigned char)((v >> 8) & 255); p[2] = (unsigned char)((v >> 16) & 255); p[3] = (unsigned char)((v >> 24) & 255); }
static void header(Ch & c) {
    unsigned char h[44]; memcpy(h, "RIFF", 4); le32(h + 4, (unsigned)(36 + c.n * 4)); memcpy(h + 8, "WAVEfmt ", 8); le32(h + 16, 16);
    h[20] = 1; h[21] = 0; h[22] = 2; h[23] = 0; le32(h + 24, c.freq); le32(h + 28, c.freq * 4); h[32] = 4; h[33] = 0; h[34] = 16; h[35] = 0; memcpy(h + 36, "data", 4); le32(h + 40, (unsigned)(c.n * 4));
    long cur = ftell(c.f); fseek(c.f, 0, SEEK_SET); fwrite(h, 1, 44, c.f); fseek(c.f, cur, SEEK_SET);
}
static bool has(const char * t) { if (solo.empty()) return true; std::string s = "," + solo + ","; return s.find(std::string(",") + t + ",") != std::string::npos; }
static void load_cfg() {
    if (cfg >= 0) return; cfg = 0;
    if (dir.empty()) { const char * e = getenv("KH_WAVDUMP"); if (e && *e) dir = e; }
    if (dir.empty()) {
        FILE * f = fopen((gApp.exedir + "\\kh_wavdump.ini").c_str(), "r");
        if (f) { char ln[400]; while (fgets(ln, sizeof ln, f)) { std::string s = ln; while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
                 if (s.compare(0, 8, "wavdump=") == 0) dir = s.substr(8); else if (s.compare(0, 8, "wavsolo=") == 0) solo = s.substr(8); else if (s.compare(0, 8, "wavfrom=") == 0) from_ms = (unsigned)(atof(s.c_str() + 8) * 1000.0); } fclose(f); }
    }
    if (dir.empty()) return;
    cfg = 1; tag = solo.empty() ? "all" : solo; for (size_t i = 0; i < tag.size(); i++) if (tag[i] == ',') tag[i] = '+';
    mx = SDL_CreateMutex();
}
static void mkdirs(const std::string & p) { for (size_t i = 1; i <= p.size(); i++) if (i == p.size() || p[i] == '\\' || p[i] == '/') plat::mkdir(p.substr(0, i).c_str()); }
}
/* mixer.cpp 가 부른다(에뮬 스레드). len 은 프레임 수, lr 은 L,R 교대 int16. */
void kh_wavdump_feed(const char * name, unsigned freq, unsigned len, const int16_t * lr) {
    if (!kh_wavdump_on || !name || !len || !khwd::mx) return;
    SDL_LockMutex(khwd::mx);
    std::string key = name; std::map<std::string, khwd::Ch>::iterator it = khwd::chs.find(key);
    if (it == khwd::chs.end()) {
        khwd::Ch c; c.n = 0; c.freq = freq; c.sq = 0; c.peak = 0; c.hdr_n = 0; std::string fn = key; for (size_t i = 0; i < fn.size(); i++) { char ch = fn[i]; if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '-')) fn[i] = '_'; }
        c.path = khwd::dir + "\\" + khwd::tag + "_" + fn + ".wav"; c.f = fopen(c.path.c_str(), "wb");
        if (!c.f) { SDL_UnlockMutex(khwd::mx); return; }
        unsigned char z[44]; memset(z, 0, sizeof z); fwrite(z, 1, 44, c.f); it = khwd::chs.insert(std::make_pair(key, c)).first;
        if (gApp.log) gh_log("WAVDUMP open %s freq=%u -> %s", name, freq, it->second.path.c_str());
    }
    khwd::Ch & c = it->second; fwrite(lr, 4, len, c.f);
    for (unsigned i = 0; i < len * 2; i++) { int v = lr[i]; c.sq += (double)v * v; int a = v < 0 ? -v : v; if (a > c.peak) c.peak = a; }
    c.n += len; if (c.n - c.hdr_n >= c.freq) { khwd::header(c); fflush(c.f); c.hdr_n = c.n; }
    SDL_UnlockMutex(khwd::mx);
}
static void kh_wavdump_close(void) {
    if (!khwd::mx || khwd::chs.empty()) { kh_wavdump_on = 0; khwd::done = true; return; }
    SDL_LockMutex(khwd::mx); kh_wavdump_on = 0; khwd::done = true;
    FILE * s = fopen((khwd::dir + "\\" + khwd::tag + "_summary.tsv").c_str(), "w"); if (s) fprintf(s, "channel\tfreq\tframes\tsec\tpeak\trms\tdbfs\n");
    for (std::map<std::string, khwd::Ch>::iterator it = khwd::chs.begin(); it != khwd::chs.end(); ++it) {
        khwd::Ch & c = it->second; if (!c.f) continue; khwd::header(c); fclose(c.f); c.f = NULL;
        double rms = c.n ? sqrt(c.sq / (double)(c.n * 2)) : 0.0, db = rms > 0.0 ? 20.0 * log10(rms / 32768.0) : -999.0, sec = c.freq ? (double)c.n / (double)c.freq : 0.0;
        if (gApp.log) gh_log("WAVDUMP %s freq=%u frames=%llu sec=%.2f peak=%d rms=%.1f dbfs=%.1f", it->first.c_str(), c.freq, c.n, sec, c.peak, rms, db);
        if (s) fprintf(s, "%s\t%u\t%llu\t%.2f\t%d\t%.1f\t%.1f\n", it->first.c_str(), c.freq, c.n, sec, c.peak, rms, db);
    }
    if (s) fclose(s); khwd::chs.clear(); SDL_UnlockMutex(khwd::mx);
}
/* GFX_Events 가 매번 부른다(since = 게임 시작 뒤 ms). 처음 한 번 켠다. */
static void kh_wavdump_tick(unsigned since) {
    khwd::load_cfg(); if (khwd::cfg != 1 || kh_wavdump_on || khwd::done || since < khwd::from_ms) return;
    khwd::mkdirs(khwd::dir); if (!khwd::ae) { khwd::ae = true; atexit(kh_wavdump_close); }
    kh_wavdump_on = 1; gh_log("WAVDUMP start t=%u dir=%s solo=%s", since, khwd::dir.c_str(), khwd::solo.empty() ? "(all)" : khwd::solo.c_str());
}
/* 덤프 설정이면 슬라이더 대신 볼륨을 고정(전부 1.0, solo 에 없는 부품은 0)하고 true. *cda = CD-DA 배율. gh_apply_volume 맨 앞에서 부른다. */
static bool kh_wavdump_volume(float * cda) {
    khwd::load_cfg(); if (khwd::cfg != 1) return false;
    MixerChannel * m = MIXER_FindChannel("PC-98"); if (m) m->SetVolume(1.0f, 1.0f);
    static const char * const sb[] = { "PCM", "SB", NULL }; const float vb = khwd::has("beep") ? 1.0f : 0.0f, vs = khwd::has("sb") ? 1.0f : 0.0f;
    { MixerChannel * d = MIXER_FindChannel("SPKR"); if (d) d->SetVolume(vb, vb); d = MIXER_FindChannel("PCSPEAKER"); if (d) d->SetVolume(vb, vb); }
    for (int i = 0; sb[i]; i++) { MixerChannel * d = MIXER_FindChannel(sb[i]); if (d) d->SetVolume(vs, vs); }
    *cda = khwd::has("cdda") ? 1.0f : 0.0f; { MixerChannel * d = MIXER_FindChannel("CDAUDIO"); if (d) d->SetVolume(*cda, *cda); }
    opngen_setvol(khwd::has("fm") ? 128u : 0u); psggen_setvol(khwd::has("ssg") ? 128u : 0u); rhythm_setvol(khwd::has("rhythm") ? 128u : 0u); rhythm_update(rhythm);
    adpcm_setvol(khwd::has("adpcm") ? 128u : 0u); adpcm_update(adpcm); pcm86gen_setvol(khwd::has("pcm86") ? 128u : 0u);
    static bool lg = false; if (!lg) { lg = true; gh_log("WAVDUMP volume fixed: solo=%s (sliders ignored)", khwd::solo.empty() ? "(all)" : khwd::solo.c_str()); }
    return true;
}
