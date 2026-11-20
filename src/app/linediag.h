/* app/linediag.h -- t842 (KH-J23, 슨상 「빌드 ㄱ」 11-17 19:28, 대장방 #1301): t840 + 덩어리 시작 직전 1초의 「직전 신호」 기록 — PCM86 제어 포트(0xA460~A46A) 쓰기 전부(포트·값·CS:IP·시각) + CD 읽기(IDE ATAPI → KhCdi::ReadSectorsHost 가로채기: LBA·섹터수·시각) → 덩어리마다 LINEDIAG pre 줄(csig·cs4·lsig·ll). (이하 t840 설명) -- t840 (KH-J21, 슨상 「빌드 ㄱ」「가」 11-17 18:01, 대장방 #1283): t839 줄 진단 + 덩어리 꼬리 지문(h256_tail) — 끝주소 키·꼬리 지문 키를 나란히 시험 (디버그 전용, 기본 꺼짐).
 * 무엇: t839 로직(PCM86 폴링 지문 + 0xA46C 쓰기 핸들러 감싸기로 읽는 포인터 기록)은 그대로. 추가: 덩어리가 닫힐 때 「끝에 이어진 같은 값 줄(무음 꼬리, 길이 tr)」을 뺀 마지막 256바이트의 지문(tail)과,
 *   그 바이트가 읽힌 주소(a1t = 꼬리 끝 바이트의 DS:SI)를 같이 찍는다. 무음 임계(512)로 덩어리가 닫히기 직전 최대 511바이트가 같은 값 줄로 덩어리 안에 남기 때문에, 그 줄을 빼고 「진짜 소리 끝」 256바이트를 지문으로 쓴다.
 *   코어 패치 없음. 원래 핸들러는 그대로 부르므로 소리·타이밍 무영향.
 * 한계: 진짜 메모리 읽기 가로채기는 아니고 쓰는 순간의 포인터 레지스터(t839 와 같음). 덩어리가 256바이트보다 짧으면 있는 만큼만(tn=지문에 쓴 바이트 수).
 * t839 에서 그대로: 무음 기준 512 · 덩어리는 무음/리셋/1000ms 공백에서만 닫음 · 머리 지문은 첫 소리 바이트부터.
 * 로그: 덩어리 한 줄 LINEDIAG pcm86 #순번 t= dur= bytes= n= h64= h256= rate= dactrl= end= skip= src= a0= a1= mono= cs:ip= miss= tail=<꼬리지문 hex> tn=<꼬리에 쓴 바이트> a1t=<꼬리 끝 주소 hex> tr=<뺀 무음 꼬리 길이>
 *       바로 다음 줄 LINEDIAG cand #순번 ... (t839 와 같음)
 * 켜는 법: --linediag / 환경변수 KH_LINEDIAG=1. 안 켜면 kh_linediag_on=0 이라 비용 없음(핸들러도 안 감쌈).
 * pcm86 구조체 오프셋(np2kai pcm86.h, UINT32 줄): wrtpos=40 realbuf=44 fifo=80 dactrl=82 buffer=88 (총 88+65536). */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
extern "C" { extern char pcm86[]; }
int kh_linediag_on = 0;
namespace khld {
enum { NC = 6 };
static const char * const cname[NC] = { "ds:si", "es:di", "ds:bx", "ds:di", "es:bx", "ss:bp" };
/* 링 칸: 0 ds 1 es 2 ss 3 si 4 di 5 bx 6 bp 7 cs 8 ip */
static const int csg[NC] = { 0, 1, 0, 0, 1, 2 };
static const int cof[NC] = { 3, 4, 5, 4, 5, 6 };
static uint16_t rg[9][65536];
static IO_WriteHandler * orig_w = NULL;
static unsigned wcount = 0, wseen = 0, ninst = 0, mism = 0;
struct Cand { uint32_t first, last, prev; unsigned mono; bool has; };
struct Chunk { bool open; unsigned t0, tlast, bytes; int n; uint32_t h64, h256; unsigned char rate, dact; Cand k[NC]; unsigned cs0, ip0, miss0; char pre[1024]; };
static Chunk c = {};
/* 꼬리 지문용: 덩어리에 들어간 마지막 1024바이트와 그 DS:SI 주소, 끝에 이어진 같은 값 줄 길이 */
static unsigned char tb[1024]; static uint32_t ta[1024]; static unsigned tbn = 0, trun = 0;
static bool cfg_done = false, have_w = false; static unsigned last_w = 0, nchunks = 0;
static int runv = -1; static unsigned runlen = 0;     /* 지금 이어지는 같은 값 줄: 값·길이 */
static int silv = -1;                                  /* 마지막으로 확인된 무음 값(없으면 -1) */
static unsigned long long skipped = 0;
/* t842: 직전 신호 링 — kind 1 = PCM86 제어 포트 쓰기(0xA460~A46A), kind 2 = CD 읽기(IDE ATAPI). 시각 = 마지막 tick 의 since(ms) */
struct Ev { unsigned t; unsigned short port; unsigned char val; unsigned short cs, ip; unsigned lba, num; unsigned char kind; };
static Ev ev[16384]; static unsigned evn = 0, cur_since = 0, ncd = 0;
static IO_WriteHandler * orig2[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static void evpush(unsigned char kind, unsigned short port, unsigned char val, unsigned lba, unsigned num) {
    Ev & e = ev[evn & 16383u]; e.t = cur_since; e.port = port; e.val = val; e.cs = (unsigned short)SegValue(cs); e.ip = (unsigned short)reg_eip; e.lba = lba; e.num = num; e.kind = kind; evn++;
}
static void wrap2(Bitu port, Bitu val, Bitu iolen) {
    evpush(1, (unsigned short)port, (unsigned char)val, 0, 0);
    IO_WriteHandler * o = orig2[((port - 0xA460u) >> 1) & 7u]; if (o) o(port, val, iolen);
}
static void cdread(unsigned long lba, unsigned long num, bool raw) { ncd++; evpush(2, raw ? 1 : 0, 0, (unsigned)lba, (unsigned)num); }
/* 덩어리 시작 t0 직전 1초(그리고 시작 순간까지)의 신호 요약: 개수·전체 서명(csig, lsig)·마지막 4개 제어 서명(cs4)·마지막 CD 읽기 LBA(ll)·최근 목록 */
static void mkpre(char * out, size_t sz, unsigned t0) {
    const unsigned lo = (t0 > 1000u) ? t0 - 1000u : 0; const unsigned n = evn > 16384u ? 16384u : evn; const unsigned first = evn - n;
    uint32_t cs_ = 0x811c9dc5u, ls_ = 0x811c9dc5u; unsigned cn = 0, ln = 0, ci[16], li[8];
    for (unsigned k = first; k < evn; k++) { const Ev & e = ev[k & 16383u]; if (e.t < lo || e.t > t0) continue;
        if (e.kind == 1) { cs_ = (cs_ ^ e.port) * 16777619u; cs_ = (cs_ ^ e.val) * 16777619u; ci[cn & 15u] = k; cn++; }
        else { ls_ = (ls_ ^ e.lba) * 16777619u; ls_ = (ls_ ^ e.num) * 16777619u; li[ln & 7u] = k; ln++; } }
    uint32_t c4 = 0x811c9dc5u; { const unsigned a4 = cn > 4u ? cn - 4u : 0; for (unsigned j = a4; j < cn; j++) { const Ev & e = ev[ci[j & 15u] & 16383u]; c4 = (c4 ^ e.port) * 16777619u; c4 = (c4 ^ e.val) * 16777619u; } }
    unsigned ll = 0; if (ln) ll = ev[li[(ln - 1) & 7u] & 16383u].lba;
    int o = snprintf(out, sz, "cn=%u ln=%u csig=%08x cs4=%08x lsig=%08x ll=%u ctl=[", cn, ln, cs_, c4, ls_, ll);
    { const unsigned a = cn > 16u ? cn - 16u : 0; for (unsigned j = a; j < cn && o < (int)sz - 60; j++) { const Ev & e = ev[ci[j & 15u] & 16383u]; o += snprintf(out + o, sz - o, "%s%04x=%02x@-%u", j > a ? "," : "", (unsigned)e.port, (unsigned)e.val, t0 - e.t); } }
    o += snprintf(out + o, sz - o, "] cd=[");
    { const unsigned a = ln > 8u ? ln - 8u : 0; for (unsigned j = a; j < ln && o < (int)sz - 60; j++) { const Ev & e = ev[li[j & 7u] & 16383u]; o += snprintf(out + o, sz - o, "%s%u+%u@-%u", j > a ? "," : "", e.lba, e.num, t0 - e.t); } }
    snprintf(out + o, sz - o, "]");
}
static inline uint32_t rd32(int off) { uint32_t v; memcpy(&v, pcm86 + off, 4); return v; }
/* 0xA46C 쓰기 핸들러를 감싸는 함수: CPU 가 바이트를 쓰는 바로 그 순간의 레지스터를 wrtpos 칸에 적고 원래 핸들러를 부른다 */
static void wrap(Bitu port, Bitu val, Bitu iolen) {
    const unsigned i = rd32(40) & 0xFFFFu;
    rg[0][i] = (uint16_t)SegValue(ds); rg[1][i] = (uint16_t)SegValue(es); rg[2][i] = (uint16_t)SegValue(ss);
    rg[3][i] = (uint16_t)reg_esi; rg[4][i] = (uint16_t)reg_edi; rg[5][i] = (uint16_t)reg_ebx; rg[6][i] = (uint16_t)reg_ebp;
    rg[7][i] = (uint16_t)SegValue(cs); rg[8][i] = (uint16_t)reg_eip;
    wcount++;
    if (orig_w) orig_w(port, val, iolen);
}
static void feed(unsigned char b) { if (c.n < 64) c.h64 = (c.h64 ^ b) * 16777619u; if (c.n < 256) { c.h256 = (c.h256 ^ b) * 16777619u; c.n++; } }
/* 덩어리에 들어간 바이트 하나의 후보 주소를 쌓는다(pos = 그 바이트의 pcm86 버퍼 칸) */
static void addr(unsigned pos) {
    for (int k = 0; k < NC; k++) {
        const uint32_t p = ((uint32_t)rg[csg[k]][pos] << 4) + rg[cof[k]][pos]; Cand & q = c.k[k];
        if (!q.has) { q.has = true; q.first = p; q.mono = 0; } else if (p == q.prev + 1u) q.mono++;
        q.prev = p; q.last = p;
    }
    if (c.bytes == 1) { c.cs0 = rg[7][pos]; c.ip0 = rg[8][pos]; }
}
/* 꼬리용: 바이트·DS:SI 주소를 링에 쌓고, 끝에 이어진 같은 값 줄 길이를 센다 */
static void tailfeed(unsigned char b, unsigned pos) {
    const unsigned i = tbn & 1023u;
    if (tbn > 0 && tb[(tbn - 1) & 1023u] == b) trun++; else trun = 1;
    tb[i] = b; ta[i] = ((uint32_t)rg[0][pos] << 4) + rg[3][pos]; tbn++;
}
static void end(const char * why) {
    if (!c.open) return; c.open = false; nchunks++;
    int best = 0; for (int k = 1; k < NC; k++) if (c.k[k].mono > c.k[best].mono) best = k;
    /* 꼬리 지문: 끝의 같은 값 줄(trun)을 뺀 마지막 256바이트 */
    uint32_t th = 0x811c9dc5u; unsigned tn = 0, a1t = 0; const unsigned e = (tbn > trun) ? tbn - trun : 0;
    if (e > 0) { const unsigned s = (e > 256u) ? e - 256u : 0; for (unsigned j = s; j < e; j++) { th = (th ^ tb[j & 1023u]) * 16777619u; tn++; } a1t = ta[(e - 1) & 1023u]; }
    gh_log("LINEDIAG pcm86 #%u t=%u dur=%u bytes=%u n=%d h64=%08x h256=%08x rate=%u dactrl=%02x end=%s skip=%llu src=%s a0=%05x a1=%05x mono=%u cs:ip=%04x:%04x miss=%u tail=%08x tn=%u a1t=%05x tr=%u",
           nchunks, c.t0, c.tlast - c.t0, c.bytes, c.n, c.h64, c.h256, (unsigned)c.rate, (unsigned)c.dact, why, skipped,
           cname[best], c.k[best].first, c.k[best].last, c.k[best].mono, c.cs0, c.ip0, mism - c.miss0, th, tn, a1t, trun);
    char buf[256]; int o = 0; buf[0] = 0;
    for (int k = 0; k < NC; k++) o += snprintf(buf + o, sizeof(buf) - o, " %s=%05x-%05x/%u", cname[k], c.k[k].first, c.k[k].last, c.k[k].mono);
    gh_log("LINEDIAG cand #%u%s", nchunks, buf);
    gh_log("LINEDIAG pre #%u t=%u bytes=%u %s", nchunks, c.t0, c.bytes, c.pre);
}
}
/* GFX_Events 가 매번 부른다(since = 게임 시작 뒤 ms). */
static void kh_linediag_tick(unsigned since) {
    if (!khld::cfg_done) { khld::cfg_done = true; const char * e = getenv("KH_LINEDIAG"); if (e && *e == '1') kh_linediag_on = 1;
        if (kh_linediag_on) gh_log("LINEDIAG on(t842): pcm86 polling (wrtpos@40 realbuf@44 fifo@80 dactrl@82 buffer@88), silence>=512 same bytes, gap=1000ms, + 0xA46C write-handler wrap (regs ds es ss si di bx bp cs:ip per byte) + tail fingerprint (last 256B before trailing same-value run)"); }
    khld::cur_since = since;
    if (!kh_linediag_on) return;
    for (unsigned p = 0xA460u; p <= 0xA46Au; p += 2) { IO_WriteHandler * cu = io_writehandlers[0][p]; if (cu && cu != khld::wrap2) { khld::orig2[(p - 0xA460u) >> 1] = cu; io_writehandlers[0][p] = khld::wrap2; } }
    { IO_WriteHandler * cur = io_writehandlers[0][0xA46C];   /* 감싸기(풀리면 다시) */
      if (cur && cur != khld::wrap) { khld::orig_w = cur; io_writehandlers[0][0xA46C] = khld::wrap; khld::ninst++;
          if (khld::ninst <= 6) gh_log("LINEDIAG hook #%u: io_writehandlers[0][0xA46C] 감쌈 (t=%u, 원래 핸들러 %p)", khld::ninst, since, (void *)cur); } }
    const unsigned w = khld::rd32(40) & 0xFFFFu;
    if (!khld::have_w) { khld::have_w = true; khld::last_w = w; khld::wseen = khld::wcount; return; }
    const unsigned d = (w - khld::last_w) & 0xFFFFu;
    khld::Chunk & c = khld::c;
    if (d > 0x4000u) { khld::end("reset"); khld::last_w = w; khld::runv = -1; khld::runlen = 0; khld::wseen = khld::wcount; return; }   /* 버퍼 리셋(wrtpos=0) 등 */
    if (d) {
        { const unsigned dw = khld::wcount - khld::wseen; if (dw != d) khld::mism += (dw > d ? dw - d : d - dw); khld::wseen = khld::wcount; }   /* 감싼 쓰기 횟수와 wrtpos 증가가 다르면 그만큼 주소 없음 */
        const unsigned char * bf = (const unsigned char *)pcm86 + 88;
        for (unsigned i = 0; i < d; i++) {
            const unsigned pos = (khld::last_w + i) & 0xFFFFu;
            const unsigned char b = bf[pos];
            if ((int)b == khld::runv) khld::runlen++; else { khld::runv = (int)b; khld::runlen = 1; }
            if (khld::runlen >= 512u) { khld::silv = (int)b; khld::skipped++; khld::end("silence"); continue; }   /* 무음: 덩어리 닫고 건너뜀 */
            if ((int)b == khld::silv && !c.open) { khld::skipped++; continue; }                                    /* 무음 뒤 아직 소리 시작 전 */
            if (!c.open) { c.open = true; c.t0 = since; c.tlast = since; c.bytes = 0; c.n = 0; c.h64 = 0x811c9dc5u; c.h256 = 0x811c9dc5u; khld::silv = -1;
                for (int k = 0; k < khld::NC; k++) { c.k[k].has = false; c.k[k].first = c.k[k].last = c.k[k].prev = 0; c.k[k].mono = 0; } c.cs0 = c.ip0 = 0; c.miss0 = khld::mism; khld::tbn = 0; khld::trun = 0; khld::mkpre(c.pre, sizeof c.pre, since); }
            khld::feed(b); c.bytes++; khld::addr(pos); khld::tailfeed(b, pos);
        }
        if (c.open) { c.tlast = since; c.rate = (unsigned char)(((const unsigned char *)pcm86)[80] & 7); c.dact = ((const unsigned char *)pcm86)[82]; }
        khld::last_w = w;
    } else if (c.open && since - c.tlast >= 1000u) khld::end("gap");
}
/* t842: CD 읽기 가로채기 — main_core.h 의 new CDROM_Interface_Image(3) 을 new KhCdi(3) 으로 바꿔 끼운다(IDE ATAPI 가 ReadSectorsHost 를 부른다). 읽는 동작은 그대로 위임 */
static void kh_cdread_hook(unsigned long lba, unsigned long num);   /* t843: linekey.h 가 정의 */
struct KhCdi : public CDROM_Interface_Image {
    KhCdi(uint8_t su) : CDROM_Interface_Image(su) {}
    bool ReadSectorsHost(void * b, bool raw, unsigned long s, unsigned long n) override { kh_cdread_hook(s, n); if (kh_linediag_on) khld::cdread(s, n, raw); return CDROM_Interface_Image::ReadSectorsHost(b, raw, s, n); }
};
