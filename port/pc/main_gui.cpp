// TestForIssues GUI - Win32 harness for PC devs
// - ROM verify (July 12 prototype)
// - ISO9660 list + extract assets the decomp needs

#include "../common/rom_check.h"
#include "../common/iso9660.h"

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
    EnableWindow(g_btn_open, b ? FALSE : TRUE);
    EnableWindow(g_btn_list, b ? FALSE : TRUE);
    EnableWindow(g_btn_extract, b ? FALSE : TRUE);
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

static void run_open_and_verify(std::string path) {
    set_busy(true);
    SetWindowTextA(g_edit, "");
    append_text("=== TestForIssues (PC harness) ===\r\n\r\n");
    append_text(("Image: " + path + "\r\n\r\n").c_str());

    set_status("Verificando ROM...");
    auto fast = RomCheck::verify_iso_fast(path);
    if (fast.result != RomCheck::Result::OK) {
        append_text(("WARN size: " + fast.message + "\r\n").c_str());
        append_text("Igual intento montar ISO9660...\r\n\r\n");
    } else {
        append_text("Size OK. SHA1 full check skipped for speed in harness.\r\n");
        append_text("(Usa verify estricto solo cuando quieras validar dump)\r\n\r\n");
    }

    Iso9660::Image img;
    set_status("Montando ISO9660...");
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
    append_text("ISO9660 montado. Usa 'List FS' o 'Extract assets'.\r\n");
    set_status("Image lista");
    set_busy(false);
}

static void run_list_fs() {
    if (g_image_path.empty()) {
        MessageBoxA(g_hwnd, "Primero abri un .bin", "TestForIssues", MB_OK);
        return;
    }
    set_busy(true);
    set_status("Listando filesystem...");
    append_text("\r\n=== LIST FS ===\r\n");

    Iso9660::Image img;
    if (!img.open(g_image_path)) {
        append_text((img.error() + "\r\n").c_str());
        set_busy(false);
        return;
    }

    auto entries = img.list_all();
    int files = 0, dirs = 0;
    for (auto& e : entries) {
        if (e.is_dir) {
            dirs++;
            append_text(("[DIR]  " + e.path + "\r\n").c_str());
        } else {
            files++;
            append_text(("[FILE] " + e.path + "  (" + std::to_string(e.size) + " bytes)\r\n").c_str());
        }
    }
    append_text(("\r\nTotal: " + std::to_string(files) + " files, " +
                 std::to_string(dirs) + " dirs\r\n").c_str());
    set_status("List listo");
    set_busy(false);
}

static void run_extract() {
    if (g_image_path.empty()) {
        MessageBoxA(g_hwnd, "Primero abri un .bin", "TestForIssues", MB_OK);
        return;
    }
    set_busy(true);
    set_status("Extrayendo assets decomp...");
    append_text("\r\n=== EXTRACT DECOMP ESSENTIALS ===\r\n");
    append_text("Target: ./extracted_iso/  (SCPS_150.17, IRX/*, MDL/*.OLM)\r\n\r\n");

    Iso9660::Image img;
    if (!img.open(g_image_path)) {
        append_text((img.error() + "\r\n").c_str());
        set_busy(false);
        return;
    }

    auto rep = img.extract_decomp_essentials("extracted_iso");
    for (auto& m : rep.messages) {
        append_text((m + "\r\n").c_str());
    }
    append_text(("\r\nExtracted: " + std::to_string(rep.extracted) +
                 " | Missing: " + std::to_string(rep.missing) + "\r\n").c_str());

    set_status(rep.missing == 0 ? "Extract OK" : "Extract con missing");
    set_busy(false);

    std::string msg = "Extracted " + std::to_string(rep.extracted) +
                      " files\nMissing " + std::to_string(rep.missing) +
                      "\n\nOutput folder: extracted_iso\\";
    MessageBoxA(g_hwnd, msg.c_str(), "Extract assets", MB_OK | MB_ICONINFORMATION);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowA("STATIC", "PaRappa 2 - TestForIssues (PC harness)",
            WS_CHILD | WS_VISIBLE, 10, 8, 560, 18, hwnd, nullptr, nullptr, nullptr);

        g_btn_open = CreateWindowA("BUTTON", "1. Open .bin",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 32, 120, 28, hwnd, (HMENU)1, nullptr, nullptr);

        g_btn_list = CreateWindowA("BUTTON", "2. List FS",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            140, 32, 120, 28, hwnd, (HMENU)2, nullptr, nullptr);

        g_btn_extract = CreateWindowA("BUTTON", "3. Extract assets",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            270, 32, 140, 28, hwnd, (HMENU)3, nullptr, nullptr);

        g_status = CreateWindowA("STATIC", "Esperando .bin...",
            WS_CHILD | WS_VISIBLE, 420, 36, 160, 20, hwnd, nullptr, nullptr, nullptr);

        g_edit = CreateWindowA("EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 70, 560, 370, hwnd, nullptr, nullptr, nullptr);

        HFONT font = CreateFontA(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
        if (font) SendMessageA(g_edit, WM_SETFONT, (WPARAM)font, TRUE);

        append_text("Harness de PC para testear sin Android.\r\n\r\n");
        append_text("1) Open .bin  -> monta el dump July 12\r\n");
        append_text("2) List FS    -> lista archivos del ISO9660\r\n");
        append_text("3) Extract    -> saca SCPS_150.17, IRX/*, MDL/*.OLM\r\n");
        append_text("               a ./extracted_iso/\r\n\r\n");
        append_text("Despues de extract podes usar esos files como el decomp.\r\n");
        return 0;
    }
    case WM_COMMAND: {
        if (g_busy) return 0;
        int id = LOWORD(wParam);
        if (id == 1) {
            std::string path = open_file_dialog(hwnd);
            if (!path.empty())
                std::thread([path]() { run_open_and_verify(path); }).detach();
        } else if (id == 2) {
            std::thread([]() { run_list_fs(); }).detach();
        } else if (id == 3) {
            std::thread([]() { run_extract(); }).detach();
        }
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

    g_hwnd = CreateWindowA("TestForIssuesGUI",
        "TestForIssues - PaRappa 2 PC Harness",
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
