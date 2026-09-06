#include <jni.h>
#include <string>
#include <android/log.h>
#include "../common/rom_check.h"

#define LOG_TAG "PaRappa2"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jstring JNICALL
Java_com_parappa_theandroid2_RomChecker_verifyIso(JNIEnv* env, jobject /* this */, jstring path) {
    const char* nativePath = env->GetStringUTFChars(path, nullptr);
    if (!nativePath) {
        return env->NewStringUTF("Error: path nulo");
    }

    LOGI("Verificando ISO: %s", nativePath);

    auto result = RomCheck::verify_iso(nativePath);

    env->ReleaseStringUTFChars(path, nativePath);

    std::string msg;
    switch (result.result) {
        case RomCheck::Result::OK:
            msg = "✅ " + result.message;
            break;
        case RomCheck::Result::WRONG_SIZE:
        case RomCheck::Result::WRONG_HASH:
            msg = "⚠️ " + result.message;
            break;
        default:
            msg = "❌ " + result.message;
            break;
    }

    return env->NewStringUTF(msg.c_str());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_parappa_theandroid2_RomChecker_isCorrectPrototype(JNIEnv* env, jobject /* this */, jstring path) {
    const char* nativePath = env->GetStringUTFChars(path, nullptr);
    if (!nativePath) return JNI_FALSE;

    auto result = RomCheck::verify_iso(nativePath);
    env->ReleaseStringUTFChars(path, nativePath);

    return (result.result == RomCheck::Result::OK) ? JNI_TRUE : JNI_FALSE;
}
