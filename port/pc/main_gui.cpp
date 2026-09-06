// TestForIssues GUI — shows PS2->PC translation map + open BIN
#include "host_translate.h"
#include "rom_check.h"
#include "gs_translate.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <sstream>

#pragma comment(lib, "comdlg32.lib")

static HWND g_edit, g_hwnd;

static void append(const std::string& s) {
    if (!g_edit) return;
    int len = GetWindowTextLengthA(g_edit);
    SendMessageA(g_edit, EM_SETSEL, len, len);
    SendMessageA(g_edit, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
}

static std::string pick_bin(HWND owner) {
    char file[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "PS2 Dump (*.bin;*.iso)\0*.bin;*.iso\0All\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    ofn.lpstrTitle = "July 12 prototype .bin";
    if (GetOpenFileNameA(&ofn)) return file;
    return {};
}

static void run_host_smoke(const std::string& bin_path) {
    SetWindowTextA(g_edit, "");
    append(Host::translation_map_text());
    append("\r\n");

    Host::Context host;
    if (!host.init_all()) {
        append("FAIL init_all\r\n");
        return;
    }
    append(host.summary());
    append("\r\n");

    if (!bin_path.empty()) {
        auto fast = RomCheck::verify_iso_fast(bin_path);
        append("[ROM] " + fast.message + "\r\n");
        if (fast.result != RomCheck::Result::OK)
            append("[ROM] WARNING — not July 12 size\r\n");
        if (host.disc->open(bin_path)) {
            append("[DVD] volume=" + host.disc->volume_id() + "\r\n");
            append("[DVD] files=" + std::to_string(host.disc->list().size()) + "\r\n");
        } else {
            append("[DVD] " + host.disc->error() + "\r\n");
        }
    } else {
        append("[ROM] WARNING — no BIN selected\r\n");
    }

    GS::Translator tr;
    tr.set_frame(640, 448, GS::Psm::PSMCT32);
    tr.emit_rect_flat(0, 380, 640, 448, 0.4f, 0.15f, 0.55f);
    tr.emit_sprite(120, 180, 216, 300, 0, 0, 1, 1, 0);
    host.gpu->clear(25, 15, 45, 255);
    host.gpu->submit(tr.draws().data(), (uint32_t)tr.draws().size());
    append("[GS]  " + tr.debug_summary() + "\r\n");
    append("[GPU] " + host.gpu->info() + "\r\n");

    host.input->poll();
    auto p = host.input->pad(0);
    std::ostringstream os;
    os << "[PAD] buttons=0x" << std::hex << p.buttons << std::dec
       << " stick=(" << (int)p.lx << "," << (int)p.ly << ")\r\n";
    append(os.str());

    host.time->pump();
    append("[VBL] count=" + std::to_string(host.time->vblank_count()) + "\r\n");
    append("[AUD] " + host.audio->info() + "\r\n");
    append("\r\nOK — PS2 subsystems mapped to PC host.\r\n");

    host.shutdown_all();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowA("STATIC", "PaRappa2 PC — Host translation (GS/PAD/AUD/DVD/VBL)",
            WS_CHILD | WS_VISIBLE, 10, 8, 560, 18, hwnd, 0, 0, 0);
        CreateWindowA("BUTTON", "1. Open .bin", WS_CHILD | WS_VISIBLE,
            10, 32, 120, 28, hwnd, (HMENU)1, 0, 0);
        CreateWindowA("BUTTON", "2. Smoke (no BIN)", WS_CHILD | WS_VISIBLE,
            140, 32, 140, 28, hwnd, (HMENU)2, 0, 0);
        g_edit = CreateWindowA("EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            10, 70, 560, 380, hwnd, 0, 0, 0);
        HFONT f = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            0, 0, 0, FIXED_PITCH | FF_MODERN, "Consolas");
        if (f) SendMessageA(g_edit, WM_SETFONT, (WPARAM)f, TRUE);
        append(Host::translation_map_text());
        append("\r\nOpen July 12 .bin or run smoke without ROM.\r\n");
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            auto p = pick_bin(hwnd);
            if (!p.empty()) run_host_smoke(p);
        } else if (LOWORD(wParam) == 2) {
            run_host_smoke({});
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int show) {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.lpszClassName = "HostTranslateGUI";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    RegisterClassA(&wc);
    g_hwnd = CreateWindowA("HostTranslateGUI", "TestForIssues — PS2 to PC",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 500, 0, 0, hi, 0);
    ShowWindow(g_hwnd, show);
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
