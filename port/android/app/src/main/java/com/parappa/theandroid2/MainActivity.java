package com.parappa.theandroid2;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class MainActivity extends Activity {
    private static final int REQ_PICK = 1001;

    private TextView log;
    private TextView screen;
    private ScrollView scrollLog;
    private String copiedPath;

    static {
        try {
            System.loadLibrary("parappa2");
        } catch (UnsatisfiedLinkError e) {
        }
    }

    public native String nativeInfo();
    public native String verifyPath(String path);
    public native String gsVulkanSmoke();
    public native String runEmuDemo(int frames);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        log = findViewById(R.id.txtLog);
        screen = findViewById(R.id.txtScreen);
        scrollLog = findViewById(R.id.scrollLog);

        Button btnEmu = findViewById(R.id.btnEmu);
        Button btnGs = findViewById(R.id.btnGsVk);
        Button btnPick = findViewById(R.id.btnPick);
        Button btnInfo = findViewById(R.id.btnNativeInfo);

        btnEmu.setOnClickListener(v -> {
            append("\n======== EMU SESSION ========\n");
            screen.setText("[GS framebuffer]\nrunning demo frames...");
            try {
                String out = runEmuDemo(5);
                append(out);
                screen.setText("[GS framebuffer]\n640x448 PSMCT32\n5 frames submitted\n(NullDevice — log only)");
            } catch (UnsatisfiedLinkError e) {
                append("FAIL native: " + e.getMessage() + "\n");
                screen.setText("native FAIL");
            }
            scrollLog.post(() -> scrollLog.fullScroll(ScrollView.FOCUS_DOWN));
        });

        btnGs.setOnClickListener(v -> {
            append("\n=== GS -> Vulkan smoke ===\n");
            try {
                append(gsVulkanSmoke() + "\n");
            } catch (UnsatisfiedLinkError e) {
                append("FAIL: " + e.getMessage() + "\n");
            }
            scrollLog.post(() -> scrollLog.fullScroll(ScrollView.FOCUS_DOWN));
        });

        btnPick.setOnClickListener(v -> {
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType("*/*");
            startActivityForResult(i, REQ_PICK);
        });

        btnInfo.setOnClickListener(v -> {
            try {
                append(nativeInfo() + "\n");
            } catch (UnsatisfiedLinkError e) {
                append("Native no cargada\n");
            }
        });

        append("UI dual: pantalla GS arriba + log del juego abajo.\n");
        append("▶ Emu = boot EE/IOP/prlib + frames GS→VK (log en vivo).\n\n");
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQ_PICK || resultCode != RESULT_OK || data == null) return;
        Uri uri = data.getData();
        if (uri == null) return;

        String name = queryName(uri);
        append("Picked: " + name + "\n");
        try {
            long expected = 4159078400L;
            long reported = querySize(uri);
            if (reported > 0) {
                append("size=" + reported + (reported == expected ? " OK July12\n" : " mismatch\n"));
            }
            if (reported > 0 && reported < 200L * 1024 * 1024) {
                File out = new File(getCacheDir(), name != null ? name : "picked.bin");
                try (InputStream in = getContentResolver().openInputStream(uri);
                     FileOutputStream fos = new FileOutputStream(out)) {
                    byte[] buf = new byte[1024 * 1024];
                    int n;
                    while ((n = in.read(buf)) > 0) fos.write(buf, 0, n);
                }
                copiedPath = out.getAbsolutePath();
                append("cached " + copiedPath + "\n");
            } else {
                copiedPath = null;
                append("bin grande: no se copia al cache\n");
            }
        } catch (Exception e) {
            append("Error: " + e.getMessage() + "\n");
        }
    }

    private String queryName(Uri uri) {
        try (Cursor c = getContentResolver().query(uri, null, null, null, null)) {
            if (c != null && c.moveToFirst()) {
                int idx = c.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (idx >= 0) return c.getString(idx);
            }
        } catch (Exception ignored) {}
        return "file.bin";
    }

    private long querySize(Uri uri) {
        try (Cursor c = getContentResolver().query(uri, null, null, null, null)) {
            if (c != null && c.moveToFirst()) {
                int idx = c.getColumnIndex(OpenableColumns.SIZE);
                if (idx >= 0) return c.getLong(idx);
            }
        } catch (Exception ignored) {}
        return -1;
    }

    private void append(String s) {
        log.append(s);
    }
}
