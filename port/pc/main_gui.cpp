// TestForIssues GUI - Win32 (no external deps)
// July 12 2001 NTSC-J Prototype checker

#include "../common/rom_check.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <thread>
#include <atomic>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

static HWND g_hwnd = nullptr;
static HWND g_edit = nullptr;
static HWND g_btn = nullptr;
static HWND g_status = nullptr;
static std::atomic<bool> g_busy{false};

static void append_text(const std::string& text) {
    if (!g_edit) return;
    int len = GetWindowTextLengthA(g_edit);
    SendMessageA(g_edit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageA(g_edit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
}

static void set_status(const char* msg) {
    if (g_status) SetWindowTextA(g_status, msg);
}

static void enable_btn(bool on) {
    if (g_btn) EnableWindow(g_btn, on ? TRUE : FALSE);
}

static std::string open_file_dialog(HWND owner) {
    char file[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "PS2 Dump (*.bin;*.iso)\0*.bin;*.iso\0BIN (*.bin)\0*.bin\0ISO (*.iso)\0*.iso\0All (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    ofn.lpstrTitle = "Seleccionar PS2 - Parappa 7-12-07.bin";
    if (GetOpenFileNameA(&ofn)) return std::string(file);
    return {};
}

static void run_check(std::string path) {
    g_busy = true;
    enable_btn(false);
    set_status("Verificando...");

    // clear log
    SetWindowTextA(g_edit, "");
    append_text("========================================\r\n");
    append_text("  PaRappa 2 - TestForIssues (GUI)\r\n");
    append_text("  Solo July 12 2001 NTSC-J Prototype\r\n");
    append_text("========================================\r\n\r\n");
    append_text(("Archivo: " + path + "\r\n\r\n").c_str());

    append_text("[1/2] Check rapido (tamano)...\r\n");
    auto fast = RomCheck::verify_iso_fast(path);
    if (fast.result != RomCheck::Result::OK) {
        append_text(("FAIL: " + fast.message + "\r\n").c_str());
        set_status("ROM incorrecta");
        enable_btn(true);
        g_busy = false;
        MessageBoxA(g_hwnd, fast.message.c_str(), "ROM incorrecta", MB_OK | MB_ICONWARNING);
        return;
    }
    append_text(("OK: " + fast.message + "\r\n\r\n").c_str());

    append_text("[2/2] Calculando SHA1 completo (~3.87 GB)...\r\n");
    append_text("Esto puede tardar 20-90 segundos, no cierres la ventana.\r\n\r\n");
    set_status("Calculando SHA1... espera");

    auto full = RomCheck::verify_iso(path);

    if (full.result == RomCheck::Result::OK) {
        append_text(("\r\nSUCCESS: " + full.message + "\r\n").c_str());
        append_text(("SHA1: " + full.detected_sha1 + "\r\n").c_str());
        append_text("\r\nROM correcta. Listo para el port.\r\n");
        set_status("ROM OK - July 12 Prototype");
        MessageBoxA(g_hwnd,
            "ROM correcta!\n\nJuly 12 2001 NTSC-J Prototype detectada.",
            "TestForIssues",
            MB_OK | MB_ICONINFORMATION);
    } else {
        append_text(("\r\nFAIL: " + full.message + "\r\n").c_str());
        set_status("ROM incorrecta");
        MessageBoxA(g_hwnd, full.message.c_str(), "ROM incorrecta", MB_OK | MB_ICONWARNING);
    }

    enable_btn(true);
    g_busy = false;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowA("STATIC", "PaRappa the Rapper 2 - TestForIssues",
            WS_CHILD | WS_VISIBLE,
            10, 10, 560, 20, hwnd, nullptr, nullptr, nullptr);

        g_btn = CreateWindowA("BUTTON", "Seleccionar .bin / .iso",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 40, 200, 32, hwnd, (HMENU)1, nullptr, nullptr);

        g_status = CreateWindowA("STATIC", "Esperando archivo...",
            WS_CHILD | WS_VISIBLE,
            220, 46, 350, 20, hwnd, nullptr, nullptr, nullptr);

        g_edit = CreateWindowA("EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 85, 560, 350, hwnd, nullptr, nullptr, nullptr);

        // font mono-ish
        HFONT font = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
        if (font) SendMessageA(g_edit, WM_SETFONT, (WPARAM)font, TRUE);

        append_text("Apreta el boton y elegi:\r\n");
        append_text("  PS2 - Parappa 7-12-07.bin\r\n\r\n");
        append_text("Tamano esperado: 4159078400 bytes\r\n");
        append_text("SHA1 esperado:   28964c33cee578ec3ce476285067044243363d08\r\n");
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1 && !g_busy) {
            std::string path = open_file_dialog(hwnd);
            if (!path.empty()) {
                // run check on background thread so UI no se congela
                std::thread([path]() { run_check(path); }).detach();
            }
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "TestForIssuesGUI";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wc);

    g_hwnd = CreateWindowA("TestForIssuesGUI",
        "TestForIssues - PaRappa 2 (July 12 Prototype)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 500,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
