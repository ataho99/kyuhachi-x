#pragma once
/* app/folder_pick.h -- t55b: 안드로이드 전용 폴더 탐색기(앱 안 화면) + 「모든 파일 접근」 권한 확인·요청(JNI).
 * 윈도우의 폴더 대화상자(plat::pick_folder_a) 대신 gh_pick_folder 가 이 화면을 연다. 메뉴 캔버스(640x400)에 그리고 패드·키·터치로 조작한다.
 * 첫 화면 = 추천 폴더(Kyuhachi·Download) + 저장소 루트(내장·외장 SD). 폴더 안 = 「이 폴더 사용」·위로·하위 폴더.
 * 터치는 기존 gh_touch_feed 가 키 이벤트로 바꿔 준다(줄 좌표는 gApp.ui_.hits).
 * B2 초안: 「파일 모드」 추가 — gh_pick_files 가 이 탐색기를 연다(폴더 안 이미지 확장자 파일 다중 선택 → 「선택 등록」). 마지막에 연 폴더를 기억한다. */
#if defined(__ANDROID__)
#include <jni.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <set>
#include "app/folder_text.inc"

static bool gh_all_files_ok(void) {   /* Environment.isExternalStorageManager(); true when API < 30 or JNI fails (do not block) */
    JNIEnv * env = (JNIEnv *)SDL_AndroidGetJNIEnv(); if (!env) return true;
    jclass c = env->FindClass("android/os/Environment"); if (!c) { env->ExceptionClear(); return true; }
    jmethodID m = env->GetStaticMethodID(c, "isExternalStorageManager", "()Z"); if (!m) { env->ExceptionClear(); return true; }
    jboolean r = env->CallStaticBooleanMethod(c, m); if (env->ExceptionCheck()) { env->ExceptionClear(); return true; }
    return r == JNI_TRUE;
}
static void gh_all_files_ask(void) {   /* opens this app's all-files-access settings page (falls back to the generic list) */
    JNIEnv * env = (JNIEnv *)SDL_AndroidGetJNIEnv(); jobject act = (jobject)SDL_AndroidGetActivity(); if (!env || !act) return;
    jclass ac = env->GetObjectClass(act), ic = env->FindClass("android/content/Intent"), uc = env->FindClass("android/net/Uri");
    jmethodID gpn = env->GetMethodID(ac, "getPackageName", "()Ljava/lang/String;"), sa = env->GetMethodID(ac, "startActivity",
        "(Landroid/content/Intent;)V");
    jmethodID parse = env->GetStaticMethodID(uc, "parse", "(Ljava/lang/String;)Landroid/net/Uri;");
    jmethodID c2 = env->GetMethodID(ic, "<init>", "(Ljava/lang/String;Landroid/net/Uri;)V"), c1 = env->GetMethodID(ic, "<init>",
        "(Ljava/lang/String;)V");
    if (env->ExceptionCheck() || !gpn || !sa || !parse || !c2 || !c1) { env->ExceptionClear(); return; }
    jstring pkg = (jstring)env->CallObjectMethod(act, gpn); const char * pk = env->GetStringUTFChars(pkg, NULL);
        std::string us = std::string("package:") + pk; env->ReleaseStringUTFChars(pkg, pk);
    jobject uri = env->CallStaticObjectMethod(uc, parse, env->NewStringUTF(us.c_str()));
    jobject it = env->NewObject(ic, c2, env->NewStringUTF("android.settings.MANAGE_APP_ALL_FILES_ACCESS_PERMISSION"), uri);
    env->CallVoidMethod(act, sa, it);
    if (env->ExceptionCheck()) { env->ExceptionClear(); jobject it2 = env->NewObject(ic, c1,
        env->NewStringUTF("android.settings.MANAGE_ALL_FILES_ACCESS_PERMISSION")); env->CallVoidMethod(act, sa, it2);
        if (env->ExceptionCheck()) env->ExceptionClear(); }
    gh_log("FOLDER all-files-access settings opened (pkg %s)", us.c_str());
}
static bool fp_isdir(const std::string & p) { struct stat st; return stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode); }
static void fp_subs(const std::string & dir, std::vector<std::string> & subs) {   /* subfolders, sorted */
    subs.clear(); DIR * d = opendir(dir.c_str()); if (!d) return; struct dirent * e;
    while ((e = readdir(d))) { std::string n = e->d_name; if (n.empty() || n[0] == '.') continue;
        if (fp_isdir(dir + "/" + n)) subs.push_back(n); }
    closedir(d); std::sort(subs.begin(), subs.end());
}
/* B2: 이미지 확장자 — gh_pick_files 의 윈도 필터와 같은 목록 */
static bool fp_isimg(const std::string & n) {
    size_t p = n.rfind('.'); if (p == std::string::npos || p + 1 >= n.size()) return false;
    std::string e = n.substr(p + 1); for (auto & ch : e) ch = (char)tolower((unsigned char)ch);
    static const char * const X[] = { "fdi", "hdi", "nhd", "d88", "d77", "88d", "98d", "nfd", "hdm", "dup", "xdf", "dsk", "fim", "flp", "img", "ima", "thd", "hdd", "hdn", "dim", "tfd", "fdd", "bin", "zip", NULL };
    for (int i = 0; X[i]; i++) if (e == X[i]) return true;
    return false;
}
static void fp_files(const std::string & dir, std::vector<std::string> & fs) {   /* image files directly inside dir, sorted */
    fs.clear(); DIR * d = opendir(dir.c_str()); if (!d) return; struct dirent * e;
    while ((e = readdir(d))) { std::string n = e->d_name; if (n.empty() || n[0] == '.') continue;
        if (!fp_isdir(dir + "/" + n) && fp_isimg(n)) fs.push_back(n); }
    closedir(d); std::sort(fs.begin(), fs.end());
}
/* B2: 마지막에 연 폴더 기억 — 설정 파일 옆 lastdir.txt 한 줄 */
static std::string fp_lastpath(void) { const std::string & c = gApp.cfgpath; size_t p = c.find_last_of("/\\"); return (p == std::string::npos ? std::string(".") : c.substr(0, p)) + "/lastdir.txt"; }
static std::string fp_last_load(void) { std::string r; FILE * f = fopen(fp_lastpath().c_str(), "rb"); if (!f) return r; char b[1024]; if (fgets(b, sizeof b, f)) { r = b; while (!r.empty() && (r.back() == '\n' || r.back() == '\r')) r.pop_back(); } fclose(f); return (!r.empty() && fp_isdir(r)) ? r : std::string(); }
static void fp_last_save(const std::string & d) { if (d.empty()) return; FILE * f = fopen(fp_lastpath().c_str(), "wb"); if (!f) return; fputs(d.c_str(), f); fputs("\n", f); fclose(f); }
struct FpRow { std::string label, path; int kind; };   /* kind 0 use,1 up,2 open,3 cancel,4 perm,5 info, 6 file(toggle), 7 confirm(파일 모드) */
static void fp_roots(std::vector<FpRow> & rows) {
    rows.clear(); std::vector<std::string> bases, sds; bases.push_back("/storage/emulated/0");
    std::vector<std::string> st; fp_subs("/storage", st); for (auto & n : st) if (n != "emulated" && n != "self") {
        sds.push_back("/storage/" + n); bases.push_back("/storage/" + n); }
    for (auto & b : bases) { static const char * nm[] = { "Kyuhachi", "Download" }; for (int i = 0; i < 2; i++) {
        std::string p = b + "/" + nm[i]; if (fp_isdir(p)) { FpRow r = { std::string("[") + fpt(10) + "] " + p, p, 2 }; rows.push_back(r);
        } } }
    { FpRow r = { std::string(fpt(3)) + "  /storage/emulated/0", "/storage/emulated/0", 2 }; rows.push_back(r); }
    for (auto & s : sds) { FpRow r = { std::string(fpt(4)) + "  " + s, s, 2 }; rows.push_back(r); }
    { FpRow r = { fpt(11), "", 3 }; rows.push_back(r); }
}
static void fp_dir_rows(const std::string & cur, std::vector<FpRow> & rows, bool filemode, const std::set<std::string> & chosen) {
    rows.clear();
    if (filemode) { FpRow r = { std::string(fpt(16)) + " (" + std::to_string((int)chosen.size()) + ")", "", 7 }; rows.push_back(r); }   /* B2: 맨 위 = 선택 등록 */
    else { FpRow r = { fpt(1), cur, 0 }; rows.push_back(r); }
    { FpRow r = { fpt(2), "", 1 }; rows.push_back(r); }
    std::vector<std::string> subs; fp_subs(cur, subs); for (auto & n : subs) { FpRow r = { n, cur + "/" + n, 2 }; rows.push_back(r); }
    std::vector<std::string> fs; if (filemode) { fp_files(cur, fs);
        for (auto & n : fs) { const std::string full = cur + "/" + n; FpRow r = { std::string(chosen.count(full) ? "[x] " : "[ ] ") + n, full, 6 }; rows.push_back(r); } }
    if (subs.empty() && fs.empty()) { FpRow r = { filemode ? fpt(17) : fpt(9), "", 5 }; rows.push_back(r); }
}
static std::string fp_up(const std::string & cur) {   /* parent folder; up from a storage root goes to the first screen (empty string) */
    size_t p = cur.rfind('/'); if (p == std::string::npos || p == 0) return std::string(); std::string u = cur.substr(0, p);
    if (u == "/storage" || u == "/storage/emulated" || u.empty()) return std::string(); return u;
}
/* filemode=false: 폴더 하나를 out 에. filemode=true: 고른 파일들(전체 경로, 정렬)을 files 에. */
static bool fp_explore(const char * title_u8, bool filemode, std::string & out, std::vector<std::string> * files) {
    static uint32_t buf[640 * 400]; ov::Canvas c = { buf, 640, 400 }; ui::Ui & U = gApp.ui_;
    std::string cur; std::vector<FpRow> rows; int sel = 0; bool perm = gh_all_files_ok(); Uint32 pt = SDL_GetTicks();
    std::set<std::string> chosen;
    if (perm) cur = fp_last_load();   /* B2: 지난번 그 자리에서 시작(폴더가 없어졌으면 첫 화면) */
    auto rebuild = [&]() { if (!perm) { rows.clear(); FpRow r = { fpt(6), "", 4 }; rows.push_back(r); }
        else if (cur.empty()) fp_roots(rows); else fp_dir_rows(cur, rows, filemode, chosen); if (sel >= (int)rows.size()) sel = (int)rows.size() - 1;
        if (sel < 0) sel = 0; };
    auto toggle = [&](const FpRow & r) { if (chosen.count(r.path)) chosen.erase(r.path); else chosen.insert(r.path); rebuild(); };
    rebuild(); gh_log("FOLDER explorer open: perm=%d mode=%s start=%s title=%s", perm ? 1 : 0, filemode ? "file" : "dir", cur.c_str(), title_u8);
    for (;;) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) { GH_QUITSET(); return false; }
            if (e.type == SDL_FINGERDOWN || e.type == SDL_FINGERMOTION || e.type == SDL_FINGERUP) { gh_touch_feed(e); continue; }
            if (gh_wm_hotkey(e)) continue;
            ui::UKey k = gh_ev2uk(e, NULL); if (k == ui::UK_NONE) continue;
            pwr::mclk::key(k);   /* E1R: explorer click */
            if (k == ui::UK_UP) { if (sel > 0) sel--; }
            else if (k == ui::UK_DOWN) { if (sel + 1 < (int)rows.size()) sel++; }
            else if (k == ui::UK_CANCEL) { if (!perm || cur.empty()) return false; cur = fp_up(cur); sel = 0; rebuild(); fp_last_save(cur); }
            else if (k == ui::UK_TRI && filemode && !rows.empty() && rows[sel].kind == 6) toggle(rows[sel]);   /* B2: X/Y 도 토글 */
            else if (k == ui::UK_OK && !rows.empty()) {
                const FpRow r = rows[sel];
                if (r.kind == 4) gh_all_files_ask();
                else if (r.kind == 3) return false;
                else if (r.kind == 1) { cur = fp_up(cur); sel = 0; rebuild(); fp_last_save(cur); }
                else if (r.kind == 2) { cur = r.path; sel = 0; rebuild(); fp_last_save(cur); }
                else if (r.kind == 6) toggle(r);   /* B2: A/탭 = 고르기·해제 */
                else if (r.kind == 7) {   /* B2: 선택 등록 — 고른 게 없으면 아무 일 없음 */
                    if (chosen.empty()) continue;
                    if (files) { files->assign(chosen.begin(), chosen.end()); }
                    gh_log("FOLDER files picked: %d (first %s)", (int)chosen.size(), chosen.begin()->c_str());
                    c.rect(0, 0, 640, 400, ov::K); ov::text(c, 40, 180, fpt(20), ov::YL, ov::K); gh_publish(buf, 640, 400);
                    return true; }
                else if (r.kind == 0) {
                    out = r.path; gh_log("FOLDER picked: %s", out.c_str());
                    c.rect(0, 0, 640, 400, ov::K); ov::text(c, 40, 180, fpt(12), ov::YL, ov::K); gh_publish(buf, 640, 400);
                    return true; }
            }
        }
        if (!perm && SDL_GetTicks() - pt >= 500u) { pt = SDL_GetTicks(); if (gh_all_files_ok()) { perm = true; cur.clear(); sel = 0;
        rebuild(); gh_log("FOLDER all-files-access granted"); } }
        gh_wm_poll();
        U.hits.clear(); c.rect(0, 0, 640, 400, ov::K);
        ov::text(c, 40, 0, KH_DISPNAME, ov::WH, ov::K); /*E1R*/
        ov::rbox(c, 16, 24, 608, 300, ov::WH, filemode ? phr_("이미지 파일 고르기") : fpt(0));
        if (!perm) { ov::text(c, 40, 48, fpt(5), ov::YL, ov::K); ov::text(c, 40, 76, ov::fit(fpt(7), 560), ov::WH, ov::K); }
        else ov::text(c, 40, 48, ov::fit(cur.empty() ? std::string(fpt(13)) : cur, 560), ov::YL, ov::K);
        const int VIS = 9, PT = 24, Y0 = perm ? 76 : 120; int top = sel - 4;
        if (top > (int)rows.size() - VIS) top = (int)rows.size() - VIS; if (top < 0) top = 0;
        for (int i = 0; i < VIS && top + i < (int)rows.size(); i++) {
            int idx = top + i, yy = Y0 + i * PT; bool sl = idx == sel, dim = rows[idx].kind == 5;
            const bool on = rows[idx].kind == 6 && chosen.count(rows[idx].path);   /* B2: 고른 파일은 노랑 */
            if (sl) c.rect(34, yy - 3, 572, 22, dim ? ov::GY : ov::WH);
            c.clip(40, yy - 3, 560, 22); ov::text(c, 40, yy, rows[idx].label, sl ? ov::K : (dim ? ov::GY : (on ? ov::YL : ov::CY)),
        sl ? (dim ? ov::GY : ov::WH) : ov::K); c.unclip();
            ui::Ui::Hit h = { 34, yy - 3, 572, 22, idx, sel }; U.hits.push_back(h);
        }
        if (top > 0) ov::text(c, 600, 52, "↑", ov::CY, ov::K); if (top + VIS < (int)rows.size()) ov::text(c, 600, 296, "↓", ov::CY,
        ov::K);
        ov::text(c, 16, c.h - 32, ov::fit(filemode ? fpt(18) : fpt(8), 608), ov::YL, ov::K);
        gh_publish(buf, 640, 400); SDL_Delay(16);
    }
}
static bool gh_folder_explorer(const char * title_u8, std::string & out) { return fp_explore(title_u8, false, out, NULL); }
static bool gh_file_explorer(const char * title_u8, std::vector<std::string> & files) { std::string dummy; return fp_explore(title_u8, true, dummy, &files); }
#endif
