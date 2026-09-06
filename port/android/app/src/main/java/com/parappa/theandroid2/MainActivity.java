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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        log = findViewById(R.id.txtLog);
        Button btnPick = findViewById(R.id.btnPick);
        Button btnVerify = findViewById(R.id.btnVerify);
        Button btnInfo = findViewById(R.id.btnNativeInfo);
        Button btnGs = findViewById(R.id.btnGsVk);

        btnPick.setOnClickListener(v -> {
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType("*/*");
            startActivityForResult(i, REQ_PICK);
        });

        btnVerify.setOnClickListener(v -> {
            if (copiedPath == null) {
                toast("Primero elegi un archivo chico, o confia en el size check del pick");
                return;
            }
            append("\nVerificando: " + copiedPath + "\n");
            try {
                append(verifyPath(copiedPath) + "\n");
            } catch (UnsatisfiedLinkError e) {
                File f = new File(copiedPath);
                long size = f.length();
                long expected = 4159078400L;
                append("size=" + size + " expected=" + expected + "\n");
            }
        });

        btnInfo.setOnClickListener(v -> {
            try {
                append(nativeInfo() + "\n");
            } catch (UnsatisfiedLinkError e) {
                append("Native lib no cargada: " + e.getMessage() + "\n");
            }
        });

        btnGs.setOnClickListener(v -> {
            append("\n=== GS -> Vulkan smoke ===\n");
            try {
                append(gsVulkanSmoke() + "\n");
            } catch (UnsatisfiedLinkError e) {
                append("FAIL native: " + e.getMessage() + "\n");
            }
        });

        append("Path B + traduccion GS->Vulkan.\n");
        append("El .bin vive en el celu. Boton 4 prueba el pipeline de traduccion.\n\n");
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
            File out = new File(getCacheDir(), name != null ? name : "picked.bin");
            long expected = 4159078400L;
            long reported = querySize(uri);
            if (reported > 0) {
                append("Reported size: " + reported + " bytes\n");
                append(reported == expected ? "SIZE matches July 12 prototype!\n" : "SIZE mismatch\n");
            }
            if (reported > 0 && reported < 200L * 1024 * 1024) {
                try (InputStream in = getContentResolver().openInputStream(uri);
                     FileOutputStream fos = new FileOutputStream(out)) {
                    byte[] buf = new byte[1024 * 1024];
                    int n;
                    while ((n = in.read(buf)) > 0) fos.write(buf, 0, n);
                }
                copiedPath = out.getAbsolutePath();
                append("Cached: " + copiedPath + "\n");
            } else {
                copiedPath = null;
                append("Archivo grande: no se copia (ahorra espacio en celu).\n");
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

    private void append(String s) { log.append(s); }
    private void toast(String s) { Toast.makeText(this, s, Toast.LENGTH_SHORT).show(); }
}
