#include <jni.h>
#include <string>
#include <memory>
#include <mutex>

#include "rom_check.h"
#include "gs_translate.h"
#include "vk_backend.h"
#include "emu_session.h"

static std::mutex g_mu;
static std::unique_ptr<Emu::Session> g_session;

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_nativeInfo(JNIEnv* env, jobject) {
    std::string s =
        "parappa2 native OK\n"
        "backend: SoftDevice (CPU raster → screen)\n"
        "modules: emu_session gs_translate vk_backend\n";
    return env->NewStringUTF(s.c_str());
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
    std::string out = g_session->run_demo(n);
    return env->NewStringUTF(out.c_str());
}

// Boot persistent session for animated playback
extern "C" JNIEXPORT jboolean JNICALL
Java_com_parappa_theandroid2_MainActivity_emuBoot(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(g_mu);
    g_session = std::make_unique<Emu::Session>();
    return g_session->boot() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_emuStep(JNIEnv* env, jobject, jint frame) {
    std::lock_guard<std::mutex> lock(g_mu);
    if (!g_session) return env->NewStringUTF("not booted");
    // capture last log line via temporary
    std::string last;
    g_session->set_log([&](const std::string& s) { last = s; });
    g_session->step_frame((int)frame);
    g_session->set_log(nullptr);
    return env->NewStringUTF(last.c_str());
}

// Returns ARGB_8888 int[] for Bitmap.copyPixelsFromBuffer / setPixels
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
