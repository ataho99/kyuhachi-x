/* kh_stem.h -- #L6: 이미지 안 실행 파일 이름 줄기(stem) 점수로 원판 게임을 찾는다.
 * 호스트(Windows·Android)·시험 도구(Linux) 공용. 원본 이미지는 읽기만 한다.
 *  - image_stems : FD(FAT12) · HDD(PC-98 HDI/NHD/원시 HD, 부트 섹터 0..128 훑기, FAT12/FAT16) 의 전체 폴더에서 .EXE .COM .BAT 줄기를 모은다.
 *                  순환 방지(방문 집합 + 클러스터·폴더 상한) · 이미지 1장 시간 상한 · 못 읽으면 false (조용히 「미확인」).
 *  - Table/rank  : gamedb_nk_stem.tsv(줄기 표)로 게임마다 sum(1/(그 줄기가 나오는 게임 수)) — l5d\score7.py 와 같은 식.
 *  - judge       : 1위/2위 비 ≥ 2 이면 자동(autolink) 아니면 상위 3 후보. */
#pragma once
#include <deque>
#include "settings/kh_prof.h"
#include <chrono>
#include <unordered_map>

namespace stem {

/* DOS 공통 11종 — 줄기에서 뺀다 */
inline bool dos_common(const std::string & s) {
    static const char * const d[] = { "COMMAND", "AUTOEXEC", "CONFIG", "IO", "IO98", "MSDOS", "MEGDOS", "HIMEM", "EMM386", "DOSKEY", "SETVER" };
    for (auto * k : d) if (s == k) return true;
    return false;
}

/* ---------------------------------------------------------------- 이미지 읽기 */
struct Rd {
    FILE * f = NULL; uint64_t base = 0, size = 0;
    Rd() {} Rd(const Rd &) = delete; Rd & operator=(const Rd &) = delete;
    ~Rd() { if (f) fclose(f); }
    bool get(uint64_t off, void * p, size_t n) const { if (!f || off + n > size) return false; if (fseek(f, (long)(base + off), SEEK_SET) != 0) return false; return fread(p, 1, n, f) == n; }
};
inline bool valid_boot(const uint8_t * s, uint32_t ss) {
    if (s[0] != 0xEB && s[0] != 0xE9) return false;
    if (prof::rd16(s + 11) != ss) return false;
    if (s[13] != 1 && s[13] != 2 && s[13] != 4 && s[13] != 8) return false;
    if (s[16] != 1 && s[16] != 2) return false;
    if (prof::rd16(s + 17) == 0) return false;
    return s[21] >= 0xF0;
}
struct Ent { std::string nm, ex; uint8_t at = 0; uint32_t cl = 0; bool ok = false; };
inline Ent parse_ent(const uint8_t * e) {
    Ent x; x.at = e[11]; x.cl = prof::rd16(e + 26);
    for (int i = 0; i < 11; i++) if (e[i] < 0x20 || e[i] > 0x7E) return x;   /* 이름에 제어·비ASCII 바이트 → 건너뜀 */
    std::string n((const char *)e, 8), t((const char *)e + 8, 3);
    while (!n.empty() && n.back() == ' ') n.pop_back(); while (!n.empty() && n[0] == ' ') n.erase(0, 1);
    while (!t.empty() && t.back() == ' ') t.pop_back(); while (!t.empty() && t[0] == ' ') t.erase(0, 1);
    for (auto & c : n) c = (char)toupper((unsigned char)c); for (auto & c : t) c = (char)toupper((unsigned char)c);
    x.nm = n; x.ex = t; x.ok = true; return x;
}
inline bool is_exec_ext(const std::string & x) { return x == "EXE" || x == "COM" || x == "BAT"; }

struct Walker {
    const Rd * r = NULL; std::vector<uint8_t> fat; bool f16 = false; uint64_t data_off = 0, clus_sz = 0; uint32_t lim = 0xFF0; size_t chain_cap = 64, dir_cap = 1024, dirs = 0;
    std::set<uint32_t> visited, active; std::set<std::string> * out = NULL; std::chrono::steady_clock::time_point t_end; bool bad = false;
    uint32_t next(uint32_t c) const {
        if (f16) { size_t o = (size_t)c * 2; if (o + 1 >= fat.size()) return 0xFFFF; return prof::rd16(&fat[o]); }
        size_t o = (size_t)c + c / 2; if (o + 1 >= fat.size()) return 0xFFF; uint32_t w = fat[o] | (fat[o + 1] << 8); return (c & 1) ? (w >> 4) : (w & 0xFFF);
    }
    void add(const Ent & e) { if (e.ok && is_exec_ext(e.ex) && !e.nm.empty()) out->insert(e.nm); }
    void walk(uint32_t cl, int depth) {   /* 순환·시간 초과면 bad = true (호출한 쪽이 통째로 버린다) */
        if (depth > 16 || bad) return;
        std::vector<uint32_t> chain; std::set<uint32_t> in_chain; uint32_t c = cl;
        while (c >= 2 && c < lim) {
            if (in_chain.count(c) || active.count(c)) { bad = true; return; }
            in_chain.insert(c); chain.push_back(c);
            if (chain.size() > chain_cap) { bad = true; return; }
            c = next(c);
        }
        for (uint32_t k : chain) {
            if (visited.count(k)) continue;
            if (active.count(k)) { bad = true; return; }
            active.insert(k);
            if (++dirs > dir_cap || std::chrono::steady_clock::now() > t_end) { bad = true; return; }
            std::vector<uint8_t> d((size_t)clus_sz);
            if (r->get(data_off + (uint64_t)(k - 2) * clus_sz, &d[0], d.size())) {
                for (size_t i = 0; i + 32 <= d.size(); i += 32) {
                    const uint8_t * e = &d[i]; if (e[0] == 0x00) break; if (e[0] == 0xE5) continue; if (e[11] & 0x08) continue;
                    Ent x = parse_ent(e); if (!x.ok || x.nm == "." || x.nm == "..") continue;
                    if (x.at & 0x10) { if (x.cl >= 2) { walk(x.cl, depth + 1); if (bad) return; } }
                    else add(x);
                }
            }
            active.erase(k); visited.insert(k);
        }
    }
};

/* im: prof::probe 결과. 성공하면 out 에 줄기(대문자·DOS 공통 11종 뺌)를 더하고 true. 지원하지 않는 형식·비FAT·순환·시간 초과는 false(오류창 없음). */
inline bool image_stems(const prof::ImgInfo & im, std::set<std::string> & out, int limit_ms = 20000) {
    using namespace prof;
    if (!(im.fmt == F_FDI || im.fmt == F_HDI || im.fmt == F_NHD || im.fmt == F_RAWFD || im.fmt == F_RAWHD)) return false;
    if (im.size <= (uint64_t)im.hdr + 1024) return false;
    Rd r; r.f = fopen(im.path.c_str(), "rb"); if (!r.f) return false;
    r.base = im.hdr; r.size = im.size - im.hdr;
    const bool small = r.size <= 2000000ull; uint32_t ss = 0, b = 0; uint8_t bs[1024]; bool got = false;
    const uint32_t ssl[2] = { 1024, 512 };
    for (int k = 0; k < 2 && !got; k++) {
        uint32_t s = ssl[k]; uint64_t nsec = r.size / s, last = small ? 1 : std::min<uint64_t>(nsec, 129);
        for (uint64_t i = 0; i < last; i++) { if (!r.get(i * s, bs, s)) break; if (valid_boot(bs, s)) { ss = s; b = (uint32_t)i; got = true; break; } }
    }
    if (!got) return false;
    uint32_t spc = bs[13], rsvd = rd16(bs + 14), nfat = bs[16], rootent = rd16(bs + 17), spfat = rd16(bs + 22);
    if (rsvd == 0 || spfat == 0 || rootent > 4096 || (uint64_t)spfat * ss > (4u << 20)) return false;
    const uint64_t fat_off = (uint64_t)(b + rsvd) * ss, root_off = (uint64_t)(b + rsvd + nfat * spfat) * ss;
    const uint64_t root_secs = ((uint64_t)rootent * 32 + ss - 1) / ss, data_off = root_off + root_secs * ss, clus_sz = (uint64_t)spc * ss;
    if (data_off >= r.size) return false;
    const uint64_t nclus = (r.size - data_off) / clus_sz;
    uint64_t tot = rd16(bs + 19); if (!tot) tot = rd32(bs + 32);
    uint64_t meta = (uint64_t)rsvd + (uint64_t)nfat * spfat + root_secs, cl_total = (tot > meta) ? (tot - meta) / spc : nclus;
    Walker w; w.r = &r; w.f16 = cl_total >= 4085; w.lim = w.f16 ? 0xFFF0 : 0xFF0; w.data_off = data_off; w.clus_sz = clus_sz;
    w.fat.resize((size_t)spfat * ss); if (!r.get(fat_off, &w.fat[0], w.fat.size())) return false;
    w.chain_cap = std::min<uint64_t>(4096, std::max<uint64_t>(64, nclus + 16)); w.dir_cap = (size_t)std::max<uint64_t>(1024, nclus + 1024);
    w.t_end = std::chrono::steady_clock::now() + std::chrono::milliseconds(limit_ms);
    std::set<std::string> got_st; w.out = &got_st;
    std::vector<uint8_t> root((size_t)rootent * 32); if (!r.get(root_off, &root[0], root.size())) return false;
    std::vector<uint32_t> subs;
    for (size_t i = 0; i + 32 <= root.size(); i += 32) {
        const uint8_t * e = &root[i]; if (e[0] == 0x00 || e[0] == 0xE5) continue; if (e[11] & 0x08) continue;
        Ent x = parse_ent(e); if (!x.ok) continue;
        if (x.at & 0x10) { if (x.cl >= 2) subs.push_back(x.cl); } else w.add(x);
    }
    for (uint32_t cl : subs) { w.walk(cl, 0); if (w.bad) return false; }
    for (auto & s : got_st) if (!dos_common(s)) out.insert(s);
    return true;
}
/* 작품 하나를 이루는 이미지들의 줄기 합집합. 한 장이라도 읽혔고 줄기가 비어 있지 않으면 true. */
inline bool work_stems(const std::vector<prof::ImgInfo> & imgs, std::set<std::string> & out, int limit_ms = 20000) {
    for (auto & im : imgs) { std::set<std::string> s; if (image_stems(im, s, limit_ms)) out.insert(s.begin(), s.end()); }
    return !out.empty();
}

/* ---------------------------------------------------------------- 줄기 표 */
struct Game { std::string name, gid_hd, gid_fd, gid_any; std::vector<int> st; };   /* name = 「퍼블리셔/게임」 폴더 이름 */
struct Table { std::vector<std::string> stems; std::vector<int> cnt; std::unordered_map<std::string, int> sid; std::vector<Game> g; bool loaded = false; };
/* 표: 「S<TAB>줄기<TAB>나온 게임 수」 줄이 먼저(순서 = 번호), 「G<TAB>이름<TAB>HD gid<TAB>FD gid<TAB>아무 gid<TAB>줄기번호,…」 */
inline bool load(const std::string & path, Table & t) {
    FILE * f = fopen(path.c_str(), "rb"); if (!f) return false;
    std::string data; { char b[1 << 16]; size_t n; while ((n = fread(b, 1, sizeof b, f)) > 0) data.append(b, n); } fclose(f);
    t = Table(); size_t pos = 0;
    while (pos < data.size()) {
        size_t e = data.find('\n', pos); if (e == std::string::npos) e = data.size();
        std::string ln = data.substr(pos, e - pos); pos = e + 1; if (!ln.empty() && ln.back() == '\r') ln.pop_back(); if (ln.size() < 3 || ln[1] != '\t') continue;
        std::vector<std::string> v; { size_t p = 0; for (;;) { size_t q = ln.find('\t', p); if (q == std::string::npos) { v.push_back(ln.substr(p)); break; } v.push_back(ln.substr(p, q - p)); p = q + 1; } }
        if (v[0] == "S" && v.size() >= 3) { t.sid[v[1]] = (int)t.stems.size(); t.stems.push_back(v[1]); t.cnt.push_back(atoi(v[2].c_str())); }
        else if (v[0] == "G" && v.size() >= 6) {
            Game g; g.name = v[1]; g.gid_hd = v[2]; g.gid_fd = v[3]; g.gid_any = v[4];
            size_t p = 0; const std::string & s = v[5]; while (p < s.size()) { size_t q = s.find(',', p); if (q == std::string::npos) q = s.size(); int id = atoi(s.substr(p, q - p).c_str()); if (id >= 0 && id < (int)t.stems.size()) g.st.push_back(id); p = q + 1; }
            std::sort(g.st.begin(), g.st.end()); t.g.push_back(g);
        }
    }
    t.loaded = !t.g.empty() && !t.stems.empty(); return t.loaded;
}

struct Cand { int g = -1; double score = 0; int nhit = 0; };
struct Verdict { std::vector<Cand> top; bool autolink = false; double ratio = 0; };   /* top = 점수 > 0 인 상위 3 · ratio = 1위/2위(2위가 없으면 0 — autolink 는 true) */

/* 점수: 줄기마다 1/(나온 게임 수) 를 게임별로 더함. 순위 = 점수 내림차순, 같으면 게임 이름순(score7.py 와 같다). 표에 없는 줄기는 점수 0. */
inline Verdict judge(const Table & t, const std::set<std::string> & stems) {
    Verdict v; if (!t.loaded) return v;
    std::vector<int> known; for (auto & s : stems) { auto it = t.sid.find(s); if (it != t.sid.end() && t.cnt[it->second] > 0) known.push_back(it->second); }
    if (known.empty()) return v;
    std::sort(known.begin(), known.end()); known.erase(std::unique(known.begin(), known.end()), known.end());
    std::vector<Cand> all;
    for (size_t gi = 0; gi < t.g.size(); gi++) {
        const Game & g = t.g[gi]; Cand c; c.g = (int)gi;
        for (int id : known) if (std::binary_search(g.st.begin(), g.st.end(), id)) { c.score += 1.0 / t.cnt[id]; c.nhit++; }
        if (c.nhit) all.push_back(c);
    }
    std::sort(all.begin(), all.end(), [&](const Cand & a, const Cand & b) { if (a.score != b.score) return a.score > b.score; return t.g[a.g].name < t.g[b.g].name; });
    for (size_t i = 0; i < all.size() && i < 3; i++) v.top.push_back(all[i]);
    if (v.top.empty()) return v;
    if (v.top.size() < 2 || v.top[1].score <= 0) { v.autolink = true; v.ratio = 0; }
    else { v.ratio = v.top[0].score / v.top[1].score; v.autolink = v.ratio >= 2.0; }
    return v;
}
/* HDD 이미지가 있는 작품이면 HD 판, 아니면 FD 판 gid(없으면 다른 쪽·아무거나) */
inline const std::string & pick_gid(const Game & g, bool hdd) {
    const std::string & a = hdd ? g.gid_hd : g.gid_fd, & b = hdd ? g.gid_fd : g.gid_hd;
    if (!a.empty()) return a; if (!b.empty()) return b; return g.gid_any;
}
/* game.ini 에 적는 후보: 「gid~이름~일치줄기수」 를 「 | 」 로 이은 한 줄 */
inline std::string cand_pack(const Table & t, const Verdict & v, bool hdd) {
    std::string o; for (auto & c : v.top) { const Game & g = t.g[c.g]; if (!o.empty()) o += " | "; o += pick_gid(g, hdd) + "~" + g.name + "~" + std::to_string(c.nhit); } return o;
}
struct Pack { std::string gid, name; int n = 0; };
inline std::vector<Pack> cand_unpack(const std::string & s) {
    std::vector<Pack> r; size_t p = 0;
    while (p <= s.size() && !s.empty()) {
        size_t q = s.find(" | ", p); std::string it = s.substr(p, q == std::string::npos ? std::string::npos : q - p);
        size_t a = it.find('~'), b = a == std::string::npos ? a : it.find('~', a + 1);
        if (b != std::string::npos) { Pack k; k.gid = it.substr(0, a); k.name = it.substr(a + 1, b - a - 1); k.n = atoi(it.c_str() + b + 1); r.push_back(k); }
        if (q == std::string::npos) break; p = q + 3;
    }
    return r;
}

} /* namespace stem */
