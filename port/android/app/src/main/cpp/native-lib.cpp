#include <jni.h>
#include <string>
#include <memory>
#include <mutex>
#include <vector>

#include "rom_check.h"
#include "gs_translate.h"
#include "vk_backend.h"
#include "emu_session.h"

static std::mutex g_mu;
static std::unique_ptr<Emu::Session> g_session;
static bool g_rom_present = false;
static std::string g_rom_name;

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_nativeInfo(JNIEnv* env, jobject) {
    std::string s =
        "parappa2 native OK\n"
        "backend: SoftDevice\n";
    s += g_rom_present ? "[ROM] flagged present\n" : "[ROM] not flagged\n";
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_parappa_theandroid2_MainActivity_setRomPresent(JNIEnv* env, jobject, jboolean present, jstring name) {
    std::lock_guard<std::mutex> lock(g_mu);
    g_rom_present = (present == JNI_TRUE);
    g_rom_name.clear();
    if (name) {
        const char* p = env->GetStringUTFChars(name, nullptr);
        if (p) {
            g_rom_name = p;
            env->ReleaseStringUTFChars(name, p);
        }
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_verifyPath(JNIEnv* env, jobject, jstring path) {
    const char* p = env->GetStringUTFChars(path, nullptr);
    if (!p) return env->NewStringUTF("null path");
    auto fast = RomCheck::verify_iso_fast(p);
    std::string msg = (fast.result == RomCheck::Result::OK)
        ? ("SIZE OK\n" + fast.message) : ("FAIL\n" + fast.message);
    env->ReleaseStringUTFChars(path, p);
    return env->NewStringUTF(msg.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_gsVulkanSmoke(JNIEnv* env, jobject) {
    GS::Translator tr;
    tr.reset();
    tr.set_frame(320, 224, GS::Psm::PSMCT32);
    tr.emit_sprite(10, 10, 100, 100, 0, 0, 1, 1);
    std::unique_ptr<VKBackend::Device> dev(VKBackend::create_device());
    VKBackend::Config cfg;
    cfg.width = 320;
    cfg.height = 224;
    dev->init(cfg);
    auto& d = tr.draws();
    dev->submit(d.data(), (uint32_t)d.size());
    std::string out = tr.debug_summary() + "\n" + dev->info();
    return env->NewStringUTF(out.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_runEmuDemo(JNIEnv* env, jobject, jint frames) {
    std::lock_guard<std::mutex> lock(g_mu);
    g_session = std::make_unique<Emu::Session>();
    int n = frames > 0 ? (int)frames : 5;
    if (n > 120) n = 120;
    return env->NewStringUTF(g_session->run_demo(n).c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_emuBoot(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lock(g_mu);
    g_session = std::make_unique<Emu::Session>();
    Emu::SessionConfig cfg;
    cfg.rom_present = g_rom_present;
    cfg.rom_name = g_rom_name;
    if (!g_session->boot(cfg)) {
        return env->NewStringUTF("BOOT FAIL\n");
    }
    std::string msg = g_rom_present
        ? "[EMU] SoftDevice boot OK — ROM mode\n"
        : "[EMU] SoftDevice boot OK — DEMO (WARNING: no BIN)\n";
    return env->NewStringUTF(msg.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_emuStep(JNIEnv* env, jobject, jint frame) {
    std::lock_guard<std::mutex> lock(g_mu);
    if (!g_session) return env->NewStringUTF("not booted");
    std::string last;
    g_session->set_log([&](const std::string& s) { last = s; });
    g_session->step_frame((int)frame);
    g_session->set_log(nullptr);
    return env->NewStringUTF(last.c_str());
}

extern "C" JNIEXPORT jintArray JNICALL
Java_com_parappa_theandroid2_MainActivity_emuFramebuffer(JNIEnv* env, jobject) {
    std::lock_guard<std::mutex> lock(g_mu);
    if (!g_session || !g_session->device()) return nullptr;
    auto* dev = g_session->device();
    const uint8_t* fb = dev->framebuffer();
    uint32_t w = dev->fb_width();
    uint32_t h = dev->fb_height();
    if (!fb || !w || !h) return nullptr;

    size_t n = (size_t)w * h;
    jintArray arr = env->NewIntArray((jsize)n);
    if (!arr) return nullptr;
    std::vector<jint> argb(n);
    for (size_t i = 0; i < n; ++i) {
        uint8_t r = fb[i * 4 + 0];
        uint8_t g = fb[i * 4 + 1];
        uint8_t b = fb[i * 4 + 2];
        uint8_t a = fb[i * 4 + 3];
        argb[i] = ((jint)a << 24) | ((jint)r << 16) | ((jint)g << 8) | (jint)b;
    }
    env->SetIntArrayRegion(arr, 0, (jsize)n, argb.data());
    return arr;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_parappa_theandroid2_MainActivity_emuWidth(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(g_mu);
    if (!g_session || !g_session->device()) return 0;
    return (jint)g_session->device()->fb_width();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_parappa_theandroid2_MainActivity_emuHeight(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(g_mu);
    if (!g_session || !g_session->device()) return 0;
    return (jint)g_session->device()->fb_height();
}
