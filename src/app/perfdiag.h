#pragma once
/* app/perfdiag.h -- #P2: 느린 기기 진단(로그 전용 — 화면 표시 없음) + 디버그 전용 「느린 코어 묶기」.
 *  ① 2초 stat 줄 옆에 「perf:」 한 줄: 에뮬 프레임 간격 · EndUpdate(훅+복사+오버레이) 소요 · GL 올림(스위즐+TexSubImage) 소요 · 구간 game/wall 비 · audio underrun/drop.
 *  ② user.ini [game] slowcpu=1 일 때만(설정 화면에 없음): 에뮬 스레드와 GL 스레드를 cpuinfo_max_freq 가 가장 낮은 코어 1~2개에 묶는다(자기 스레드만, 루트 불필요).
 *     안드로이드가 아니거나 코어 속도를 못 읽거나 모든 코어 속도가 같으면 조용히 무시(로그 한 줄만). 윈도는 아무 일도 안 한다.
 * SDL_GetPerformanceCounter 만 쓴다(SDL.h 가 먼저 포함돼 있어야 함). */
#include <atomic>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <time.h>
#if defined(__ANDROID__)
# include <sys/syscall.h>
/* t718 빌드에서 unistd.h 의 syscall/sysconf 가 안 보였다(빌드 shim) — 헤더에 기대지 않고 심볼만 직접 선언(이름을 달리해 선언 충돌 없음) */
extern "C" long pdg_syscall(long, ...) __asm__("syscall");
#endif
/* t723 (#P8): 코어(vga_draw.cpp 패치 사본)가 올리는 그리기 횟수 — 안드로이드 libmain 에서만 코어와 같이 링크됨(윈도 exe 는 코어 별도라 0) */
extern "C" { unsigned long long kh_vgacnt[4] = { 0, 0, 0, 0 }; }
namespace pdg {
static inline uint64_t now_us() { static const double f = (double)SDL_GetPerformanceFrequency(); return (uint64_t)((double)SDL_GetPerformanceCounter() * 1e6 / f); }
struct Acc { std::atomic<uint64_t> n{0}, sum{0}, mx{0}; void add(uint64_t v) { n.fetch_add(1, std::memory_order_relaxed); sum.fetch_add(v, std::memory_order_relaxed); if (v > mx.load(std::memory_order_relaxed)) mx.store(v, std::memory_order_relaxed); } };
static Acc g_gap, g_eu, g_gl;                       /* 에뮬 프레임 간격 · EndUpdate 소요 · GL 올림 소요 (쓰는 쪽은 각각 한 스레드) */
static std::atomic<uint64_t> g_last{0};
struct EmuTimer {                                   /* RENDER_EndUpdate 안에서 한 프레임 동안 산다 */
    uint64_t t0;
    EmuTimer() { t0 = now_us(); const uint64_t lt = g_last.load(std::memory_order_relaxed); if (lt) g_gap.add(t0 - lt); g_last.store(t0, std::memory_order_relaxed); }
    ~EmuTimer() { g_eu.add(now_us() - t0); }
};
static inline void gl_up(uint64_t us) { g_gl.add(us); }
struct Snap { unsigned en, gn; double gap_avg, gap_max, eu_avg, eu_max, gl_avg, gl_max; };
static inline void take1(Acc & a, unsigned & n, double & avg, double & mx) { const uint64_t c = a.n.exchange(0), s = a.sum.exchange(0), m = a.mx.exchange(0); n = (unsigned)c; avg = c ? (double)s / (double)c / 1000.0 : 0.0; mx = (double)m / 1000.0; }
static Snap take() { Snap r; unsigned dummy = 0; take1(g_gap, r.en, r.gap_avg, r.gap_max); take1(g_eu, dummy, r.eu_avg, r.eu_max); take1(g_gl, r.gn, r.gl_avg, r.gl_max); return r; }
/* ---- 느린 코어 묶기 ---- */
static std::atomic<int> g_slow{0};                  /* 1 = 켬(에뮬 스레드가 slow_init 에서 정함) */
static unsigned long g_mask[16];
static bool g_have = false, g_gl_bound = false;
static std::atomic<int> g_glres{0};                 /* GL 스레드 묶기 결과 0 아직/안 함 1 성공 2 실패 */
static std::string g_note;                          /* 첫 perf 줄에 한 번 붙는 설명(로그용) */
static inline bool bind_self() {
#if defined(__ANDROID__)
    if (!g_have) return false;
    return pdg_syscall((long)__NR_sched_setaffinity, 0L, (long)sizeof g_mask, (long)g_mask) == 0;   /* pid 0 = 부른 스레드만 */
#else
    return false;
#endif
}
static inline void slow_init(bool on) {            /* 에뮬 스레드(RunMachine 을 부르는 스레드)에서 한 번 */
    if (!on) return;
#if defined(__ANDROID__)
    long fr[64]; long mn = -1, mxf = -1; int n = 0;   /* 코어 수 = cpu0.. 에서 cpuinfo_max_freq 가 읽히는 만큼(앞에서부터, 끊기면 거기까지) */
    for (int i = 0; i < 64; i++) { char p[96]; snprintf(p, sizeof p, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i); FILE * f = fopen(p, "r"); if (!f) break; fclose(f); n++; }
    if (n < 2) { g_note = "slowcpu: 코어 속도 파일을 못 읽음 — 무시"; return; }
    for (int i = 0; i < n; i++) { char p[96]; snprintf(p, sizeof p, "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", i); long v = -1; FILE * f = fopen(p, "r"); if (f) { if (fscanf(f, "%ld", &v) != 1) v = -1; fclose(f); } fr[i] = v; if (v > 0) { if (mn < 0 || v < mn) mn = v; if (v > mxf) mxf = v; } }
    if (mn < 0) { g_note = "slowcpu: 코어 속도를 못 읽음 — 무시"; return; }
    if (mn == mxf) { g_note = "slowcpu: 모든 코어 속도가 같음 — 무시"; return; }
    memset(g_mask, 0, sizeof g_mask); int cnt = 0; std::string lst;
    for (int i = 0; i < n && cnt < 2; i++) if (fr[i] == mn) { g_mask[i / (8 * (int)sizeof(unsigned long))] |= 1ul << (i % (8 * (int)sizeof(unsigned long))); cnt++; lst += std::to_string(i) + " "; }
    g_have = true; const bool ok = bind_self(); g_slow.store(ok ? 1 : 0);
    g_note = std::string("slowcpu: 코어 ") + lst + "(최저 " + std::to_string(mn / 1000) + " MHz, 최고 " + std::to_string(mxf / 1000) + " MHz)에 에뮬 스레드 묶기 " + (ok ? "성공" : "실패");
#else
    g_note = "slowcpu: 안드로이드 전용 — 무시";
#endif
}
static inline void gl_tick() {                      /* GL 스레드 루프 맨 위 — 켜졌으면 자기 자신을 한 번 묶는다 */
    if (g_gl_bound || g_slow.load(std::memory_order_relaxed) == 0) return;
    g_gl_bound = true; g_glres.store(bind_self() ? 1 : 2);
}
/* ---- #P4: GFX_Events 구간 재기 + 훅 가르기(evdiv) ---- */
static std::atomic<int> g_evdiv{1};                 /* user.ini [game] evdiv=N (디버그 전용, 설정 화면에 없음) — 세 훅을 N 틱에 한 번만 부른다. 1 = 지금 동작 그대로 */
static inline void evdiv_init(int n) { g_evdiv.store(n < 1 ? 1 : n > 1000 ? 1000 : n); }
static inline bool run_hooks(unsigned & c) { const int d = g_evdiv.load(std::memory_order_relaxed); if (d <= 1) return true; return (++c % (unsigned)d) == 0; }
struct HkAcc { std::atomic<uint64_t> n{0}, us{0}; };
static HkAcc g_hk[4];                               /* 0 = GFX_Events 전체 · 1 = kh_input_tick · 2 = kh_stk_tick · 3 = kha_ld_tick (모두 에뮬 스레드) */
struct HkT { int i; uint64_t t0; explicit HkT(int i_) : i(i_), t0(now_us()) {} ~HkT() { g_hk[i].n.fetch_add(1, std::memory_order_relaxed); g_hk[i].us.fetch_add(now_us() - t0, std::memory_order_relaxed); } };
#if defined(__ANDROID__) || defined(__linux__)
static inline uint64_t thr_cpu_us() { struct timespec ts; if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) != 0) return 0; return (uint64_t)ts.tv_sec * 1000000ull + (uint64_t)ts.tv_nsec / 1000ull; }
#else
static inline uint64_t thr_cpu_us() { return 0; }   /* 윈도: 스레드 CPU 시간 안 잼(비율 칸은 0) */
#endif
/* 2초 창(win_ms)마다 에뮬 스레드에서 한 번: 「perf2:」 한 줄 문자열 */
static std::string take_vga(unsigned win_ms) {
    static unsigned long long p[4] = { 0, 0, 0, 0 }; const double w = win_ms ? (double)win_ms / 1000.0 : 1.0; double r[4];
    for (int i = 0; i < 4; i++) { const unsigned long long c = kh_vgacnt[i]; r[i] = (double)(c - p[i]) / w; p[i] = c; }
    char b[220]; snprintf(b, sizeof b, " | vga Xlat32 %.0f줄/s DrawSingleLine %.0f회/s 프레임끝EndUpdate %.1f회/s VerticalTimer %.1f회/s", r[0], r[1], r[2], r[3]);
    return std::string(b);
}
static std::string take_hk(unsigned win_ms) {
    static uint64_t c0 = 0; const uint64_t c1 = thr_cpu_us(); const double w = win_ms ? (double)win_ms / 1000.0 : 1.0, cpu = c0 ? (double)(c1 - c0) / 1000.0 / w : 0.0; c0 = c1;
    double n[4], ms[4]; for (int i = 0; i < 4; i++) { n[i] = (double)g_hk[i].n.exchange(0) / w; ms[i] = (double)g_hk[i].us.exchange(0) / 1000.0 / w; }
    char b[420]; snprintf(b, sizeof b, "perf2: GFX_Events %.0f회/s %.1f ms/s (에뮬 스레드 CPU %.1f ms/s 중 %.1f%%, 벽시계 %.1f%%) | input_tick %.0f회/s %.2f ms/s | stk_tick %.0f회/s %.2f ms/s | ld_tick %.0f회/s %.2f ms/s | evdiv=%d",
        n[0], ms[0], cpu, cpu > 0 ? ms[0] * 100.0 / cpu : 0.0, ms[0] / 10.0, n[1], ms[1], n[2], ms[2], n[3], ms[3], g_evdiv.load());
    return std::string(b) + take_vga(win_ms);
}
}
