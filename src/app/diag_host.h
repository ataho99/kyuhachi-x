#pragma once
/* app/diag_host.h -- 설정 > 진단 (메뉴 정리안 §7): 진동 시험 · 컨트롤러 정보 · 진단 로그 저장/복사.
   * gh_vibe(menu_boot.h) 뒤에 포함한다. 로그는 개인정보(경로의 사용자 이름·기기/컨트롤러 일련번호)를 가려서 만든다. */
#ifdef __ANDROID__
#include <sys/system_properties.h>
#endif
static int g_diag_masked = 0;
static std::string gh_diag_mask(std::string s) {   /* 윈도 경로 \Users\<이름>\ · 안드 /Users 계열 → <user> */
    const char * keys[] = { "\\Users\\", "\\users\\", "/Users/", "/users/" };
    for (const char * k : keys) {
        size_t p = 0; const size_t kl = strlen(k);
        while ((p = s.find(k, p)) != std::string::npos) {
            size_t b = p + kl, e = b; while (e < s.size() && s[e] != '\\' && s[e] != '/' && s[e] != '\r' && s[e] != '\n' && s[e] != ' ' && s[e] != '"') e++;
            if (e > b && s.compare(b, e - b, "<user>") != 0) { s.replace(b, e - b, "<user>"); g_diag_masked++; }
            p = b + 6;
        }
    }
    return s;
}
static std::string gh_diag_serial(const std::string & s) { if (s.empty()) return T("(없음)"); g_diag_masked++; return s.substr(0, 2) + "**"; }
static std::string gh_diag_devinfo(void) {
    std::string o;
    char b[160];
#ifdef __ANDROID__
    char mk[PROP_VALUE_MAX] = "", md[PROP_VALUE_MAX] = "", rl[PROP_VALUE_MAX] = "", sd[PROP_VALUE_MAX] = "";
    __system_property_get("ro.product.manufacturer", mk); __system_property_get("ro.product.model", md);
    __system_property_get("ro.build.version.release", rl); __system_property_get("ro.build.version.sdk", sd);
    snprintf(b, sizeof b, T("기기: %s %s · Android %s (API %s)\n"), mk, md, rl, sd); o += b;
#else
    snprintf(b, sizeof b, T("기기: %s (윈도 버전 자세히는 수집 안 함)\n"), SDL_GetPlatform()); o += b;
#endif
    SDL_version sv; SDL_GetVersion(&sv); snprintf(b, sizeof b, T("SDL %d.%d.%d · CPU 스레드 %d\n"), sv.major, sv.minor, sv.patch, SDL_GetCPUCount()); o += b;
    return o;
}
static std::string gh_diag_ctrls(void) {   /* 연결된 컨트롤러: 이름 · VID/PID · 입력 소스 */
    std::string o; char b[256]; const int n = SDL_NumJoysticks();
    snprintf(b, sizeof b, T("연결된 입력 장치: %d\n"), n); o += b;
    for (int i = 0; i < n; i++) {
        const char * nm = SDL_JoystickNameForIndex(i); int vid = 0, pid = 0;
#if SDL_VERSION_ATLEAST(2, 0, 6)
        vid = SDL_JoystickGetDeviceVendor(i); pid = SDL_JoystickGetDeviceProduct(i);
#endif
        std::string src = SDL_IsGameController(i) ? T("게임 컨트롤러 매핑 있음") : T("조이스틱(매핑 없음)");
        snprintf(b, sizeof b, " [%d] %s · VID %04X PID %04X · %s\n", i, nm ? nm : T("(이름 없음)"), vid, pid, src.c_str()); o += b;
    }
    return o;
}
/* 기기 진동기 시험(t831): 100ms 켬 / 200ms 쉼 / 300ms 켬. 「호출 성공」이 「울렸다」는 뜻이 아니므로 결과를 구분해 적는다 — 실제로 느껴졌는지는 사람이 확인 */
static std::string gh_diag_devvibe(void) {
#ifdef __ANDROID__
    JNIEnv * env = (JNIEnv *)SDL_AndroidGetJNIEnv(); jobject act = (jobject)SDL_AndroidGetActivity();
    if (!env || !act) return T("기기 진동기: 호출 실패 (JNI 환경/Activity 없음)");
    jclass c = env->GetObjectClass(act); jmethodID m2 = c ? env->GetMethodID(c, "khVibrate2", "(II)I") : NULL;
    if (env->ExceptionCheck()) { env->ExceptionClear(); m2 = NULL; }
    jmethodID mi = c ? env->GetMethodID(c, "khVibeInfo", "()Ljava/lang/String;") : NULL;
    if (env->ExceptionCheck()) { env->ExceptionClear(); mi = NULL; }
    if (!m2) { if (c) env->DeleteLocalRef(c); env->DeleteLocalRef(act); return T("기기 진동기: 진동 연결(khVibrate2)을 찾지 못함 - 이 빌드엔 새 진단 없음"); }
    std::string info; if (mi) { jstring js = (jstring)env->CallObjectMethod(act, mi); if (env->ExceptionCheck()) env->ExceptionClear(); else if (js) { const char * u = env->GetStringUTFChars(js, NULL); if (u) { info = u; env->ReleaseStringUTFChars(js, u); } env->DeleteLocalRef(js); } }
    int r1 = env->CallIntMethod(act, m2, (jint)100, (jint)255); if (env->ExceptionCheck()) { env->ExceptionClear(); r1 = -1; }
    SDL_Delay(200);
    int r2 = env->CallIntMethod(act, m2, (jint)300, (jint)255); if (env->ExceptionCheck()) { env->ExceptionClear(); r2 = -1; }
    env->DeleteLocalRef(c); env->DeleteLocalRef(act);
    const int r = r2 > r1 ? r2 : r1; const char * w = r < 0 ? T("호출 중 예외 발생") : r == 0 ? T("울릴 진동기가 없음 (기기 진동기 없음 · 게임패드 진동기 없음)") : r == 1 ? T("기기 진동기 호출됨") : r == 2 ? T("게임패드 진동기 호출됨") : T("기기·게임패드 진동기 모두 호출됨");
    std::string o = std::string(T("진동: ")) + w; if (r > 0) o += std::string(" - ") + T("호출됨은 실제로 울렸다는 뜻이 아님. 느껴졌는지 직접 확인");
    if (!info.empty()) o += "\n  " + info;
    return o;
#else
    return T("기기 진동기: 해당 없음 (윈도는 기기 진동기가 없음)");
#endif
}
/* 컨트롤러 진동 시험: 각 장치에 같은 패턴. 반환 = 장치별 결과 줄 */
static std::string gh_diag_ctlvibe(void) {
    std::string o; char b[256]; const int n = SDL_NumJoysticks();
    if (n <= 0) return T("컨트롤러 진동: 연결된 장치 없음\n");
    for (int i = 0; i < n; i++) {
        const char * nm = SDL_JoystickNameForIndex(i);
#if SDL_VERSION_ATLEAST(2, 0, 9)
        SDL_Joystick * j = SDL_JoystickOpen(i);
        if (!j) { snprintf(b, sizeof b, T("컨트롤러 진동 [%d] %s: 열기 실패 (%s)\n"), i, nm ? nm : "?", SDL_GetError()); o += b; continue; }
        int r1 = SDL_JoystickRumble(j, 0xFFFF, 0xFFFF, 100); SDL_Delay(200);
        int r2 = SDL_JoystickRumble(j, 0xFFFF, 0xFFFF, 300);
        std::string er = (r1 != 0 || r2 != 0) ? std::string(T(" · 오류: ")) + SDL_GetError() : std::string();
        snprintf(b, sizeof b, T("컨트롤러 진동 [%d] %s: %s (반환 %d,%d)%s\n"), i, nm ? nm : "?", (r1 == 0 && r2 == 0) ? T("호출 성공") : T("미지원/실패"), r1, r2, er.c_str()); o += b;
        SDL_JoystickClose(j);
#else
        snprintf(b, sizeof b, T("컨트롤러 진동 [%d] %s: 이 SDL 버전은 진동 API 없음\n"), i, nm ? nm : "?"); o += b;
#endif
    }
    return o;
}
static std::string gh_diag_tail_log(int nmax) {   /* 지금 로그 파일의 마지막 에러 줄 최대 nmax 개 */
    std::vector<std::string> keep; FILE * f = fopen((gApp.outdir + "\\kyuhachi_log.txt").c_str(), "rb"); if (!f) return T("(로그 파일을 못 열었음)\n");
    std::string line; int ch;
    auto flush = [&]() { std::string l = line; line.clear(); if (l.empty()) return;
        std::string lo = l; for (auto & x : lo) x = (char)tolower((unsigned char)x);
        if (lo.find("fail") != std::string::npos || lo.find("error") != std::string::npos || l.find("실패") != std::string::npos || l.find("오류") != std::string::npos || l.find("예외") != std::string::npos) { keep.push_back(l); if ((int)keep.size() > nmax) keep.erase(keep.begin()); } };
    while ((ch = fgetc(f)) != EOF) { if (ch == '\n') flush(); else if (ch != '\r' && line.size() < 400) line += (char)ch; }
    flush(); fclose(f);
    std::string o; for (auto & l : keep) o += l + "\n"; if (o.empty()) o = T("(에러 줄 없음)\n"); return o;
}
/* op 0 진동 시험(기기+컨트롤러) · 2 컨트롤러 정보 · 3 전체 로그 본문 */
static std::string gh_diag_op(int op, const std::string & note) {
    g_diag_masked = 0;
    if (op == 0) return gh_diag_devvibe() + "\n" + gh_diag_ctlvibe();
    if (op == 2) return gh_diag_ctrls();
    std::string o = T("큐하치 진단 로그\n");
    o += std::string(T("앱: ")) + KH_VERSION + std::string(T(" · 빌드 ")) + __DATE__ + " " + __TIME__ + "\n";
    o += gh_diag_devinfo(); o += gh_diag_ctrls();
    o += std::string(T("\n[진동]\n")) + gh_diag_devvibe() + "\n" + gh_diag_ctlvibe();
    { const ui::Settings & s = gApp.ui_.g; char b[96]; snprintf(b, sizeof b, T("전역 설정: vibe=%d vibe_pct=%d\n"), s.vibe, s.vibe_pct); o += b;
      const int cg = gApp.cur_game; if (cg >= 0 && cg < (int)ui::Ui::MAXG && gApp.ui_.has_t[cg]) { snprintf(b, sizeof b, T("이 작품 설정: vibe=%d vibe_pct=%d\n"), gApp.ui_.t[cg].vibe, gApp.ui_.t[cg].vibe_pct); o += b; } }
    if (!note.empty()) o += std::string(T("\n[사용자 시험 응답]\n")) + note + "\n";
    o += std::string(T("\n[마지막 에러 줄 50개]\n")) + gh_diag_tail_log(50);
    o = gh_diag_mask(o);
    char b[64]; snprintf(b, sizeof b, T("\n(개인정보 가린 항목 %d개)\n"), g_diag_masked); o += b;
    return o;
}
static std::string gh_diag_save(const std::string & text) {   /* 반환 = 저장한 경로, 실패 시 빈 문자열 */
    char tb[32]; time_t t = time(NULL); struct tm * tmv = localtime(&t); if (tmv) strftime(tb, sizeof tb, "%Y%m%d_%H%M%S", tmv); else snprintf(tb, sizeof tb, "log");
    std::string name = std::string("kyuhachi_diag_") + tb + ".txt";
#ifdef __ANDROID__
    const char * dirs[] = { "/storage/emulated/0/Download", "/sdcard/Download" };
    for (const char * d : dirs) { std::string p = std::string(d) + "/" + name; FILE * f = fopen(p.c_str(), "wb"); if (f) { fwrite(text.data(), 1, text.size(), f); fclose(f); return p; } }
    return std::string();
#else
    std::string p = gApp.exedir + "\\" + name; FILE * f = fopen(p.c_str(), "wb"); if (!f) return std::string();
    fwrite(text.data(), 1, text.size(), f); fclose(f); return p;
#endif
}
static void gh_diag_env_install(void) {
    ui::Env & E = gApp.ui_.env;
    E.diag = [](int op, const std::string & note) { return gh_diag_op(op, note); };
    E.diag_save = [](const std::string & text) { return gh_diag_save(text); };
}
