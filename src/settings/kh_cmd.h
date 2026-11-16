/* settings/kh_cmd.h -- hanacd: 폴더 안 .cmd(RetroArch np2kai 방식) 해석. 순수 문자열 처리(파일 접근 없음).
   .cmd 한 줄 = np2kai "a.hdm" "b.hdi" "c.ccd" — 파일 이름만 확장자별로 FD/HDD/CD 로 가른다. 이 헤더는 한 번만 포함된다. */
namespace prof {
struct CmdSlots { std::vector<std::string> fd, hd, cd; };

inline std::vector<std::string> cmd_tokens(const std::string & s) {
    std::vector<std::string> out; size_t i = 0, n = s.size();
    while (i < n) {
        while (i < n && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) i++;
        if (i >= n) break;
        std::string t;
        if (s[i] == '"') { i++; while (i < n && s[i] != '"') t += s[i++]; if (i < n) i++; }
        else while (i < n && s[i] != ' ' && s[i] != '\t' && s[i] != '\r' && s[i] != '\n') t += s[i++];
        if (!t.empty()) out.push_back(t);
    }
    return out;
}

/* 1 = FD, 2 = HDD, 3 = CD(시트·ISO), 4 = CD 데이터(.img/.bin — 시트가 없을 때만 CD), 0 = 모름 */
inline int cmd_kind(const std::string & ext) {
    static const char * const fd[] = { ".hdm", ".fdi", ".d88", ".d98", ".88d", ".98d", ".2hd", ".xdf", ".dup", ".tfd", ".fdd", ".nfd", 0 };
    static const char * const hd[] = { ".hdi", ".thd", ".nhd", ".hdd", ".hdn", ".nvd", 0 };
    static const char * const cd[] = { ".ccd", ".cue", ".iso", ".mdf", 0 };
    for (int i = 0; fd[i]; i++) if (ext == fd[i]) return 1;
    for (int i = 0; hd[i]; i++) if (ext == hd[i]) return 2;
    for (int i = 0; cd[i]; i++) if (ext == cd[i]) return 3;
    if (ext == ".img" || ext == ".bin") return 4;
    return 0;
}

/* txt = .cmd 내용, files = 그 폴더의 파일 이름들. 폴더에 실제로 있는 파일만 받는다(이름은 대소문자 무시로 맞춰 실제 이름을 돌려줌). */
inline CmdSlots cmd_parse(const std::string & txt, const std::vector<std::string> & files) {
    CmdSlots r; std::string data;
    for (auto & t : cmd_tokens(txt)) {
        size_t d = t.find_last_of("\\/"); std::string b = d == std::string::npos ? t : t.substr(d + 1);
        std::string e = ext_of(b); if (e.empty()) continue;          /* 맨 앞 np2kai 같은 코어 이름 */
        const std::string * hit = NULL; for (auto & f : files) if (lower_s(f) == lower_s(b)) { hit = &f; break; }
        if (!hit) continue;
        int k = cmd_kind(e);
        if (k == 1) r.fd.push_back(*hit); else if (k == 2) r.hd.push_back(*hit);
        else if (k == 3) r.cd.push_back(*hit); else if (k == 4 && data.empty()) data = *hit;
    }
    if (r.cd.empty() && !data.empty()) r.cd.push_back(data);
    return r;
}
}
