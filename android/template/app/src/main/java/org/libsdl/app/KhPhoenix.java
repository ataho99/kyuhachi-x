package org.libsdl.app;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Process;

// t147d: 별 프로세스(:khphoenix) 에서 돌며 본 프로세스를 끝내고 앱을 새 프로세스로 다시 시작한다.
public class KhPhoenix extends Activity {
    @Override
    protected void onCreate(Bundle b) {
        super.onCreate(b);
        try {
            int pid = getIntent().getIntExtra("pid", -1);
            if (pid > 0) Process.killProcess(pid);
            Intent m = getPackageManager().getLaunchIntentForPackage(getPackageName());
            if (m != null) { m.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK); startActivity(m); }
        } catch (Exception e) { }
        finish();
        Runtime.getRuntime().exit(0);
    }
}
