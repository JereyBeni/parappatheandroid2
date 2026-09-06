#include <jni.h>
#include <string>
#include <android/log.h>

#include "rom_check.h"
#include "iso9660.h"
#include "prlib_stub.h"

#define LOG_TAG "PaRappa2"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_nativeInfo(JNIEnv* env, jobject) {
    std::string s =
        "parappa2 native OK\n"
        "modules: rom_check, iso9660, prlib_stub\n"
        "target: July 12 2001 NTSC-J Prototype\n"
        "SHA1: 28964c33cee578ec3ce476285067044243363d08\n";
    return env->NewStringUTF(s.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_MainActivity_verifyPath(JNIEnv* env, jobject, jstring path) {
    const char* p = env->GetStringUTFChars(path, nullptr);
    if (!p) return env->NewStringUTF("null path");

    auto fast = RomCheck::verify_iso_fast(p);
    std::string msg;
    if (fast.result == RomCheck::Result::OK) {
        msg = "SIZE OK\n" + fast.message;
        // optional full sha1 is too slow/heavy on phone by default
        msg += "\n(SHA1 full skipped on Android by default)";
    } else {
        msg = "FAIL\n" + fast.message;
    }

    env->ReleaseStringUTFChars(path, p);
    return env->NewStringUTF(msg.c_str());
}
