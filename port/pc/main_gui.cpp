// TestForIssues GUI - Win32 harness for PC devs
// - ISO9660 mount / list / extract
// - prlib: TIM2 + SPM scan on extracted assets

#include "../common/rom_check.h"
#include "../common/iso9660.h"
#include "../common/prlib_stub.h"

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
static HWND g_btn_open = nullptr;
static HWND g_btn_list = nullptr;
static HWND g_btn_extract = nullptr;
static HWND g_btn_prlib = nullptr;
static HWND g_status = nullptr;
static std::atomic<bool> g_busy{false};
static std::string g_image_path;

static void append_text(const std::string& text) {
    if (!g_edit) return;
    int len = GetWindowTextLengthA(g_edit);
    SendMessageA(g_edit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageA(g_edit, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
}

static void set_status(const char* msg) {
    if (g_status) SetWindowTextA(g_status, msg);
}

static void set_busy(bool b) {
    g_busy = b;
    BOOL en = b ? FALSE : TRUE;
    EnableWindow(g_btn_open, en);
    EnableWindow(g_btn_list, en);
    EnableWindow(g_btn_extract, en);
    EnableWindow(g_btn_prlib, en);
}

static std::string open_file_dialog(HWND owner) {
    char file[MAX_PATH] = {};
    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "PS2 Dump (*.bin;*.iso)\0*.bin;*.iso\0All (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    ofn.lpstrTitle = "Seleccionar PS2 - Parappa 7-12-07.bin";
    if (GetOpenFileNameA(&ofn)) return std::string(file);
    return {};
}

static void run_open_and_verify(std::string path) {
    set_busy(true);
    SetWindowTextA(g_edit, "");
    append_text("=== TestForIssues (PC harness) ===\r\n\r\n");
    append_text(("Image: " + path + "\r\n\r\n").c_str());

    auto fast = RomCheck::verify_iso_fast(path);
    if (fast.result != RomCheck::Result::OK)
        append_text(("WARN size: " + fast.message + "\r\n").c_str());
    else
        append_text("Size OK (July 12 prototype size match)\r\n\r\n");

    Iso9660::Image img;
    if (!img.open(path)) {
        append_text(("FAIL mount: " + img.error() + "\r\n").c_str());
        set_status("No se pudo montar");
        g_image_path.clear();
        set_busy(false);
        MessageBoxA(g_hwnd, img.error().c_str(), "ISO9660", MB_OK | MB_ICONERROR);
        return;
    }
    g_image_path = path;
    append_text(("Volume ID: " + img.volume_id() + "\r\n").c_str());
    append_text("Montado. Siguiente: List / Extract / prlib scan.\r\n");
    set_status("Image lista");
    set_busy(false);
}

static void run_list_fs() {
    if (g_image_path.empty()) { MessageBoxA(g_hwnd, "Primero Open .bin", "TestForIssues", MB_OK); return; }
    set_busy(true);
    set_status("Listando FS...");
    append_text("\r\n=== LIST FS ===\r\n");
    Iso9660::Image img;
    if (!img.open(g_image_path)) { append_text(img.error().c_str()); set_busy(false); return; }
    auto entries = img.list_all();
    int files = 0, dirs = 0;
    for (auto& e : entries) {
        if (e.is_dir) { dirs++; append_text(("[DIR]  " + e.path + "\r\n").c_str()); }
        else { files++; append_text(("[FILE] " + e.path + " (" + std::to_string(e.size) + ")\r\n").c_str()); }
    }
    append_text(("\r\nTotal: " + std::to_string(files) + " files, " + std::to_string(dirs) + " dirs\r\n").c_str());
    set_status("List listo");
    set_busy(false);
}

static void run_extract() {
    if (g_image_path.empty()) { MessageBoxA(g_hwnd, "Primero Open .bin", "TestForIssues", MB_OK); return; }
    set_busy(true);
    set_status("Extracting...");
    append_text("\r\n=== EXTRACT (decomp essentials) ===\r\n");
    Iso9660::Image img;
    if (!img.open(g_image_path)) { append_text(img.error().c_str()); set_busy(false); return; }
    auto rep = img.extract_decomp_essentials("extracted_iso");
    for (auto& m : rep.messages) append_text((m + "\r\n").c_str());
    append_text(("\r\nExtracted " + std::to_string(rep.extracted) + " | Missing " + std::to_string(rep.missing) + "\r\n").c_str());
    set_status(rep.missing == 0 ? "Extract OK" : "Extract partial");
    set_busy(false);
    MessageBoxA(g_hwnd, "Mira extracted_iso\\ al lado del exe", "Extract", MB_OK | MB_ICONINFORMATION);
}

static void run_prlib_scan() {
    set_busy(true);
    set_status("prlib scan...");
    append_text("\r\n=== prlib SCAN (TIM2 / SPM) ===\r\n");
    append_text("Escaneando extracted_iso\\ ...\r\n\r\n");

    auto res = PrlibStub::scan_extracted_dir("extracted_iso");
    for (auto& line : res.lines) {
        append_text((line + "\r\n").c_str());
    }

    if (res.tim2_count == 0 && res.spm_count == 0 && res.other_count == 0) {
        append_text("\r\nNada en extracted_iso. Corre '3. Extract assets' primero.\r\n");
        set_status("Sin assets");
    } else {
        set_status("prlib scan OK");
    }
    set_busy(false);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowA("STATIC", "PaRappa 2 - TestForIssues + prlib stub",
            WS_CHILD | WS_VISIBLE, 10, 6, 580, 18, hwnd, nullptr, nullptr, nullptr);

        g_btn_open = CreateWindowA("BUTTON", "1. Open .bin",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 28, 110, 26, hwnd, (HMENU)1, nullptr, nullptr);
        g_btn_list = CreateWindowA("BUTTON", "2. List FS",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 125, 28, 100, 26, hwnd, (HMENU)2, nullptr, nullptr);
        g_btn_extract = CreateWindowA("BUTTON", "3. Extract",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 230, 28, 100, 26, hwnd, (HMENU)3, nullptr, nullptr);
        g_btn_prlib = CreateWindowA("BUTTON", "4. prlib scan",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 335, 28, 110, 26, hwnd, (HMENU)4, nullptr, nullptr);

        g_status = CreateWindowA("STATIC", "listo",
            WS_CHILD | WS_VISIBLE, 455, 32, 120, 18, hwnd, nullptr, nullptr, nullptr);

        g_edit = CreateWindowA("EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 60, 560, 380, hwnd, nullptr, nullptr, nullptr);

        HFONT font = CreateFontA(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FIXED_PITCH | FF_MODERN, "Consolas");
        if (font) SendMessageA(g_edit, WM_SETFONT, (WPARAM)font, TRUE);

        append_text("PC harness para laburar sin Android.\r\n\r\n");
        append_text("1 Open .bin     monta ISO9660\r\n");
        append_text("2 List FS       lista el disco\r\n");
        append_text("3 Extract       SCPS_150.17 + IRX + MDL/*.OLM\r\n");
        append_text("4 prlib scan    busca TIM2 (texturas) y SPM (modelos)\r\n");
        append_text("                en extracted_iso\\\r\n\r\n");
        append_text("prlib arranca aca: parse TIM2 + header SPM.\r\n");
        return 0;
    }
    case WM_COMMAND: {
        if (g_busy) return 0;
        int id = LOWORD(wParam);
        if (id == 1) {
            auto p = open_file_dialog(hwnd);
            if (!p.empty()) std::thread([p]() { run_open_and_verify(p); }).detach();
        } else if (id == 2) std::thread([]() { run_list_fs(); }).detach();
        else if (id == 3) std::thread([]() { run_extract(); }).detach();
        else if (id == 4) std::thread([]() { run_prlib_scan(); }).detach();
        return 0;
    }
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

    g_hwnd = CreateWindowA("TestForIssuesGUI", "TestForIssues + prlib",
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
