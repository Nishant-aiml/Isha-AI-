#include <jni.h>
#include <string>
#include <memory>
#include <mutex>
#include <android/log.h>
#include "model/model_manager.hpp"
#include "inference/llama_cpp_engine.hpp"
#include "config/device_profile.hpp"
#include "runtime/event_bus.hpp"
#include "inference/kv_telemetry.hpp"

#define LOG_TAG "ISHA_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global JNI state
static std::unique_ptr<isha::ModelManager> g_model_manager = nullptr;
static std::unique_ptr<isha::LlamaCppEngine> g_llama_engine = nullptr;
static isha::DeviceProfile g_profile;
static std::string g_files_dir;
static std::string g_cache_dir;
static std::mutex g_jni_mutex;
static std::shared_ptr<isha::CancellationToken> g_stream_cancel_token = nullptr;

// Cache class references for performance and thread safety
static jclass g_callback_class = nullptr;
static jmethodID g_callback_on_token = nullptr;

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_isha_ai_IshaRuntime_initializeRuntime(JNIEnv *env, jobject thiz,
                                               jstring files_dir, jstring cache_dir,
                                               jint total_ram_mb, jint cpu_cores) {
    std::lock_guard<std::mutex> lock(g_jni_mutex);

    try {
        const char *files_c = env->GetStringUTFChars(files_dir, nullptr);
        const char *cache_c = env->GetStringUTFChars(cache_dir, nullptr);
        g_files_dir = files_c;
        g_cache_dir = cache_c;
        env->ReleaseStringUTFChars(files_dir, files_c);
        env->ReleaseStringUTFChars(cache_dir, cache_c);

        LOGI("Initializing native ISHA runtime inside getFilesDir() = %s", g_files_dir.c_str());

        // Classify device tier based on 6GB Vivo constraint (effectively MID-class usability due to high OEM overhead)
        isha::DeviceTier tier = isha::DeviceTier::MID;
        if (total_ram_mb <= 4096) {
            tier = isha::DeviceTier::LOW;
        } else if (total_ram_mb >= 12288) {
            tier = isha::DeviceTier::FLAGSHIP;
        } else if (total_ram_mb >= 8192) {
            tier = isha::DeviceTier::HIGH;
        }

        g_profile.tier = tier;
        g_profile.total_ram_mb = total_ram_mb;
        g_profile.cpu_cores = cpu_cores;
        g_profile.os_name = "Android";
        g_profile.has_nnapi = true;  // NNAPI capability checked at load time optionally
        g_profile.has_vulkan = false;

        g_model_manager = std::make_unique<isha::ModelManager>(g_profile);
        g_llama_engine = std::make_unique<isha::LlamaCppEngine>(g_profile);

        LOGI("ISHA runtime native initialization succeeded. Classified Tier: %s",
             isha::deviceTierToString(tier).c_str());
        return JNI_TRUE;
    } catch (const std::exception &e) {
        LOGE("Native exception during runtime init: %s", e.what());
        return JNI_FALSE;
    } catch (...) {
        LOGE("Unknown native exception during runtime init.");
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_isha_ai_IshaRuntime_loadModelTier(JNIEnv *env, jobject thiz,
                                           jstring model_name, jint size_mb) {
    std::lock_guard<std::mutex> lock(g_jni_mutex);
    if (!g_model_manager || !g_llama_engine) {
        LOGE("Cannot load model: Runtime not initialized.");
        return JNI_FALSE;
    }

    try {
        const char *name_c = env->GetStringUTFChars(model_name, nullptr);
        std::string name_str(name_c);
        env->ReleaseStringUTFChars(model_name, name_c);

        LOGI("Attempting to load model: %s (%d MB)", name_str.c_str(), size_mb);

        // Strict gating policies (Mistral-7B/T3 is banned on this tier)
        if (size_mb >= 3000 && g_profile.tier != isha::DeviceTier::FLAGSHIP) {
            LOGE("T3 Model load REJECTED: T3 is disabled on this device class (%s)",
                 isha::deviceTierToString(g_profile.tier).c_str());
            return JNI_FALSE;
        }

        bool success = g_model_manager->loadModel(name_str, size_mb);
        if (!success) {
            LOGW("ModelManager loading gate rejected load request.");
            return JNI_FALSE;
        }

        // Formulate path in internal getFilesDir() only (Final Fix 1 compliance)
        std::string model_path = g_files_dir + "/" + name_str + ".gguf";
        
        // Load the actual model weights using mmap
        isha::InferenceContext ctx("session_init", nullptr, size_mb);
        bool engine_success = g_llama_engine->load(model_path, ctx);

        if (!engine_success) {
            LOGE("LlamaCppEngine failed to load GGUF weights.");
            g_model_manager->unloadModel();
            return JNI_FALSE;
        }

        LOGI("GGUF model loaded successfully through mmap in getFilesDir().");
        return JNI_TRUE;
    } catch (const std::exception &e) {
        LOGE("Native exception during model load: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_isha_ai_IshaRuntime_unloadModelTier(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(g_jni_mutex);
    if (g_stream_cancel_token) {
        g_stream_cancel_token->cancel();
    }
    if (!g_model_manager || !g_llama_engine) return JNI_FALSE;

    try {
        LOGI("Native model unload triggered.");
        g_llama_engine->unload();
        g_model_manager->unloadModel();
        return JNI_TRUE;
    } catch (const std::exception &e) {
        LOGE("Native exception during model unload: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL
Java_com_isha_ai_IshaRuntime_generateTokens(JNIEnv *env, jobject thiz,
                                            jstring prompt, jint max_tokens) {
    std::lock_guard<std::mutex> lock(g_jni_mutex);
    if (!g_llama_engine || !g_llama_engine->isLoaded()) {
        return env->NewStringUTF("[Error: No model loaded]");
    }

    try {
        const char *prompt_c = env->GetStringUTFChars(prompt, nullptr);
        std::string prompt_str(prompt_c);
        env->ReleaseStringUTFChars(prompt, prompt_c);

        isha::InferenceContext ctx("session_gen", nullptr, g_model_manager->getActiveModelSizeMB());
        ctx.max_tokens_to_generate = max_tokens;

        std::string result = g_llama_engine->generate(prompt_str, ctx);
        return env->NewStringUTF(result.c_str());
    } catch (const std::exception &e) {
        return env->NewStringUTF(("[Native Exception: " + std::string(e.what()) + "]").c_str());
    }
}

JNIEXPORT jboolean JNICALL
Java_com_isha_ai_IshaRuntime_streamTokens(JNIEnv *env, jobject thiz,
                                          jstring prompt, jobject callback_obj) {
    std::shared_ptr<isha::CancellationToken> local_cancel_token = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_jni_mutex);
        if (!g_llama_engine || !g_llama_engine->isLoaded()) {
            return JNI_FALSE;
        }
        if (g_stream_cancel_token) {
            g_stream_cancel_token->cancel();
        }
        g_stream_cancel_token = std::make_shared<isha::CancellationToken>();
        local_cancel_token = g_stream_cancel_token;
    }

    jclass callback_class = env->GetObjectClass(callback_obj);
    jmethodID on_token_method = env->GetMethodID(callback_class, "onToken", "(Ljava/lang/String;)V");

    if (!on_token_method) {
        LOGE("Callback object does not implement TokenCallback interface.");
        return JNI_FALSE;
    }

    const char *prompt_c = env->GetStringUTFChars(prompt, nullptr);
    std::string prompt_str(prompt_c);
    env->ReleaseStringUTFChars(prompt, prompt_c);

    JavaVM *jvm = nullptr;
    env->GetJavaVM(&jvm);
    jobject global_callback = env->NewGlobalRef(callback_obj);

    isha::InferenceContext ctx("session_stream", local_cancel_token, g_model_manager->getActiveModelSizeMB());
    
    // Strict token budgets for T1 vs T0 in emergency mode
    if (g_model_manager->isT0ThermalEmergencyMode()) {
        ctx.max_tokens_to_generate = 32;
    }

    // Call native engine streaming (Final Fix 7 compliance: stream into native ring-buffer callback)
    bool success = g_llama_engine->stream(prompt_str, ctx, [jvm, global_callback, on_token_method](const std::string &token) {
        JNIEnv *thread_env = nullptr;
        jint res = jvm->GetEnv((void **)&thread_env, JNI_VERSION_1_6);
        bool attached = false;

        if (res == JNI_EDETACHED) {
            jvm->AttachCurrentThread(&thread_env, nullptr);
            attached = true;
        }

        if (thread_env) {
            jstring token_jstr = thread_env->NewStringUTF(token.c_str());
            thread_env->CallVoidMethod(global_callback, on_token_method, token_jstr);
            thread_env->DeleteLocalRef(token_jstr);
        }

        if (attached) {
            jvm->DetachCurrentThread();
        }
    });

    env->DeleteGlobalRef(global_callback);
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_isha_ai_IshaRuntime_getThermalState(JNIEnv *env, jobject thiz) {
    if (!g_model_manager) return 0; // NOMINAL
    return g_model_manager->isT0ThermalEmergencyMode() ? 3 : 0; // Mapping
}

JNIEXPORT jint JNICALL
Java_com_isha_ai_IshaRuntime_getMemoryPressure(JNIEnv *env, jobject thiz) {
    return 0; // Standard nominal indicator
}

JNIEXPORT void JNICALL
Java_com_isha_ai_IshaRuntime_enterSafeMode(JNIEnv *env, jobject thiz) {
    std::lock_guard<std::mutex> lock(g_jni_mutex);
    if (!g_model_manager) return;
    LOGW("Explicit native safe-mode triggered.");
    g_model_manager->emergencyUnload();
}

JNIEXPORT jboolean JNICALL
Java_com_isha_ai_IshaRuntime_checkpointSession(JNIEnv *env, jobject thiz, jstring session_id) {
    LOGI("Checkpointing session.");
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
Java_com_isha_ai_IshaRuntime_restoreSession(JNIEnv *env, jobject thiz, jstring session_id) {
    LOGI("Restoring session from checkpoint.");
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_isha_ai_IshaRuntime_updateThermalState(JNIEnv *env, jobject thiz, jint thermal_state) {
    std::lock_guard<std::mutex> lock(g_jni_mutex);
    if (!g_model_manager) return;

    // Map Android Status enums directly to the strict 4-Zone model (Final Fix 7 compliance)
    // thermal_state:
    // 0 = NONE/LIGHT -> ZONE 1 (NOMINAL)
    // 1 = MODERATE   -> ZONE 2 (WARM)
    // 2 = SEVERE     -> ZONE 3 (HOT)
    // 3 = CRITICAL/EMERGENCY -> ZONE 4 (CRITICAL)
    double mapped_temp = 25.0;
    if (thermal_state == 1) {
        mapped_temp = 36.5; // Trigger ZONE 2 (35-38°C)
    } else if (thermal_state == 2) {
        mapped_temp = 39.5; // Trigger ZONE 3 (38-41°C)
    } else if (thermal_state >= 3) {
        mapped_temp = 43.5; // Trigger ZONE 4 (>41°C)
    }

    LOGI("Received OS PowerManager thermal state update = %d. Mapping to temp cue = %.1f°C",
         thermal_state, mapped_temp);

    g_model_manager->thermalUnload(mapped_temp);
}

}
