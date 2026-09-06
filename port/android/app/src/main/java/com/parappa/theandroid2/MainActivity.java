package com.parappa.theandroid2;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.OpenableColumns;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.ScrollView;
import android.widget.TextView;

public class MainActivity extends Activity {
    private static final int REQ_PICK = 1001;

    private TextView log;
    private ScrollView scrollLog;
    private ImageView imgScreen;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private boolean playing = false;
    private int frame = 0;
    private Bitmap fbBitmap;

    static {
        try {
            System.loadLibrary("parappa2");
        } catch (UnsatisfiedLinkError ignored) {
        }
    }

    public native String nativeInfo();
    public native String verifyPath(String path);
    public native String gsVulkanSmoke();
    public native String runEmuDemo(int frames);
    public native boolean emuBoot();
    public native String emuStep(int frame);
    public native int[] emuFramebuffer();
    public native int emuWidth();
    public native int emuHeight();

    private final Runnable tick = new Runnable() {
        @Override
        public void run() {
            if (!playing) return;
            try {
                String line = emuStep(frame);
                append(line + "\n");
                blitFramebuffer();
                frame++;
                if (frame > 300) frame = 0; // loop animation
            } catch (UnsatisfiedLinkError e) {
                append("native error: " + e.getMessage() + "\n");
                playing = false;
                return;
            }
            scrollLog.post(() -> scrollLog.fullScroll(ScrollView.FOCUS_DOWN));
            handler.postDelayed(this, 50); // ~20 fps soft
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        log = findViewById(R.id.txtLog);
        scrollLog = findViewById(R.id.scrollLog);
        imgScreen = findViewById(R.id.imgScreen);

        Button btnEmu = findViewById(R.id.btnEmu);
        Button btnStop = findViewById(R.id.btnStop);
        Button btnPick = findViewById(R.id.btnPick);
        Button btnInfo = findViewById(R.id.btnNativeInfo);

        btnEmu.setOnClickListener(v -> startEmu());
        btnStop.setOnClickListener(v -> stopEmu());

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

        append("SoftDevice raster: vas a VER sprites en movimiento.\n");
        append("▶ Play emu — pantalla + log.\n\n");
    }

    private void startEmu() {
        stopEmu();
        append("\n======== EMU PLAY ========\n");
        try {
            if (!emuBoot()) {
                append("BOOT FAIL\n");
                return;
            }
            append("[EMU] boot OK — animating\n");
            frame = 0;
            playing = true;
            handler.post(tick);
        } catch (UnsatisfiedLinkError e) {
            append("FAIL: " + e.getMessage() + "\n");
        }
    }

    private void stopEmu() {
        playing = false;
        handler.removeCallbacks(tick);
        append("[EMU] stopped\n");
    }

    private void blitFramebuffer() {
        int[] pixels = emuFramebuffer();
        int w = emuWidth();
        int h = emuHeight();
        if (pixels == null || w <= 0 || h <= 0) return;
        if (fbBitmap == null || fbBitmap.getWidth() != w || fbBitmap.getHeight() != h) {
            fbBitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        }
        fbBitmap.setPixels(pixels, 0, w, 0, 0, w, h);
        imgScreen.setImageBitmap(fbBitmap);
    }

    @Override
    protected void onDestroy() {
        stopEmu();
        super.onDestroy();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQ_PICK || resultCode != RESULT_OK || data == null) return;
        Uri uri = data.getData();
        if (uri == null) return;
        String name = "file.bin";
        long size = -1;
        try (Cursor c = getContentResolver().query(uri, null, null, null, null)) {
            if (c != null && c.moveToFirst()) {
                int ni = c.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                int si = c.getColumnIndex(OpenableColumns.SIZE);
                if (ni >= 0) name = c.getString(ni);
                if (si >= 0) size = c.getLong(si);
            }
        } catch (Exception ignored) {}
        append("Picked: " + name + " size=" + size + "\n");
        if (size == 4159078400L) append("SIZE OK July 12\n");
    }

    private void append(String s) {
        log.append(s);
    }
}
