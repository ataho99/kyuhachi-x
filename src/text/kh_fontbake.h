#pragma once
/* text/kh_fontbake.h -- t865: 「내 글꼴 가져오기」 굽기 (UI·플랫폼 무관). 사용자가 고른 TTF/OTF 를 읽어 t855 규격 비트맵(anex86 BMP 2048x2048 1bpp, 칸 16x16, 열=구·행=점+32, 잉크=0)으로 굽는다.
 * 윤곽 → 안티앨리어싱(stb_truetype) → 문턱 1비트. 한글 = KS X 1001 2350자(구 16~40), 반각 ASCII = 열 0(게임 8칸 / 메뉴 칸폭+폭표 열 100). 글꼴에 없는 기호·일본 문자 칸은 앱 기본 글꼴 시트(base)에서 채운다.
 * 원 글꼴 파일은 읽고 버린다(복사·보관 없음). 구운 결과의 라이선스 책임은 사용자. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <utility>
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "text/stb_truetype.h"
#include "text/ksx_hangul_tab.h"
namespace fontbake {
static const uint8_t kBmpHdr[62] = { 0x42,0x4d,0x3e,0x00,0x08,0x00, 0,0,0,0, 0x3e,0,0,0, 0x28,0,0,0, 0,0x08,0,0, 0,0x08,0,0, 1,0, 1,0, 0,0,0,0, 0,0,0x08,0, 0xc4,0x0e,0,0, 0xc4,0x0e,0,0, 2,0,0,0, 2,0,0,0, 0,0,0,0, 0xff,0xff,0xff,0 };
struct Info { int size = 0, bw = 0, bh = 0, clip = 0, hangul = 0, full = 16, fb_sym = 0, fb_asc = 0; };
struct State { int st = 0, purpose = 0, size = 15, thr = 110, ret_mode = 0, ret_prev = 0; std::string path, stem, disp, msg, saved, saved2; Info info; };
inline int fdiv(int a, int b) { int q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) q--; return q; }
inline std::map<std::string, std::string> & names() { static std::map<std::string, std::string> m; return m; }   /* t866: 파일 이름(u_…) → 표시 이름(UTF-8) */
inline std::string label(const std::string & n) {
    std::map<std::string, std::string>::iterator i = names().find(n); if (i != names().end() && !i->second.empty()) return i->second;
    std::string s = n.size() > 2 && n.compare(0, 2, "u_") == 0 ? n.substr(2) : n; size_t p = s.find_last_of('_');
    if (p != std::string::npos && p + 1 < s.size()) { bool dg = true; for (size_t k = p + 1; k < s.size(); k++) if (s[k] < '0' || s[k] > '9') dg = false; if (dg) s.resize(p); }
    return s;
}
inline void load_side(const std::string & name, const std::string & bmp) {   /* <시트>.txt 첫 줄 = 표시 이름 */
    size_t d = bmp.find_last_of('.'); if (d == std::string::npos) return; FILE * f = fopen((bmp.substr(0, d) + ".txt").c_str(), "rb"); if (!f) return;
    char b[256]; size_t n = fread(b, 1, 255, f); fclose(f); std::string s(b, n); if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) s.erase(0, 3);
    size_t e = s.find_first_of("\r\n"); if (e != std::string::npos) s.resize(e); if (!s.empty()) names()[name] = s;
}
inline std::string raw_stem(const std::string & p) {   /* 파일 이름에서 확장자만 뗀 원래 이름(ANSI 그대로) */
    size_t a = p.find_last_of("/\\"); std::string n = a == std::string::npos ? p : p.substr(a + 1); size_t d = n.find_last_of('.'); if (d != std::string::npos) n = n.substr(0, d); return n;
}
inline std::string stem_of(const std::string & p) {   /* 파일 이름 → 영숫자 소문자 12자 이내(없으면 font) */
    size_t a = p.find_last_of("/\\"); std::string n = a == std::string::npos ? p : p.substr(a + 1); size_t d = n.find_last_of('.'); if (d != std::string::npos) n = n.substr(0, d);
    std::string o; for (size_t i = 0; i < n.size(); i++) { char ch = n[i]; if (ch >= 'A' && ch <= 'Z') ch = (char)(ch - 'A' + 'a'); if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) o += ch; }
    if (o.size() > 12) o.resize(12); return o.empty() ? std::string("font") : o;
}
inline void sheet_init(std::vector<uint8_t> & S) { S.assign(kBmpHdr, kBmpHdr + 62); S.resize(524350, 0xff); }
inline bool sheet_ink(const std::vector<uint8_t> & S, int x, int y) { if (x < 0 || x > 2047 || y < 0 || y > 2047) return false; return !((S[62 + (size_t)(2047 - y) * 256 + (x >> 3)] >> (7 - (x & 7))) & 1); }
inline void sheet_set(std::vector<uint8_t> & S, int x, int y) { if (x < 0 || x > 2047 || y < 0 || y > 2047) return; S[62 + (size_t)(2047 - y) * 256 + (x >> 3)] &= (uint8_t)(~(0x80 >> (x & 7))); }
inline bool cell_empty(const std::vector<uint8_t> & S, int cx, int cy) { for (int j = 0; j < 16; j++) for (int i = 0; i < 16; i++) if (sheet_ink(S, cx * 16 + i, cy * 16 + j)) return false; return true; }
inline void cell_copy(std::vector<uint8_t> & D, const std::vector<uint8_t> & S, int cx, int cy) { for (int j = 0; j < 16; j++) for (int i = 0; i < 16; i++) if (sheet_ink(S, cx * 16 + i, cy * 16 + j)) sheet_set(D, cx * 16 + i, cy * 16 + j); }
struct Box { int x0 = 999, y0 = 999, x1 = -999, y1 = -999; };
struct Baker {
    std::vector<uint8_t> data; stbtt_fontinfo f; bool ok = false;
    int open(const std::string & path) {   /* 0 = 성공 · 1 = 파일 못 읽음 · 2 = 글꼴 형식 아님 */
        ok = false; data.clear(); FILE * fp = fopen(path.c_str(), "rb"); if (!fp) return 1;
        static uint8_t b[65536]; size_t n; while ((n = fread(b, 1, sizeof b, fp)) > 0 && data.size() < ((size_t)64 << 20)) data.insert(data.end(), b, b + n); fclose(fp);
        if (data.size() < 1024) { data.clear(); return 2; }
        int off = stbtt_GetFontOffsetForIndex(data.data(), 0); if (off < 0 || !stbtt_InitFont(&f, data.data(), off)) { data.clear(); return 2; }
        ok = true; return 0;
    }
    void close() { std::vector<uint8_t>().swap(data); ok = false; }
    int hangul_have() { int n = 0; if (!ok) return 0; for (int i = 0; i < 2350; i++) if (stbtt_FindGlyphIndex(&f, kKsxTab[i])) n++; return n; }
    void pts(int cp, float sc, int thr, std::vector<std::pair<int, int> > & out) {   /* 잉크 점(기준선·글자 원점 기준) */
        out.clear(); int x0, y0, x1, y1; stbtt_GetCodepointBitmapBox(&f, cp, sc, sc, &x0, &y0, &x1, &y1); int w = x1 - x0, h = y1 - y0; if (w <= 0 || h <= 0) return;
        static std::vector<uint8_t> buf; buf.assign((size_t)w * h, 0); stbtt_MakeCodepointBitmap(&f, buf.data(), w, h, w, sc, sc, cp);
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) if (buf[(size_t)y * w + x] >= thr) out.push_back(std::make_pair(x0 + x, y0 + y));
    }
    float scale(int s) { return stbtt_ScaleForMappingEmToPixels(&f, (float)s); }
    Box hbox(int s, int thr) {
        Box b; std::vector<std::pair<int, int> > p; const float sc = scale(s);
        for (int i = 0; i < 2350; i++) { if (!stbtt_FindGlyphIndex(&f, kKsxTab[i])) continue; pts(kKsxTab[i], sc, thr, p); for (size_t k = 0; k < p.size(); k++) { b.x0 = std::min(b.x0, p[k].first); b.x1 = std::max(b.x1, p[k].first); b.y0 = std::min(b.y0, p[k].second); b.y1 = std::max(b.y1, p[k].second); } }
        return b;
    }
    int auto_size(int thr) { for (int s = 16; s >= 10; s--) { Box b = hbox(s, thr); if (b.x1 >= b.x0 && b.x1 - b.x0 + 1 <= 15 && b.y1 - b.y0 + 1 <= 16) return s; } return 10; }
    /* purpose 0 = 게임 한글(칸 16 · 반각 8) · 1 = 메뉴(칸 = Info.full · 반각 full/2+2 · 폭표 열 100). base = 앱 기본 글꼴 시트(없으면 NULL) */
    bool bake(int purpose, int s, int thr, const std::vector<uint8_t> * base, std::vector<uint8_t> & out, Info & I) {
        if (!ok) return false; I = Info(); I.size = s; Box b = hbox(s, thr); if (b.x1 < b.x0) return false;
        const int bw = b.x1 - b.x0 + 1, bh = b.y1 - b.y0 + 1; I.bw = bw; I.bh = bh; I.full = std::max(10, std::min(16, bw + 1));
        const int ox = fdiv(15 - bw, 2) - b.x0, oy = fdiv(16 - bh, 2) - b.y0, hw = I.full / 2 + 2;
        std::vector<uint8_t> S; sheet_init(S); std::vector<std::pair<int, int> > p; const float sc = scale(s);
        for (int i = 0; i < 2350; i++) {
            if (!stbtt_FindGlyphIndex(&f, kKsxTab[i])) continue; pts(kKsxTab[i], sc, thr, p); bool cl = false; const int cx = 16 + i / 94, cy = 1 + i % 94 + 32;
            for (size_t k = 0; k < p.size(); k++) { int x = p[k].first + ox, y = p[k].second + oy; if (x < 0 || x > 15 || y < 0 || y > 15) { cl = true; continue; } sheet_set(S, cx * 16 + x, cy * 16 + y); }
            I.hangul++; if (cl) I.clip++;
        }
        for (int c = 0x21; c < 0x7F; c++) {   /* 반각 ASCII */
            if (!stbtt_FindGlyphIndex(&f, c)) continue;
            if (purpose == 0) {   /* 한글 크기부터 내려가며 폭 ≤ 7 에 처음 들어가는 것 */
                for (int t = s; t >= 8; t--) {
                    pts(c, scale(t), thr, p); if (p.empty()) break; int mn = 999, mx = -999; for (size_t k = 0; k < p.size(); k++) { mn = std::min(mn, p[k].first); mx = std::max(mx, p[k].first); }
                    int sh = mn < 0 ? -mn : 0; mx += sh; if (mx > 6) continue; if (mx <= 5 && mn + sh == 0) sh += 1;
                    for (size_t k = 0; k < p.size(); k++) { int x = p[k].first + sh, y = p[k].second + oy; if (x >= 0 && x < 8 && y >= 0 && y < 16) sheet_set(S, x, c * 16 + y); }
                    break;
                }
            } else {
                const int tm = std::max(8, s - 3); pts(c, scale(tm), thr, p); if (p.empty()) continue; int mn = 999, mx = -999; for (size_t k = 0; k < p.size(); k++) { mn = std::min(mn, p[k].first); mx = std::max(mx, p[k].first); }
                int sh = mn < 0 ? -mn : 0; mx += sh; if (mx > 14) continue; if (mx <= hw - 3 && mn + sh == 0) { sh += 1; mx += 1; }
                const int adv = mx <= hw - 1 ? hw : mx + 2;
                for (size_t k = 0; k < p.size(); k++) { int x = p[k].first + sh, y = p[k].second + oy; if (x >= 0 && x < 16 && y >= 0 && y < 16) sheet_set(S, x, c * 16 + y); }
                if (adv != hw) for (int i = 0; i < adv && i < 16; i++) sheet_set(S, 1600 + i, c * 16);
            }
        }
        if (base && base->size() == 524350) {   /* 글꼴에 없는 기호·일본 문자 칸(구 1~8)·못 넣은 반각은 앱 기본 글꼴에서 */
            for (int ku = 1; ku <= 8; ku++) for (int t = 1; t <= 94; t++) if (cell_empty(S, ku, t + 32) && !cell_empty(*base, ku, t + 32)) { cell_copy(S, *base, ku, t + 32); I.fb_sym++; }
            for (int c = 0x21; c < 0x7F; c++) if (cell_empty(S, 0, c) && !cell_empty(*base, 0, c)) { cell_copy(S, *base, 0, c); I.fb_asc++; if (purpose == 1) for (int i = 0; i < 16; i++) if (sheet_ink(*base, 1600 + i, c * 16)) sheet_set(S, 1600 + i, c * 16); }
        }
        out.swap(S); return true;
    }
};
} /* namespace fontbake */
