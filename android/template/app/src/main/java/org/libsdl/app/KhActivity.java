package org.libsdl.app;

import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.system.Os;
import android.util.Log;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

// t16: SDLActivity subclass wiring internal-storage config location.
// (a) HOME/XDG_CONFIG_HOME/XDG_DATA_HOME -> getFilesDir() before super.onCreate
// (b) copy assets/dosbox-x.conf to <files>/dosbox-x/dosbox-x.conf (+dosbox.conf) only if absent
// (c) super.onCreate
public class KhActivity extends SDLActivity {
    private static final String TAG = "KhActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        try {
            khFeIntent_(getIntent());   // E4 (manual): cold start via VIEW
            String files = getFilesDir().getAbsolutePath();
            try { Os.setenv("HOME", files, true); } catch (Exception e) { Log.v(TAG, "setenv HOME fail " + e); }
            try { Os.setenv("XDG_CONFIG_HOME", files, true); } catch (Exception e) { Log.v(TAG, "setenv XDG_CONFIG_HOME fail " + e); }
            try { Os.setenv("XDG_DATA_HOME", files + "/share", true); } catch (Exception e) { Log.v(TAG, "setenv XDG_DATA_HOME fail " + e); }
            File cfgDir = new File(files + "/dosbox-x");
            if (!cfgDir.isDirectory()) cfgDir.mkdirs();
            File shareDir = new File(files + "/share");
            if (!shareDir.isDirectory()) shareDir.mkdirs();
            String[] targets = new String[] {
                cfgDir.getAbsolutePath() + "/dosbox-x.conf",
                cfgDir.getAbsolutePath() + "/dosbox.conf",
                files + "/dosbox-x.conf"
            };
            for (String t : targets) {
                File f = new File(t);
                if (f.isFile()) continue;
                InputStream in = null;
                OutputStream out = null;
                try {
                    in = getAssets().open("dosbox-x.conf");
                    out = new FileOutputStream(f);
                    byte[] buf = new byte[8192];
                    int n;
                    while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
                } catch (Exception e) {
                    Log.v(TAG, "asset copy skip " + t + " " + e);
                } finally {
                    try { if (in != null) in.close(); } catch (Exception ignored) {}
                    try { if (out != null) out.close(); } catch (Exception ignored) {}
                }
                break; // one successful/attempted copy seeds all? no: copy same bytes to remaining missing targets below
            }
            // ensure remaining missing targets get same content from first existing target
            File seed = null;
            for (String t : targets) { File f = new File(t); if (f.isFile()) { seed = f; break; } }
            if (seed != null) {
                for (String t : targets) {
                    File f = new File(t);
                    if (f.isFile()) continue;
                    InputStream in = null;
                    OutputStream out = null;
                    try {
                        in = new java.io.FileInputStream(seed);
                        out = new FileOutputStream(f);
                        byte[] buf = new byte[8192];
                        int n;
                        while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
                    } catch (Exception e) { Log.v(TAG, "seed copy skip " + t + " " + e); }
                    finally {
                        try { if (in != null) in.close(); } catch (Exception ignored) {}
                        try { if (out != null) out.close(); } catch (Exception ignored) {}
                    }
                }
            }
        } catch (Exception e) {
            Log.v(TAG, "KhActivity pre-super fail " + e);
        }
        super.onCreate(savedInstanceState);
        sFeTaken = true;   // E4 (manual): later VIEW calls go to onNewIntent
    }

    // t55c: hide status+navigation bars (they overlapped the top title row and bottom help line)
    private void hideBars_() {
        try {
            if (Build.VERSION.SDK_INT >= 30) {
                getWindow().setDecorFitsSystemWindows(false);   // t147b: 바 상태와 무관하게 내용 영역을 화면 전체로
                WindowInsetsController c = getWindow().getInsetsController();
                if (c != null) { c.hide(WindowInsets.Type.systemBars()); c.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE); }
            } else {
                getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
            }
        } catch (Exception e) { Log.v(TAG, "hideBars fail " + e); }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) { hideBars_(); kh_posthide_(); }
    }
    @Override
    protected void onResume() { super.onResume(); hideBars_(); kh_posthide_(); }
    @Override
    protected void onNewIntent(android.content.Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        khFeIntent_(intent);   // E4 (manual): running -> ignored + log
    }
    // t147b: 홈 복귀 직후 시스템 바가 잠깐 보였다 사라질 때 표면이 어긋나는 문제 -- 복귀 뒤 0.15/0.6/1.5초에 다시 숨기고 레이아웃을 다시 요청
    private void kh_posthide_() {
        try {
            final android.view.View d = getWindow().getDecorView();
            Runnable r = new Runnable() { public void run() { hideBars_(); d.requestLayout(); } };
            d.postDelayed(r, 150); d.postDelayed(r, 600); d.postDelayed(r, 1500);
        } catch (Exception e) { Log.v(TAG, "posthide fail " + e); }
    }

    // E4 (manual): ES-DE/Daijisho VIEW intent -> next_args.txt "--gamefile <path>".
    private static boolean sFeTaken = false;   // native up once onCreate done
    private void khFeIntent_(android.content.Intent it) {
        if (it == null) return;
        if (!android.content.Intent.ACTION_VIEW.equals(it.getAction())) return;
        if (sFeTaken) { android.util.Log.i(TAG, "FE intent ignored (already running)"); return; }
        android.net.Uri d = it.getData();
        if (d == null) { android.util.Log.i(TAG, "FE intent no data"); return; }
        String path = null;
        String sch = d.getScheme();
        if (sch == null) { String s = d.toString(); if (s != null && s.startsWith("/")) path = s; }
        else if (sch.equals("file")) path = d.getPath();
        else if (sch.equals("content")) path = khFeCopy_(d);
        if (path == null || path.isEmpty() || path.indexOf('"') >= 0) {
            android.util.Log.i(TAG, "FE intent unusable: " + d); return; }
        try {
            java.io.FileWriter w = new java.io.FileWriter(new java.io.File(getFilesDir(), "next_args.txt"), false);
            w.write("\"kyuhachi\" \"--gamefile\" \"" + path + "\"\n");   // p688: kha_take_nextargs skips token 0
            w.close();
            android.util.Log.i(TAG, "FE intent -> --gamefile " + path);
        } catch (Exception e) { android.util.Log.v(TAG, "FE intent write fail " + e); }
    }
    private String khFeCopy_(android.net.Uri d) {   // SAF -> getCacheDir()/fe_inbox, newest 3 kept
        try {
            String nm = "fe_" + System.currentTimeMillis();
            try {
                android.database.Cursor c = getContentResolver().query(d, null, null, null, null);
                if (c != null) {
                    int ix = c.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME);
                    if (c.moveToFirst() && ix >= 0) { String dn = c.getString(ix); if (dn != null && !dn.isEmpty()) nm = dn; }
                    c.close(); } } catch (Exception e) { android.util.Log.v(TAG, "FE name fail " + e); }
            nm = nm.replaceAll("[/\\\\]", "_");
            java.io.File dir = new java.io.File(getCacheDir(), "fe_inbox");
            dir.mkdirs();
            java.io.File[] olds = dir.listFiles();
            if (olds != null && olds.length >= 3) {
                java.util.Arrays.sort(olds, new java.util.Comparator<java.io.File>() {
                    public int compare(java.io.File a, java.io.File b) { return Long.compare(a.lastModified(), b.lastModified()); } });
                for (int i = 0; i + 3 <= olds.length; i++) olds[i].delete(); }
            java.io.File out = new java.io.File(dir, nm);
            java.io.InputStream in = getContentResolver().openInputStream(d);
            java.io.OutputStream os = new java.io.FileOutputStream(out);
            byte[] buf = new byte[8192]; int n;
            while ((n = in.read(buf)) > 0) os.write(buf, 0, n);
            in.close(); os.close();
            return out.getAbsolutePath();
        } catch (Exception e) { android.util.Log.v(TAG, "FE copy fail " + e); return null; }
    }

    // t145: 프로세스 재시작 -- 별 프로세스 KhPhoenix 가 이 프로세스를 끝내고 앱을 다시 띄운다(next_args.txt 의 인자로 시작).
    public void khRestart() {
        try {
            Intent i = new Intent(this, KhPhoenix.class);
            i.putExtra("pid", android.os.Process.myPid());
            i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(i);
        } catch (Exception e) { Log.v(TAG, "khRestart fail " + e); }
    }
}
