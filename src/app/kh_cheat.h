#pragma once
/* kh_cheat.h -- 치트 1단계(kx-cheat): 화면에 보이는 값으로 RAM 을 찾아 쓰기·고정한다. 옛 cheat-o-matic 처럼 원시적으로.
 * 에뮬 스레드에서만 부른다(게임 중 메뉴 = 중첩 루프, 프레임 훅 = RENDER_EndUpdate).
 * 주소는 절대값으로 저장하지 않는다: 프로그램 로드 기준(PSP 세그먼트*16)에서의 거리 + 앞뒤 8바이트 무늬로 저장하고 다시 찾는다. */
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>
namespace kc {
enum { RAMTOP = 0xA0000, MAXC = 50000, SHOWN = 8, CTX = 8 };
struct Cand { uint32_t a; uint8_t w; };
struct Frz { uint32_t a; uint8_t w; uint32_t v; };
struct Saved { std::string name; int w = 1; uint32_t value = 0; bool freeze = false; long long off = 0; std::string pat; int pre = 0;
    bool lost = false; };
static std::vector<Cand> cand; static bool searched = false, trunc = false;
static std::vector<Frz> frz;
static std::vector<Saved> saved; static std::string saved_id; static bool saved_ok = false;
static void (*presave)(void) = NULL;

static uint32_t rd(uint32_t a, int w) { uint32_t v = 0; for (int i = 0; i < w; i++) v |= (uint32_t)mem_readb(a + i) << (8 * i); return v; }
static void wr(uint32_t a, int w, uint32_t v) { for (int i = 0; i < w; i++) mem_writeb(a + i, (uint8_t)((v >> (8 * i)) & 0xFF)); }
static bool fits(int64_t v, int w) { return v >= 0 && (w >= 4 || v < ((int64_t)1 << (8 * w))); }
static void snapshot(std::vector<uint8_t> & ram) { ram.resize(RAMTOP);
    for (uint32_t i = 0; i < RAMTOP; i++) ram[i] = (uint8_t)mem_readb(i); }
static long long base_addr(void) { return (long long)dos.psp() << 4; }

static void reset(void) { cand.clear(); searched = false; trunc = false; }
static int find(int64_t v) {
    reset(); searched = true; if (v < 0) return 0;
    std::vector<uint8_t> ram; snapshot(ram);
    for (int w = 1; w <= 4; w *= 2) {
        if (!fits(v, w)) continue;
        uint8_t b[4]; for (int i = 0; i < w; i++) b[i] = (uint8_t)((v >> (8 * i)) & 0xFF);
        for (uint32_t a = 0; a + w <= RAMTOP; a++) {
            if (ram[a] != b[0]) continue; bool ok = true; for (int i = 1; i < w; i++) if (ram[a + i] != b[i]) { ok = false; break; }
            if (!ok) continue; if ((int)cand.size() >= MAXC) { trunc = true; return (int)cand.size(); }
            Cand c = { a, (uint8_t)w }; cand.push_back(c);
        }
    }
    return (int)cand.size();
}
static int refine(int64_t v) {
    std::vector<Cand> n;
        for (size_t i = 0; i < cand.size(); i++) if (fits(v, cand[i].w) && (int64_t)rd(cand[i].a, cand[i].w) == v) n.push_back(cand[i]);
    cand.swap(n); trunc = false; return (int)cand.size();
}
static std::string hex8(uint32_t a) { char b[16]; snprintf(b, sizeof b, "%05X", (unsigned)a); return b; }
static std::string list_lines(void) {
    std::string s; for (size_t i = 0; i < cand.size() && i < (size_t)SHOWN; i++) { char b[96];
        snprintf(b, sizeof b, "%s  %dB  = %u\n", hex8(cand[i].a).c_str(), (int)cand[i].w, (unsigned)rd(cand[i].a, cand[i].w)); s += b; }
    return s;
}
/* ---- 쓰기·고정 */
static bool do_write(uint32_t a, int w, int64_t v, bool freeze) {
    if (!fits(v, w) || a + w > RAMTOP) return false;
    if (presave) presave();
    wr(a, w, (uint32_t)v);
    if (freeze) { for (size_t i = 0; i < frz.size(); i++) if (frz[i].a == a) { frz[i].w = (uint8_t)w; frz[i].v = (uint32_t)v; return true;
        } Frz f = { a, (uint8_t)w, (uint32_t)v }; frz.push_back(f); }
    return true;
}
static void tick(void) { for (size_t i = 0; i < frz.size(); i++) wr(frz[i].a, frz[i].w, frz[i].v); }
/* ---- 저장 (사람이 읽는 JSON) */
static std::string dir(void) {
#if defined(__ANDROID__)
    return std::string("/storage/emulated/0/Kyuhachi") + "/cheats";
#else
    std::string e = plat::exe_path(); size_t p = e.find_last_of("\\/");
        return (p == std::string::npos ? std::string(".") : e.substr(0, p)) + "\\cheats";
#endif
}
static std::string path(const std::string & id) {
#if defined(__ANDROID__)
    return dir() + "/" + id + ".json";
#else
    return dir() + "\\" + id + ".json";
#endif
}
static std::string clean(const std::string & s) { std::string o; for (size_t i = 0; i < s.size(); i++) { char c = s[i];
    o += (c == '"' || c == '\\' || (unsigned char)c < 32) ? '_' : c; } return o; }
static std::string jget(const std::string & o, const char * key) {
    std::string k = std::string("\"") + key + "\""; size_t p = o.find(k); if (p == std::string::npos) return ""; p = o.find(':', p);
        if (p == std::string::npos) return "";
    p++; while (p < o.size() && (o[p] == ' ' || o[p] == '\t')) p++;
    if (p < o.size() && o[p] == '"') { size_t q = o.find('"', p + 1); return q == std::string::npos ? "" : o.substr(p + 1, q - p - 1); }
    size_t q = p; while (q < o.size() && o[q] != ',' && o[q] != '}' && o[q] != '\n') q++; std::string r = o.substr(p, q - p);
        while (!r.empty() && (r.back() == ' ' || r.back() == '\r')) r.pop_back(); return r;
}
static bool save_file(const std::string & id) {
    plat::mkdir(dir()); FILE * f = fopen(path(id).c_str(), "wb"); if (!f) return false;
    fprintf(f, "{\n  \"game\": \"%s\",\n  \"note\": \"offset = address - program load base (PSP*16). "
        "pattern = hex bytes around the value (?? = the value itself). Edit by hand if you like.\",\n  \"cheats\": [\n", clean(id).c_str());
    for (size_t i = 0; i < saved.size(); i++) { const Saved & s = saved[i];
        fprintf(f, "    {\"name\": \"%s\", \"width\": %d, \"value\": %u, \"mode\": \"%s\", "
            "\"offset\": %lld, \"before\": %d, \"pattern\": \"%s\"}%s\n", clean(s.name).c_str(), s.w, (unsigned)s.value,
                s.freeze ? "freeze" : "write", s.off, s.pre, s.pat.c_str(), i + 1 < saved.size() ? "," : ""); }
    fprintf(f, "  ]\n}\n"); fclose(f); return true;
}
static void load_file(const std::string & id) {
    if (saved_ok && saved_id == id) return; saved.clear(); saved_id = id; saved_ok = true;
    FILE * f = fopen(path(id).c_str(), "rb"); if (!f) return; std::string t; char b[1024]; size_t n;
        while ((n = fread(b, 1, sizeof b, f)) > 0) t.append(b, n); fclose(f);
    size_t p = t.find("\"cheats\""); if (p == std::string::npos) return;
    while (true) { size_t o = t.find('{', p); if (o == std::string::npos) break; size_t e = t.find('}', o);
        if (e == std::string::npos) break; std::string ob = t.substr(o, e - o + 1); p = e + 1;
        Saved s; s.name = jget(ob, "name"); s.w = atoi(jget(ob, "width").c_str());
            s.value = (uint32_t)strtoul(jget(ob, "value").c_str(), NULL, 10); s.freeze = jget(ob, "mode") == "freeze";
        s.off = atoll(jget(ob, "offset").c_str()); s.pre = atoi(jget(ob, "before").c_str()); s.pat = jget(ob, "pattern");
        if (s.w >= 1 && s.w <= 4 && !s.pat.empty() && (s.pat.size() % 2) == 0) saved.push_back(s); }
}
static int hv(char c) { return c >= '0' && c <= '9' ? c - '0' : (c | 32) >= 'a' && (c | 32) <= 'f' ? (c | 32) - 'a' + 10 : -1; }
static int mism(const std::vector<uint8_t> & ram, const std::string & pat, long long start, int lim) {   /* 무늬와 다른 바이트 수 */
    long long n = (long long)pat.size() / 2; if (start < 0 || start + n > RAMTOP) return 999; int m = 0;
    for (long long i = 0; i < n; i++) { if (pat[i * 2] == '?') continue; int h = hv(pat[i * 2]), l = hv(pat[i * 2 + 1]);
        if (h < 0 || l < 0) return 999; if (ram[(size_t)(start + i)] != (uint8_t)(h * 16 + l) && ++m > lim) return m; }
    return m;
}
/* 앞뒤 바이트가 조금 달라져도(커서·시계 같은 값) 찾는다: 무늬 바이트의 1/4 까지 달라도 받고, 덜 다른 쪽 → 기대 위치에 가까운 쪽 */
static bool relocate(const Saved & s, uint32_t & addr) {
    std::vector<uint8_t> ram; snapshot(ram); const long long n = (long long)s.pat.size() / 2;
    const long long start = base_addr() + s.off - s.pre; int fixedn = 0;
    for (long long i = 0; i < n; i++) if (s.pat[i * 2] != '?') fixedn++;
    const int lim = fixedn / 4; long long best = -1, bd = 0; int bm = lim + 1;
    for (long long a = 0; a + n <= RAMTOP; a++) {
        int m = mism(ram, s.pat, a, lim); if (m > lim) continue; long long d = a > start ? a - start : start - a;
        if (best < 0 || m < bm || (m == bm && d < bd)) { best = a; bm = m; bd = d; }
    }
    if (best < 0) return false; addr = (uint32_t)(best + s.pre); return true;
}
static bool make_saved(int idx, int64_t v, bool freeze, const std::string & id) {
    if (idx < 0 || idx >= (int)cand.size() || !fits(v, cand[idx].w)) return false;
    load_file(id); const Cand & c = cand[idx]; std::vector<uint8_t> ram; snapshot(ram);
    int pre = (int)c.a < CTX ? (int)c.a : CTX; long long post = (long long)RAMTOP - (c.a + c.w); if (post > CTX) post = CTX;
    std::string pat; char b[4];
    for (int i = -pre; i < (int)c.w + (int)post; i++) { if (i >= 0 && i < c.w) pat += "??"; else {
        snprintf(b, sizeof b, "%02x", (unsigned)ram[(size_t)((long long)c.a + i)]); pat += b; } }
    Saved s; char nm[64]; snprintf(nm, sizeof nm, "w%d %u @%s", (int)c.w, (unsigned)v, hex8(c.a).c_str()); s.name = nm; s.w = c.w;
        s.value = (uint32_t)v; s.freeze = freeze; s.off = (long long)c.a - base_addr(); s.pat = pat; s.pre = pre;
    saved.push_back(s); return save_file(id);
}
static std::string saved_lines(const std::string & id) {
    load_file(id); std::string o; for (size_t i = 0; i < saved.size(); i++) {
        o += saved[i].name + (saved[i].freeze ? "  [F]" : "  [W]") + (saved[i].lost ? "  [?]" : "") + "\n"; } return o;
}
static int apply_saved(int idx, const std::string & id) {
    load_file(id); if (idx < 0 || idx >= (int)saved.size()) return -1; Saved & s = saved[idx]; uint32_t a = 0;
    if (!relocate(s, a)) { s.lost = true; return 0; } s.lost = false; return do_write(a, s.w, s.value, s.freeze) ? 1 : -1;
}
static bool del_saved(int idx, const std::string & id) { load_file(id); if (idx < 0 || idx >= (int)saved.size()) return false;
    saved.erase(saved.begin() + idx); return save_file(id); }
}   /* namespace kc */

/* Ui 쪽 호출구: op 0 처음부터 · 1 찾기(v) · 2 다시 찾기(v) · 3 후보 줄(io 에 채움) · 4 쓰기(idx, v) · 5 고정(idx, v) · 6 고정 모두 해제 ·
 * 7 저장 목록(io) · 8 저장본 적용(idx) · 9 후보를 저장(idx, v, io="f"|"w") · 10 저장본 지움(idx) · 11 고정 개수 · 12 지금 후보 수(-1 = 아직 안 찾음) */
static int gh_cheat_op(int op, int idx, int64_t v, std::string & io) {
    const std::string id = gApp.prof_.id ? gApp.prof_.id : "x";
    switch (op) {
    case 0: kc::reset(); return 0;
    case 1: return kc::find(v);
    case 2: return kc::refine(v);
    case 3: io = kc::list_lines(); return (int)kc::cand.size();
    case 4: return idx >= 0 && idx < (int)kc::cand.size() && kc::do_write(kc::cand[idx].a, kc::cand[idx].w, v, false) ? 1 : 0;
    case 5: return idx >= 0 && idx < (int)kc::cand.size() && kc::do_write(kc::cand[idx].a, kc::cand[idx].w, v, true) ? 1 : 0;
    case 6: { int n = (int)kc::frz.size(); kc::frz.clear(); return n; }
    case 7: io = kc::saved_lines(id); return (int)kc::saved.size();
    case 8: return kc::apply_saved(idx, id);
    case 9: return kc::make_saved(idx, v, io == "f", id) ? 1 : 0;
    case 10: return kc::del_saved(idx, id) ? 1 : 0;
    case 11: return (int)kc::frz.size();
    case 12: return kc::searched ? (int)kc::cand.size() : -1;
    }
    return 0;
}
