#include <jni.h>
#include <string>
#include <android/log.h>
#include <memory>

#include "rom_check.h"
#include "iso9660.h"
#include "prlib_stub.h"
#include "gs_translate.h"
#include "vk_backend.h"

#define LOG_TAG "PaRappa2"

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_nativeInfo(JNIEnv* env, jobject) {
    std::string s =
        "parappa2 native OK\n"
        "modules: rom_check, iso9660, prlib_stub, gs_translate, vk_backend\n"
        "path: B (native) + GS->Vulkan translation layer\n"
        "target: July 12 2001 NTSC-J Prototype\n";
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_verifyPath(JNIEnv* env, jobject, jstring path) {
    const char* p = env->GetStringUTFChars(path, nullptr);
    if (!p) return env->NewStringUTF("null path");

    auto fast = RomCheck::verify_iso_fast(p);
    std::string msg;
    if (fast.result == RomCheck::Result::OK) {
        msg = "SIZE OK\n" + fast.message + "\n(SHA1 full skipped on Android)";
    } else {
        msg = "FAIL\n" + fast.message;
    }
    env->ReleaseStringUTFChars(path, p);
    return env->NewStringUTF(msg.c_str());
}

// Smoke test: GS translator -> Null Vulkan backend
extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_gsVulkanSmoke(JNIEnv* env, jobject) {
    GS::Translator tr;
    tr.reset();
    tr.set_frame(640, 448, GS::Psm::PSMCT32);
    tr.set_tex(64, 64, GS::Psm::PSMCT32);
    tr.emit_sprite(0, 0, 64, 64, 0, 0, 1, 1);
    tr.emit_sprite(100, 100, 200, 200, 0, 0, 1, 1);

    std::unique_ptr<VKBackend::Device> dev(VKBackend::create_device());
    VKBackend::Config cfg;
    cfg.width = 640;
    cfg.height = 448;
    dev->init(cfg);

    // fake 2x2 texture upload
    uint8_t rgba[16] = {
        255,0,0,255,  0,255,0,255,
        0,0,255,255,  255,255,0,255
    };
    dev->upload_texture_rgba(1, 2, 2, rgba);

    auto& draws = tr.draws();
    if (!draws.empty())
        dev->submit(draws.data(), (uint32_t)draws.size());

    std::string out = tr.debug_summary();
    out += "\n";
    out += dev->info();
    out += "\nGS->Vulkan translation pipeline: OK (null backend)\n";
    out += "Next: real VkDevice + TIM2 textures from extract\n";
    return env->NewStringUTF(out.c_str());
}
