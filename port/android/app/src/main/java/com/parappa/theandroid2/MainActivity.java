package com.parappa.theandroid2;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.OpenableColumns;
import android.widget.ImageView;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {
    private static final int REQ_PICK = 1001;
    private static final long JULY12_SIZE = 4159078400L;

    private TextView log;
    private ScrollView scrollLog;
    private ImageView imgScreen;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private boolean playing = false;
    private int frame = 0;
    private Bitmap fbBitmap;

    private boolean romLoaded = false;
    private String romName = null;
    private long romSize = -1;

    static {
        try {
            System.loadLibrary("parappa2");
        } catch (UnsatisfiedLinkError ignored) {
        }
    }

    public native String nativeInfo();
    public native void setRomPresent(boolean present, String name);
    public native String verifyPath(String path);
    public native String gsVulkanSmoke();
    public native String runEmuDemo(int frames);
    public native String emuBoot();
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
                if (line != null && !line.isEmpty()) append(line + "\n");
                blitFramebuffer();
                frame++;
                if (frame > 300) frame = 0;
            } catch (UnsatisfiedLinkError e) {
                append("native error: " + e.getMessage() + "\n");
                playing = false;
                return;
            }
            if (frame % 10 == 0) {
                scrollLog.post(() -> scrollLog.fullScroll(ScrollView.FOCUS_DOWN));
            }
            handler.postDelayed(this, 50);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        log = findViewById(R.id.txtLog);
        scrollLog = findViewById(R.id.scrollLog);
        imgScreen = findViewById(R.id.imgScreen);

        findViewById(R.id.btnEmu).setOnClickListener(v -> startEmu());
        findViewById(R.id.btnStop).setOnClickListener(v -> stopEmu());

        findViewById(R.id.btnPick).setOnClickListener(v -> {
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType("*/*");
            startActivityForResult(i, REQ_PICK);
        });

        findViewById(R.id.btnNativeInfo).setOnClickListener(v -> {
            try {
                append(nativeInfo() + "\n");
                append(romStatusLine() + "\n");
            } catch (UnsatisfiedLinkError e) {
                append("Native no cargada\n");
            }
        });

        try {
            setRomPresent(false, null);
        } catch (UnsatisfiedLinkError ignored) {
        }

        append("1) Pick .bin  (July 12 prototype)\n");
        append("2) Play emu\n\n");
        append(romStatusLine() + "\n\n");
    }

    private String romStatusLine() {
        if (romLoaded) {
            return "[ROM] OK — " + romName + " (" + romSize + " bytes) July 12";
        }
        return "[ROM] WARNING — no .BIN loaded (harness only / no real assets)";
    }

    private void startEmu() {
        stopEmu();
        append("\n======== EMU PLAY ========\n");
        append(romStatusLine() + "\n");

        if (!romLoaded) {
            append("[WARN] Sin .BIN correcto: solo demo procedural.\n");
            append("[WARN] Importa PS2 - Parappa 7-12-07.bin para modo ROM.\n");
            Toast.makeText(this,
                    "WARNING: No July 12 .BIN\nDemo only",
                    Toast.LENGTH_LONG).show();
            new AlertDialog.Builder(this)
                    .setTitle("ROM missing")
                    .setMessage(
                            "No July 12 prototype .BIN detected.\n\n" +
                            "Expected: PS2 - Parappa 7-12-07.bin\n" +
                            "Size: 4159078400 bytes\n\n" +
                            "Emu runs in DEMO mode.\n" +
                            "Use Pick .bin to import the ROM.")
                    .setPositiveButton("Play demo", (d, w) -> doBootAndPlay())
                    .setNegativeButton("Cancel", null)
                    .show();
            return;
        }

        append("[ROM] Using imported dump — July 12.\n");
        doBootAndPlay();
    }

    private void doBootAndPlay() {
        try {
            setRomPresent(romLoaded, romName);
            String boot = emuBoot();
            append(boot != null ? boot : "boot null\n");
            if (boot != null && boot.contains("FAIL")) return;
            frame = 0;
            playing = true;
            blitFramebuffer();
            handler.post(tick);
        } catch (UnsatisfiedLinkError e) {
            append("FAIL: " + e.getMessage() + "\n");
        }
    }

    private void stopEmu() {
        playing = false;
        handler.removeCallbacks(tick);
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

        append("Picked: " + name + "\n");
        append("size=" + size + "\n");

        romName = name;
        romSize = size;

        if (size == JULY12_SIZE) {
            romLoaded = true;
            try { setRomPresent(true, name); } catch (UnsatisfiedLinkError ignored) {}
            append("[ROM] OK — July 12 prototype size match\n");
            append("[ROM] Listo para Play emu\n");
            Toast.makeText(this, "ROM OK — July 12", Toast.LENGTH_SHORT).show();
        } else if (size > 0) {
            romLoaded = false;
            try { setRomPresent(false, name); } catch (UnsatisfiedLinkError ignored) {}
            append("[ROM] WARNING — size mismatch\n");
            append("[ROM] expected " + JULY12_SIZE + "\n");
            append("[ROM] got      " + size + "\n");
            Toast.makeText(this, "WARNING: Not July 12 .BIN", Toast.LENGTH_LONG).show();
        } else {
            romLoaded = false;
            try { setRomPresent(false, null); } catch (UnsatisfiedLinkError ignored) {}
            append("[ROM] WARNING — could not read size\n");
            Toast.makeText(this, "WARNING: cannot verify ROM", Toast.LENGTH_LONG).show();
        }
    }

    private void append(String s) {
        log.append(s);
    }
}
