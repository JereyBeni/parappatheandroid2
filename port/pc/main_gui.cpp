// TestForIssues PC — sm64ex-coop style flow:
// 1) Big window: "Import a BIN!"
// 2) Correct July 12 BIN -> loading assets + tips
// 3) Ready: host translation layer online

#include "host_translate.h"
#include "rom_check.h"
#include "iso9660.h"
#include "gs_translate.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

enum class UiPhase {
    ImportPrompt,  // no ROM — big "Import a BIN!"
    Loading,       // extracting / verifying
    Ready,         // host up
    Error,
};

static HWND g_hwnd = nullptr;
static HWND g_canvas = nullptr;   // owner-draw area
static HWND g_log = nullptr;
static HWND g_btnImport = nullptr;
static HWND g_btnQuit = nullptr;

static UiPhase g_phase = UiPhase::ImportPrompt;
static std::string g_status;
static std::string g_tip;
static std::string g_romPath;
static std::atomic<bool> g_busy{false};
static int g_loadPct = 0;

static const char* kTips[] = {
    "Tip: July 12 2001 NTSC-J prototype only (Hidden Palace dump).",
    "Tip: Expected size 4,159,078,400 bytes — we check that first.",
    "Tip: GS draws go through Host::Gpu (SoftDevice → Vulkan later).",
    "Tip: Pad maps to keyboard for now (Z=Cross, arrows=D-pad).",
    "Tip: Audio is a PCM queue stub until WASAPI/SDL.",
    "Tip: prlib models (.OLM / SPM) load after ISO9660 extract.",
    "Tip: Same host layer targets PC first, Android later.",
    "Tip: sm64ex-coop style: no ROM, no game — import the BIN.",
};
static const int kTipCount = sizeof(kTips) / sizeof(kTips[0]);

static void set_phase(UiPhase p) {
    g_phase = p;
    if (g_canvas) InvalidateRect(g_canvas, nullptr, TRUE);
    if (g_btnImport) {
        EnableWindow(g_btnImport, (p == UiPhase::ImportPrompt || p == UiPhase::Error || p == UiPhase::Ready) ? TRUE : FALSE);
        SetWindowTextA(g_btnImport, p == UiPhase::Ready ? "Re-import BIN" : "Import a BIN!");
    }
}

static void append_log(const std::string& s) {
    if (!g_log) return;
    int len = GetWindowTextLengthA(g_log);
    SendMessageA(g_log, EM_SETSEL, len, len);
    SendMessageA(g_log, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
}

static std::string open_bin_dialog() {
    char file[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "PS2 BIN/ISO (*.bin;*.iso)\0*.bin;*.iso\0All\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    ofn.lpstrTitle = "Import July 12 prototype BIN";
    if (GetOpenFileNameA(&ofn)) return file;
    return {};
}

static void paint_canvas(HDC hdc, RECT rc) {
    // Background
    HBRUSH bg = CreateSolidBrush(RGB(18, 14, 28));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    SetBkMode(hdc, TRANSPARENT);

    if (g_phase == UiPhase::ImportPrompt || g_phase == UiPhase::Error) {
        // Big title like sm64ex-coop
        HFONT big = CreateFontA(48, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
            0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        HFONT old = (HFONT)SelectObject(hdc, big);
        SetTextColor(hdc, RGB(255, 220, 80));
        const char* title = (g_phase == UiPhase::Error) ? "Invalid BIN" : "Import a BIN!";
        DrawTextA(hdc, title, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdc, old);
        DeleteObject(big);

        HFONT sub = CreateFontA(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        old = (HFONT)SelectObject(hdc, sub);
        SetTextColor(hdc, RGB(200, 200, 210));
        RECT r2 = rc;
        r2.top = (rc.bottom + rc.top) / 2 + 40;
        DrawTextA(hdc,
            "PaRappa the Rapper 2 — July 12 2001 NTSC-J prototype\n"
            "Drop-in: PS2 - Parappa 7-12-07.bin  (4,159,078,400 bytes)",
            -1, &r2, DT_CENTER | DT_TOP);
        if (g_phase == UiPhase::Error && !g_status.empty()) {
            r2.top += 60;
            SetTextColor(hdc, RGB(255, 120, 120));
            DrawTextA(hdc, g_status.c_str(), -1, &r2, DT_CENTER | DT_TOP);
        }
        SelectObject(hdc, old);
        DeleteObject(sub);
        return;
    }

    if (g_phase == UiPhase::Loading) {
        HFONT title = CreateFontA(32, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
            0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        HFONT old = (HFONT)SelectObject(hdc, title);
        SetTextColor(hdc, RGB(180, 255, 180));
        RECT r = rc;
        r.top += 40;
        DrawTextA(hdc, "Loading assets...", -1, &r, DT_CENTER | DT_TOP);
        SelectObject(hdc, old);
        DeleteObject(title);

        // Progress bar
        int barW = (rc.right - rc.left) - 80;
        int barH = 24;
        int bx = rc.left + 40;
        int by = rc.top + 100;
        RECT outline = { bx, by, bx + barW, by + barH };
        HBRUSH edge = CreateSolidBrush(RGB(80, 80, 100));
        FrameRect(hdc, &outline, edge);
        DeleteObject(edge);
        int fill = (barW * g_loadPct) / 100;
        RECT fillR = { bx + 2, by + 2, bx + 2 + fill, by + barH - 2 };
        HBRUSH fillB = CreateSolidBrush(RGB(255, 200, 40));
        FillRect(hdc, &fillR, fillB);
        DeleteObject(fillB);

        HFONT tipF = CreateFontA(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        old = (HFONT)SelectObject(hdc, tipF);
        SetTextColor(hdc, RGB(220, 210, 255));
        RECT tipR = rc;
        tipR.top = by + 50;
        tipR.left += 40;
        tipR.right -= 40;
        std::string tipLine = g_tip.empty() ? kTips[0] : g_tip;
        DrawTextA(hdc, tipLine.c_str(), -1, &tipR, DT_CENTER | DT_WORDBREAK);
        if (!g_status.empty()) {
            tipR.top += 70;
            SetTextColor(hdc, RGB(180, 180, 190));
            DrawTextA(hdc, g_status.c_str(), -1, &tipR, DT_CENTER | DT_WORDBREAK);
        }
        SelectObject(hdc, old);
        DeleteObject(tipF);
        return;
    }

    if (g_phase == UiPhase::Ready) {
        HFONT title = CreateFontA(28, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
            0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
        HFONT old = (HFONT)SelectObject(hdc, title);
        SetTextColor(hdc, RGB(120, 255, 160));
        RECT r = rc;
        r.top += 30;
        DrawTextA(hdc, "ROM loaded — host ready", -1, &r, DT_CENTER | DT_TOP);
        SelectObject(hdc, old);
        DeleteObject(title);

        HFONT body = CreateFontA(15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            0, 0, CLEARTYPE_QUALITY, FIXED_PITCH, "Consolas");
        old = (HFONT)SelectObject(hdc, body);
        SetTextColor(hdc, RGB(210, 210, 220));
        r.top += 50;
        r.left += 30;
        r.right -= 30;
        std::string bodyText = g_status;
        DrawTextA(hdc, bodyText.c_str(), -1, &r, DT_LEFT | DT_TOP | DT_WORDBREAK);
        SelectObject(hdc, old);
        DeleteObject(body);
    }
}

static LRESULT CALLBACK CanvasProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        paint_canvas(hdc, rc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void load_job(std::string path) {
    g_busy = true;
    set_phase(UiPhase::Loading);
    g_loadPct = 0;
    g_status = "Checking ROM...";
    g_tip = kTips[0];
    InvalidateRect(g_canvas, nullptr, TRUE);

    auto tip_at = [](int pct) {
        int idx = (pct * kTipCount) / 100;
        if (idx >= kTipCount) idx = kTipCount - 1;
        return std::string(kTips[idx]);
    };

    // 1) Size check
    g_loadPct = 10;
    g_tip = tip_at(10);
    g_status = "Verifying file size (July 12)...";
    InvalidateRect(g_canvas, nullptr, TRUE);
    auto fast = RomCheck::verify_iso_fast(path);
    if (fast.result != RomCheck::Result::OK) {
        g_status = "WRONG ROM: " + fast.message +
                   "\nNeed July 12 prototype (4159078400 bytes).";
        set_phase(UiPhase::Error);
        append_log("[ROM] FAIL " + fast.message + "\r\n");
        g_busy = false;
        return;
    }
    append_log("[ROM] size OK\r\n");

    // 2) Mount ISO
    g_loadPct = 30;
    g_tip = tip_at(30);
    g_status = "Mounting ISO9660...";
    InvalidateRect(g_canvas, nullptr, TRUE);

    Host::Context host;
    if (!host.init_all()) {
        g_status = "Host init failed";
        set_phase(UiPhase::Error);
        g_busy = false;
        return;
    }
    if (!host.disc->open(path)) {
        g_status = "ISO mount failed: " + host.disc->error();
        set_phase(UiPhase::Error);
        append_log("[DVD] " + host.disc->error() + "\r\n");
        host.shutdown_all();
        g_busy = false;
        return;
    }
    append_log("[DVD] volume=" + host.disc->volume_id() + "\r\n");

    // 3) List / "load" essentials
    g_loadPct = 55;
    g_tip = tip_at(55);
    g_status = "Scanning disc filesystem...";
    InvalidateRect(g_canvas, nullptr, TRUE);
    auto entries = host.disc->list();
    append_log("[DVD] entries=" + std::to_string(entries.size()) + "\r\n");

    g_loadPct = 70;
    g_tip = tip_at(70);
    g_status = "Looking for SCPS_150.17 / IRX / MDL...";
    InvalidateRect(g_canvas, nullptr, TRUE);

    int found_olm = 0, found_irx = 0;
    bool found_elf = false;
    for (auto& e : entries) {
        if (e.is_dir) continue;
        std::string up = e.path;
        for (char& c : up) if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        if (up.find("SCPS_150") != std::string::npos) found_elf = true;
        if (up.size() >= 4 && up.compare(up.size() - 4, 4, ".OLM") == 0) found_olm++;
        if (up.size() >= 4 && up.compare(up.size() - 4, 4, ".IRX") == 0) found_irx++;
    }
    append_log("[ASSET] ELF=" + std::string(found_elf ? "yes" : "no") +
               " OLM=" + std::to_string(found_olm) +
               " IRX=" + std::to_string(found_irx) + "\r\n");

    // 4) Init GS path
    g_loadPct = 90;
    g_tip = tip_at(90);
    g_status = "Init GS → Host::Gpu...";
    InvalidateRect(g_canvas, nullptr, TRUE);
    GS::Translator tr;
    tr.set_frame(640, 448, GS::Psm::PSMCT32);
    tr.emit_rect_flat(0, 380, 640, 448, 0.35f, 0.15f, 0.5f);
    host.gpu->clear(20, 12, 36, 255);
    host.gpu->submit(tr.draws().data(), (uint32_t)tr.draws().size());

    g_loadPct = 100;
    g_tip = tip_at(99);
    g_romPath = path;

    std::string summary;
    summary += Host::translation_map_text();
    summary += "\nROM: " + path + "\n";
    summary += "Volume: " + host.disc->volume_id() + "\n";
    summary += "Files: " + std::to_string(entries.size()) +
               " | OLM=" + std::to_string(found_olm) +
               " | IRX=" + std::to_string(found_irx) + "\n";
    summary += host.gpu->info() + "\n";
    summary += host.input->info() + "\n";
    summary += "Ready. (Full gameplay still needs prlib + real Vulkan/audio.)\n";
    g_status = summary;

    append_log("[EMU] Ready\r\n");
    host.shutdown_all();
    set_phase(UiPhase::Ready);
    g_busy = false;
    InvalidateRect(g_canvas, nullptr, TRUE);
}

static void start_import() {
    if (g_busy) return;
    auto path = open_bin_dialog();
    if (path.empty()) return;
    append_log("Import: " + path + "\r\n");
    std::thread([path]() { load_job(path); }).detach();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // Register canvas class
        WNDCLASSA cc = {};
        cc.lpfnWndProc = CanvasProc;
        cc.hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
        cc.lpszClassName = "ImportCanvas";
        cc.hbrBackground = nullptr;
        cc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassA(&cc);

        g_canvas = CreateWindowA("ImportCanvas", "",
            WS_CHILD | WS_VISIBLE,
            0, 0, 800, 360, hwnd, nullptr, cc.hInstance, nullptr);

        g_btnImport = CreateWindowA("BUTTON", "Import a BIN!",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            20, 370, 160, 36, hwnd, (HMENU)1, nullptr, nullptr);

        g_btnQuit = CreateWindowA("BUTTON", "Quit",
            WS_CHILD | WS_VISIBLE,
            190, 370, 100, 36, hwnd, (HMENU)2, nullptr, nullptr);

        g_log = CreateWindowA("EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
            ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
            20, 420, 760, 140, hwnd, nullptr, nullptr, nullptr);

        HFONT f = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            0, 0, 0, FIXED_PITCH, "Consolas");
        if (f) SendMessageA(g_log, WM_SETFONT, (WPARAM)f, TRUE);

        append_log("=== PaRappa 2 PC harness (sm64ex-coop style import) ===\r\n");
        append_log("Import PS2 - Parappa 7-12-07.bin to continue.\r\n");
        set_phase(UiPhase::ImportPrompt);
        return 0;
    }
    case WM_SIZE: {
        int w = LOWORD(lParam), h = HIWORD(lParam);
        if (g_canvas) MoveWindow(g_canvas, 0, 0, w, h - 220, TRUE);
        if (g_btnImport) MoveWindow(g_btnImport, 20, h - 200, 160, 36, TRUE);
        if (g_btnQuit) MoveWindow(g_btnQuit, 190, h - 200, 100, 36, TRUE);
        if (g_log) MoveWindow(g_log, 20, h - 155, w - 40, 140, TRUE);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) start_import();
        if (LOWORD(wParam) == 2) PostQuitMessage(0);
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
    wc.lpszClassName = "PaRappa2Import";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wc);

    g_hwnd = CreateWindowA("PaRappa2Import",
        "PaRappa the Rapper 2 — Import a BIN!",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 820, 640,
        nullptr, nullptr, hi, nullptr);

    ShowWindow(g_hwnd, show);
    UpdateWindow(g_hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
