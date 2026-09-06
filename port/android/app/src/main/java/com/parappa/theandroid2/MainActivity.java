package com.parappa.theandroid2;

import android.app.Activity;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class MainActivity extends Activity {
    private static final int REQ_PICK = 1001;

    private TextView log;
    private String copiedPath; // local cache of picked file

    static {
        try {
            System.loadLibrary("parappa2");
        } catch (UnsatisfiedLinkError e) {
            // native may fail on some emulators; UI still works
        }
    }

    public native String nativeInfo();
    public native String verifyPath(String path);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        log = findViewById(R.id.txtLog);
        Button btnPick = findViewById(R.id.btnPick);
        Button btnVerify = findViewById(R.id.btnVerify);
        Button btnInfo = findViewById(R.id.btnNativeInfo);

        btnPick.setOnClickListener(v -> {
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType("*/*");
            startActivityForResult(i, REQ_PICK);
        });

        btnVerify.setOnClickListener(v -> {
            if (copiedPath == null) {
                toast("Primero elegi un archivo");
                return;
            }
            append("\nVerificando: " + copiedPath + "\n");
            try {
                String r = verifyPath(copiedPath);
                append(r + "\n");
            } catch (UnsatisfiedLinkError e) {
                // fallback pure Java size check
                File f = new File(copiedPath);
                long size = f.length();
                long expected = 4159078400L;
                append("size=" + size + " expected=" + expected + "\n");
                append(size == expected ? "SIZE OK (July 12)\n" : "SIZE MISMATCH\n");
            }
        });

        btnInfo.setOnClickListener(v -> {
            try {
                append(nativeInfo() + "\n");
            } catch (UnsatisfiedLinkError e) {
                append("Native lib no cargada: " + e.getMessage() + "\n");
            }
        });

        append("Harness Android.\nPodes dejar el .bin en el celu (mucho espacio).\n");
        append("No hace falta copiarlo a la PC.\n\n");
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQ_PICK || resultCode != RESULT_OK || data == null) return;
        Uri uri = data.getData();
        if (uri == null) return;

        String name = queryName(uri);
        append("Picked: " + name + "\n");
        append("(copiando a cache interna para NDK... puede tardar si es 3.8GB)\n");

        // NOTE: full 3.8GB copy is heavy. For size-only check we stream length via ParcelFileDescriptor if possible.
        try {
            // Prefer not to full-copy huge files: store URI string and only copy if needed.
            // For native path-based APIs we still need a real path -> copy to cache.
            File out = new File(getCacheDir(), name != null ? name : "picked.bin");
            long expected = 4159078400L;

            // Fast path: if we can get size from cursor and it matches, note it without full copy for now
            long reported = querySize(uri);
            if (reported > 0) {
                append("Reported size: " + reported + " bytes\n");
                if (reported == expected) {
                    append("SIZE matches July 12 prototype!\n");
                } else {
                    append("SIZE does not match expected " + expected + "\n");
                }
            }

            // Only copy if smaller than 200MB (IRX/OLM tests). Skip full ISO copy on phone by default.
            if (reported > 0 && reported < 200L * 1024 * 1024) {
                try (InputStream in = getContentResolver().openInputStream(uri);
                     FileOutputStream fos = new FileOutputStream(out)) {
                    byte[] buf = new byte[1024 * 1024];
                    int n;
                    while ((n = in.read(buf)) > 0) fos.write(buf, 0, n);
                }
                copiedPath = out.getAbsolutePath();
                append("Cached at: " + copiedPath + "\n");
            } else {
                copiedPath = null;
                append("Archivo grande: no se copia entero al cache (ahorra espacio).\n");
                append("Size check ya hecho arriba. Extract/prlib en PC o con copy selectivo despues.\n");
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

    private void toast(String s) {
        Toast.makeText(this, s, Toast.LENGTH_SHORT).show();
    }
}
