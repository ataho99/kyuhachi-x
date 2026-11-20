/* app/linekey.h -- t846 (KH-P14 표 경로 폴백 imgdir) = t845 + 아래. t845 설명: (KH-P13 reset 분기 수정·KH-J26 줄 메뉴 기본 숨김 lineui) = t844 + 아래. t844 설명: (KH-J25, 슨상 「빌드 ㄱ」 11-17 20:18, 대장방 #1311): t843 + 줄 표 자동 켜짐(linekey.tsv 가 있으면 켜짐, game.ini linekey=0 이면 끔) + 범위 행 R + 화면 줄 슬라이더용 이름·값 함수 — 시험판.
 * 키: 주 = 덩어리가 열리는 순간(첫 소리 바이트)의 「마지막 CD 읽기 LBA」(t842 실측: 대사마다 다르고 A·B·C 재현 100퍼센트, 지연 0 → 처음 바이트부터 곱함).
 *   보조 = t841 꼬리 지문(256B, 끝 같은 값 줄 뺌)+길이 — LBA 행이 없을 때(CD 를 안 읽는 재생) 덩어리가 닫힐 때 아직 안 읽힌 구간에 곱함. 같은 LBA 덩어리는 한 줄(이름)로 묶임.
 * CD 읽기는 KhCdi(CDROM_Interface_Image 상속, ReadSectorsHost)가 kh_cdread_hook 으로 알려 준다(제니스 = IDE ATAPI). 이 파일을 안 켜면(기본) 훅은 값만 기록하고 아무 영향 없음.
 * 켜는 법(기본 꺼짐): game.ini linekey=1(또는 log = 덩어리마다 로그) · --linekey · KH_LINEKEY=1. 표: games\<id>\linekey.tsv(있으면 자동 켜짐; linekey=0 이면 끔) — 줄마다 「L <LBA> <이름>」(LBA 키) 또는 「R <시작LBA> <끝LBA> <이름>」(범위, 정확한 L 이 우선) 또는 「<꼬리지문8hex> <길이> <이름>」(꼬리 키), #은 주석. 음량: user.ini linegain=이름:퍼센트,… (KH_LINEGAIN 이 덮음).
 * 로그: KH_LINEKEY_LOG=1(또는 linekey=log)이면 덩어리마다, 아니면 키에 걸린 덩어리만 「LINEKEY #n t= dur= bytes= len= lba0= lba1= grp= cdn= mode=lba|tail|none key= gain= scaled=<곱한>/<덩어리 바이트> tail= tn= sil= dact= end=」.
 *   grp = 덩어리가 열려 있던 동안 새로 시작된 CD 읽기 묶음 수, cdn = 그동안의 CD 읽기 횟수(섞임 확인용: grp>0 이면 다른 소리의 읽기가 겹친 것).
 * 한계: 8비트(dactrl&0x40)만 곱함(제니스 f2 = 8비트 스테레오). 곱하기 기준선은 0(무음값 sil 이 0 이 아니면 로그 sil= 로 보임). 코어 패치 없음 — 전역 pcm86 를 읽고 버퍼 바이트만 고친다. */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <vector>
extern "C" { extern char pcm86[]; }
int kh_linekey_on = 0;
namespace khlk {
struct Row { bool isl; unsigned lba, lba2; uint32_t tail; unsigned len; std::string name; };   /* isl: L(lba=lba2) 또는 R(lba..lba2) */
static std::vector<Row> rows;
static std::map<std::string, int> gain;
static bool logall = false, inited = false, have_w = false;
static unsigned last_w = 0, nchunks = 0, nrows = 0;
static unsigned cur_lba = 0, cd_n = 0, cd_grp = 0, cd_end = 0xFFFFFFFFu;     /* CD 읽기 기록: 마지막 읽기 LBA·횟수·묶음 수(연속이 아니면 새 묶음) */
static unsigned char tb[1024]; static unsigned tbn = 0, trun = 0;
static int runv = -1; static unsigned runlen = 0; static int silv = -1;
static bool open_ = false; static unsigned t0 = 0, tlast = 0, nbytes = 0, spos = 0;
static unsigned lba0 = 0, grp0 = 0, cdn0 = 0, live = 0; static int cgain = 100; static bool bylba = false; static std::string cname;
static unsigned long long skipped = 0; static unsigned nreset = 0, hd0 = 0;   /* t845: reset 횟수 · 덩어리 첫 바이트가 읽기 위치보다 얼마나 앞인지 */
static inline uint32_t rd32(int off) { uint32_t v; memcpy(&v, pcm86 + off, 4); return v; }
static void parse_gain(const std::string & s) {
    size_t i = 0;
    while (i < s.size()) { size_t j = s.find(',', i); if (j == std::string::npos) j = s.size(); std::string it = s.substr(i, j - i); i = j + 1;
        size_t c = it.find(':'); if (c == std::string::npos || c == 0) continue;
        gain[it.substr(0, c)] = atoi(it.c_str() + c + 1); }
}
static void load_table(const std::string & path, bool quiet) {
    FILE * f = fopen(path.c_str(), "rb"); if (!f) { if (!quiet) gh_log("LINEKEY table 없음: %s", path.c_str()); return; }
    char ln[512];
    while (fgets(ln, sizeof ln, f)) {
        if (ln[0] == '#' || ln[0] == '\r' || ln[0] == '\n') continue;
        unsigned t = 0, l = 0, l2 = 0; char nm[128] = {0};
        if (ln[0] == 'L') { if (sscanf(ln, "L %u %127s", &l, nm) == 2) { Row r; r.isl = true; r.lba = l; r.lba2 = l; r.tail = 0; r.len = 0; r.name = nm; rows.push_back(r); } }
        else if (ln[0] == 'R') { if (sscanf(ln, "R %u %u %127s", &l, &l2, nm) == 3 && l2 >= l) { Row r; r.isl = true; r.lba = l; r.lba2 = l2; r.tail = 0; r.len = 0; r.name = nm; rows.push_back(r); } }
        else if (sscanf(ln, "%x %u %127s", &t, &l, nm) == 3) { Row r; r.isl = false; r.lba = 0; r.lba2 = 0; r.tail = t; r.len = l; r.name = nm; rows.push_back(r); }
    }
    fclose(f); nrows = (unsigned)rows.size();
}
/* LBA → 줄: 정확한 L 행이 먼저, 없으면 R 범위(여럿이면 먼저 적힌 것) */
static int find_lba(unsigned lba) {
    int rg = -1;
    for (unsigned i = 0; i < rows.size(); i++) { const Row & r = rows[i]; if (!r.isl) continue; if (r.lba == r.lba2 && r.lba == lba) return (int)i; if (r.lba != r.lba2 && lba >= r.lba && lba <= r.lba2 && rg < 0) rg = (int)i; }
    return rg;
}
/* 표가 있으면 자동 켜짐(game.ini linekey=0 이면 끔). 표가 없으면 기존처럼 linekey=1|log·KH_LINEKEY 로만 켜짐 */
static void init(const std::string & flag, const std::string & gains, const std::string & tab) {
    const char * e = getenv("KH_LINEKEY"); const bool force = (e && *e == '1') || flag == "1" || flag == "log";
    if (flag == "0") return;
    load_table(tab, !force); if (rows.empty() && !force) return;
    kh_linekey_on = 1;
    const char * lg = getenv("KH_LINEKEY_LOG"); logall = (lg && *lg == '1') || flag == "log";
    parse_gain(gains); const char * eg = getenv("KH_LINEGAIN"); if (eg) parse_gain(eg);
    unsigned nl = 0; for (auto & r : rows) nl += r.isl ? 1u : 0u;
    std::string gs; for (auto & kv : gain) { char b[160]; snprintf(b, sizeof b, " %s=%d", kv.first.c_str(), kv.second); gs += b; }
    gh_log("LINEKEY on(t846%s): CD-read LBA key (main, from the first sound byte) + tail-fingerprint key (aux); table=%s rows=%u (LBA/range %u) gain:%s log=%d", force ? "" : " auto: table present", tab.c_str(), nrows, nl, gs.empty() ? " (none)" : gs.c_str(), logall ? 1 : 0);
}
/* 화면 줄 슬라이더: 표의 줄 이름(처음 나온 순서, 중복 없음)과 지금 값(퍼센트) */
static void names(std::vector<std::string> & n, std::vector<int> & p) {
    n.clear(); p.clear();
    for (auto & r : rows) { bool dup = false; for (auto & x : n) if (x == r.name) { dup = true; break; } if (dup) continue; n.push_back(r.name); auto it = gain.find(r.name); p.push_back(it != gain.end() ? it->second : 100); }
}
static std::string gains_str() {
    std::string s; for (auto & kv : gain) { if (kv.second == 100) continue; char b[160]; snprintf(b, sizeof b, "%s%s:%d", s.empty() ? "" : ",", kv.first.c_str(), kv.second); s += b; }
    return s;
}
static inline void scale_byte(unsigned pos, int g) {
    unsigned char * bf = (unsigned char *)pcm86 + 88; int v = (int)(int8_t)bf[pos & 0xFFFFu]; v = v * g / 100; if (v > 127) v = 127; if (v < -128) v = -128; bf[pos & 0xFFFFu] = (unsigned char)(int8_t)v;
}
/* 덩어리가 닫힐 때: LBA 키였으면 로그만, 아니면 꼬리 지문 키로 찾아 아직 안 읽힌 구간에 곱한다 */
static void end(const char * why) {
    if (!open_) return; open_ = false; nchunks++;
    const unsigned e = (tbn > trun) ? tbn - trun : 0;
    uint32_t th = 0x811c9dc5u; unsigned tn = 0;
    if (e > 0) { const unsigned s = (e > 256u) ? e - 256u : 0; for (unsigned j = s; j < e; j++) { th = (th ^ tb[j & 1023u]) * 16777619u; tn++; } }
    const unsigned char dact = ((const unsigned char *)pcm86)[82];
    const char * mode = "none"; std::string key = "?"; int g = 100; unsigned scaled = live, unplayed = 0; const char * note = "";
    if (bylba) { mode = "lba"; key = cname; g = cgain; if (!(dact & 0x40) && g != 100) note = " fmt16"; }
    else if (tn >= 256u) {
        int bi = -1; unsigned bd = 0xFFFFFFFFu;
        for (unsigned i = 0; i < rows.size(); i++) if (!rows[i].isl && rows[i].tail == th) { unsigned d = rows[i].len > nbytes ? rows[i].len - nbytes : nbytes - rows[i].len; if (d < bd) { bd = d; bi = (int)i; } }
        if (bi >= 0) { mode = "tail"; key = rows[bi].name; auto it = gain.find(rows[bi].name); if (it != gain.end()) g = it->second;
            if (g != 100 && e > 0) {
                if (!(dact & 0x40)) note = " fmt16";
                else {
                    const unsigned rp = rd32(36) & 0xFFFFu, wp = rd32(40) & 0xFFFFu; unplayed = (wp - rp) & 0xFFFFu;
                    const unsigned cnt = e > 0xC000u ? 0xC000u : e; const unsigned from = (spos + e - cnt) & 0xFFFFu;
                    const int base = (silv >= 0) ? (int)(int8_t)(unsigned char)silv : 0;
                    for (unsigned i = 0; i < cnt; i++) { const unsigned p = (from + i) & 0xFFFFu; if (((p - rp) & 0xFFFFu) >= unplayed) continue;
                        int v = (int)(int8_t)((unsigned char *)pcm86 + 88)[p]; v = base + (v - base) * g / 100; if (v > 127) v = 127; if (v < -128) v = -128; ((unsigned char *)pcm86 + 88)[p] = (unsigned char)(int8_t)v; scaled++; }
                } } }
    }
    if (logall || bylba || mode[0] == 't')
        gh_log("LINEKEY #%u t=%u dur=%u bytes=%u len=%u lba0=%u lba1=%u grp=%u cdn=%u mode=%s key=%s gain=%d scaled=%u/%u tail=%08x tn=%u sil=%d unplayed=%u dact=%02x head=%u end=%s%s",
               nchunks, t0, tlast - t0, nbytes, e, lba0, cur_lba, cd_grp - grp0, cd_n - cdn0, mode, key.c_str(), g, scaled, nbytes, th, tn, silv, unplayed, (unsigned)dact, hd0, why, note);
}
static void tailfeed(unsigned char b) {
    const unsigned i = tbn & 1023u;
    if (tbn > 0 && tb[(tbn - 1) & 1023u] == b) trun++; else trun = 1;
    tb[i] = b; tbn++;
}
}
static void khlk_init(const std::string & flag, const std::string & gains, const std::string & tab) { if (!khlk::inited) { khlk::inited = true; khlk::init(flag, gains, tab); } }
/* t845 (KH-J26, 슨상 11-17 22:56 「줄 소리 조정은 배포판에선 안보이게」): 줄 음량 메뉴는 기본 숨김. 앱 폴더(exe 옆)에 lineui.on 파일이 있거나 환경변수 KH_LINEUI=1 일 때만 보인다(시험판). 기능(표·linegain)은 그대로 돈다.
 * settings.ini 의 키로 하지 않은 이유: 설정 저장(ini_put)이 고정 목록으로 파일을 다시 써서 모르는 키(lineui)가 지워진다. */
static bool khlk_lineui() {
    const char * e = getenv("KH_LINEUI"); if (e && *e == '1') return true;
    FILE * f = fopen((gApp.exedir + "\\lineui.on").c_str(), "rb"); if (!f) return false;
    fclose(f); return true;
}
/* 게임 중 메뉴가 열릴 때 부른다(menu_boot.h): 줄 표가 켜져 있으면 줄 이름·값을 Ui 에 채운다 */
static void kh_lk_fill(std::vector<std::string> & n, std::vector<int> & p) { n.clear(); p.clear(); if (kh_linekey_on && khlk_lineui()) khlk::names(n, p); }
/* 화면 줄 슬라이더에서 값이 바뀔 때(A_LINEGAIN): 바로 적용하고 user.ini linegain 에 저장(100 인 줄은 적지 않음) */
static void kh_lk_set(const std::string & name, int pct, int gi) {
    if (!kh_linekey_on || name.empty()) return; if (pct < 0) pct = 0; if (pct > 100) pct = 100;
    if (pct == 100) khlk::gain.erase(name); else khlk::gain[name] = pct;
    const std::string gs = khlk::gains_str(); bool ok = false;
    if (gi >= 0 && gi < (int)gApp.ui_.games.size()) ok = prof::set_game_key(gApp.ui_.games[gi].dir, "linegain", gs);
    gh_log("LINEKEY gain %s=%d (linegain=%s saved=%d)", name.c_str(), pct, gs.empty() ? "(없음)" : gs.c_str(), ok ? 1 : 0);
}
/* KhCdi(linediag.h)가 CD 읽기마다 부른다 — 켜져 있든 아니든 값만 기록 */
static void kh_cdread_hook(unsigned long lba, unsigned long num) {
    using namespace khlk;
    cd_n++; cur_lba = (unsigned)lba; if (cd_end == 0xFFFFFFFFu || (unsigned)lba != cd_end) cd_grp++; cd_end = (unsigned)(lba + num);
}
/* GFX_Events 가 매번 부른다(since = 게임 시작 뒤 ms). 꺼져 있으면 첫 줄에서 돌아간다 */
static void kh_linekey_tick(unsigned since) {
    using namespace khlk;
    if (!kh_linekey_on) return;
    const unsigned w = rd32(40) & 0xFFFFu;
    if (!have_w) { have_w = true; last_w = w; return; }
    const unsigned d = (w - last_w) & 0xFFFFu;
    if (d > 0x4000u) {   /* t845 (KH-P13): 쓰기 위치 되감김·큰 점프 - t844 는 이 구간을 건너뛰어 새 대사 첫머리가 음성 0% 에서도 샜다. 아직 안 읽힌 새 바이트에 현재 LBA 줄 배율을 곱하고 로그를 남긴다. */
        khlk::end("reset");
        const unsigned rp2 = rd32(36) & 0xFFFFu, unp2 = (w - rp2) & 0xFFFFu; const unsigned st2 = (w < last_w) ? 0u : last_w;
        int fi2 = cur_lba ? find_lba(cur_lba) : -1; int g2 = 100; std::string nm2 = "?";
        if (fi2 >= 0) { nm2 = rows[fi2].name; auto it2 = gain.find(nm2); if (it2 != gain.end()) g2 = it2->second; }
        unsigned sc2 = 0, nb2 = 0; const bool b82 = (((const unsigned char *)pcm86)[82] & 0x40) != 0;
        for (unsigned pos2 = st2; pos2 != w; pos2 = (pos2 + 1) & 0xFFFFu) { nb2++; if (g2 != 100 && b82 && ((pos2 - rp2) & 0xFFFFu) < unp2) { scale_byte(pos2, g2); sc2++; } }
        nreset++;
        gh_log("LINEKEY reset #%u t=%u d=%u last_w=%u w=%u rp=%u unp=%u new=%u lba=%u key=%s gain=%d scaled=%u dact=%02x", nreset, since, d, last_w, w, rp2, unp2, nb2, cur_lba, nm2.c_str(), g2, sc2, (unsigned)((const unsigned char *)pcm86)[82]);
        last_w = w; runv = -1; runlen = 0; return;
    }
    if (d) {
        const unsigned char * bf = (const unsigned char *)pcm86 + 88;
        const unsigned rp = rd32(36) & 0xFFFFu, unp = (w - rp) & 0xFFFFu; const bool b8 = (((const unsigned char *)pcm86)[82] & 0x40) != 0;
        for (unsigned i = 0; i < d; i++) {
            const unsigned pos = (last_w + i) & 0xFFFFu; const unsigned char b = bf[pos];
            if ((int)b == runv) runlen++; else { runv = (int)b; runlen = 1; }
            if (runlen >= 512u) { silv = (int)b; skipped++; khlk::end("silence"); continue; }
            if ((int)b == silv && !open_) { skipped++; continue; }
            if (!open_) {
                open_ = true; t0 = tlast = since; nbytes = 0; spos = pos; hd0 = (pos - rp) & 0xFFFFu; tbn = 0; trun = 0; silv = -1; live = 0;
                lba0 = cur_lba; grp0 = cd_grp; cdn0 = cd_n; cgain = 100; bylba = false; cname.clear();
                if (lba0) { const int fi = find_lba(lba0); if (fi >= 0) { bylba = true; cname = rows[fi].name; auto it = gain.find(cname); if (it != gain.end()) cgain = it->second; } }
            }
            nbytes++; tailfeed(b);
            if (bylba && cgain != 100 && b8 && ((pos - rp) & 0xFFFFu) < unp) { scale_byte(pos, cgain); live++; }   /* LBA 키: 처음 바이트부터 곱함(아직 안 읽힌 바이트만) */
        }
        if (open_) tlast = since;
        last_w = w;
    } else if (open_ && since - tlast >= 1000u) khlk::end("gap");
}
/* t846 (KH-P14, 슨상 11-18 07:45 「빌드 ㄱ」): 표 경로 폴백. 기존 exedir\games\<id>\linekey.tsv 가 우선, 없으면 작품의 imgdir\linekey.tsv (노바: exedir 의 games\<id> 는 앱 소유라 adb 로 못 쓰는데 imgdir=게임 폴더에는 표가 이미 있다) */
static std::string khlk_tabpath(const std::string & first, const std::string & imgdir) {
    FILE * f = fopen(first.c_str(), "rb"); if (f) { fclose(f); return first; }
    if (imgdir.empty()) return first;
    const std::string alt = imgdir + "\\linekey.tsv";
    f = fopen(alt.c_str(), "rb"); if (f) { fclose(f); return alt; }
    return first;
}
/* gfx_events.h 의 kh_linediag_tick(since); 앞에 sed 로 끼운다 — gApp 은 호출 자리에서 보인다 */
#define KHLK_HOOK(s) do { static bool lk_ = false; if (!lk_) { lk_ = true; khlk_init(gApp.prof_.kv.count("linekey") ? gApp.prof_.kv["linekey"] : std::string(), \
    gApp.prof_.kv.count("linegain") ? gApp.prof_.kv["linegain"] : std::string(), \
    khlk_tabpath(gApp.exedir + "\\games\\" + std::string(gApp.prof_.id ? gApp.prof_.id : "") + "\\linekey.tsv", gApp.prof_.imgdir)); } kh_linekey_tick(s); } while (0)
