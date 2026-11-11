#pragma once
/* settings/kh_info.h -- v0.6.8 라이브러리 메타(library_meta.tsv) 조회. 파일이 없으면 기능만 꺼진다(앱은 그대로).
 * 형식: 탭 구분 UTF-8, 첫 줄 헤더. 쓰는 열: id title_ko title_jp publisher year genre intro1 intro2 intro3. 소개는 우리가 쓴 글만. */
#include <stdio.h>
#include <string>
#include <vector>
#include <map>

namespace libmeta {
struct Row { std::string title_ko, title_jp, publisher, year, genre, intro[3]; };
inline std::map<std::string, Row> & tbl() { static std::map<std::string, Row> t; return t; }
inline bool & loaded() { static bool b = false; return b; }
inline std::vector<std::string> split_tab(const std::string & s) {
    std::vector<std::string> v; size_t a = 0;
    for (;;) { size_t b = s.find('\t', a); if (b == std::string::npos) { v.push_back(s.substr(a)); break; } v.push_back(s.substr(a, b - a)); a = b + 1; }
    return v;
}
inline void load(const std::string & path) {
    if (loaded()) return; loaded() = true;
    FILE * f = fopen(path.c_str(), "rb"); if (!f) return;
    std::string all; char buf[4096]; size_t n; while ((n = fread(buf, 1, sizeof buf, f)) > 0) all.append(buf, n); fclose(f);
    if (all.size() >= 3 && (unsigned char)all[0] == 0xEF && (unsigned char)all[1] == 0xBB && (unsigned char)all[2] == 0xBF) all.erase(0, 3);
    std::map<std::string, int> col; bool hdr = true; size_t pos = 0;
    while (pos < all.size()) {
        size_t e = all.find('\n', pos); std::string ln = all.substr(pos, e == std::string::npos ? std::string::npos : e - pos); pos = e == std::string::npos ? all.size() : e + 1;
        if (!ln.empty() && ln[ln.size() - 1] == '\r') ln.erase(ln.size() - 1);
        if (ln.empty()) continue;
        std::vector<std::string> c = split_tab(ln);
        if (hdr) { for (size_t i = 0; i < c.size(); i++) col[c[i]] = (int)i; hdr = false; if (!col.count("id")) return; continue; }
        auto g = [&](const char * k) -> std::string { std::map<std::string, int>::const_iterator it = col.find(k); if (it == col.end() || it->second >= (int)c.size()) return std::string(); std::string s = c[it->second]; if (s.size() > 400) s.resize(400); return s; };
        std::string id = g("id"); if (id.empty()) continue;
        Row r; r.title_ko = g("title_ko"); r.title_jp = g("title_jp"); r.publisher = g("publisher"); r.year = g("year"); r.genre = g("genre");
        r.intro[0] = g("intro1"); r.intro[1] = g("intro2"); r.intro[2] = g("intro3");
        tbl()[id] = r;
    }
}
inline const Row * find(const std::string & id) { std::map<std::string, Row>::const_iterator it = tbl().find(id); return it == tbl().end() ? NULL : &it->second; }
}   /* namespace libmeta */
