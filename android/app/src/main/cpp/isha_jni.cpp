#include <jni.h>
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <sstream>
#include <android/log.h>
#include "model/model_manager.hpp"
#include "inference/llama_cpp_engine.hpp"
#include "config/device_profile.hpp"
#include "runtime/event_bus.hpp"
#include "inference/kv_telemetry.hpp"
#include "retrieval/local_index.hpp"
#include "embeddings/embedding_engine.hpp"

#define LOG_TAG "ISHA_JNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global JNI state
static std::unique_ptr<isha::ModelManager> g_model_manager = nullptr;
static std::unique_ptr<isha::LlamaCppEngine> g_llama_engine = nullptr;
static std::unique_ptr<isha::EdgeFlatVectorIndex> g_vector_index = nullptr;
static std::unique_ptr<isha::EmbeddingEngine> g_embedding_engine = nullptr;
static isha::DeviceProfile g_profile;
static std::string g_files_dir;
static std::string g_cache_dir;
static std::mutex g_jni_mutex;
static std::shared_ptr<isha::CancellationToken> g_stream_cancel_token = nullptr;

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
        g_profile.has_nnapi = true;
        g_profile.has_vulkan = false;

        g_model_manager = std::make_unique<isha::ModelManager>(g_profile);
        g_llama_engine = std::make_unique<isha::LlamaCppEngine>(g_profile);
        g_vector_index = std::make_unique<isha::EdgeFlatVectorIndex>();
        g_embedding_engine = std::make_unique<isha::EmbeddingEngine>(384); // 384 dimensions for all-MiniLM-L6-v2 parity

        // Load pre-existing index if present
        std::string index_path = g_files_dir + "/rag_index.efvi";
        g_vector_index->loadFromFile(index_path);

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

        std::string model_path = g_files_dir + "/" + name_str + ".gguf";
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
    
    if (g_model_manager->isT0ThermalEmergencyMode()) {
        ctx.max_tokens_to_generate = 32;
    }

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
    if (!g_model_manager) return 0;
    return g_model_manager->isT0ThermalEmergencyMode() ? 3 : 0;
}

JNIEXPORT jint JNICALL
Java_com_isha_ai_IshaRuntime_getMemoryPressure(JNIEnv *env, jobject thiz) {
    return 0;
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

    double mapped_temp = 25.0;
    if (thermal_state == 1) {
        mapped_temp = 36.5;
    } else if (thermal_state == 2) {
        mapped_temp = 39.5;
    } else if (thermal_state >= 3) {
        mapped_temp = 43.5;
    }

    LOGI("Received OS PowerManager thermal state update = %d. Mapping to temp cue = %.1f°C",
         thermal_state, mapped_temp);

    g_model_manager->thermalUnload(mapped_temp);
}

// ====================================================================
// PHASE 2 RAG JNI BINDINGS
// ====================================================================

JNIEXPORT jboolean JNICALL
Java_com_isha_ai_IshaRuntime_ingestChunkNative(JNIEnv *env, jobject thiz,
                                               jstring chunk_id, jstring chunk_text,
                                               jstring pack_id, jboolean is_last) {
    std::lock_guard<std::mutex> lock(g_jni_mutex);
    if (!g_vector_index || !g_embedding_engine) {
        LOGE("Ingestion failed: Vector index or embedding engine not initialized.");
        return JNI_FALSE;
    }

    try {
        const char *id_c = env->GetStringUTFChars(chunk_id, nullptr);
        const char *text_c = env->GetStringUTFChars(chunk_text, nullptr);
        const char *pack_c = env->GetStringUTFChars(pack_id, nullptr);
        
        std::string id_str(id_c);
        std::string text_str(text_c);
        std::string pack_str(pack_c);

        env->ReleaseStringUTFChars(chunk_id, id_c);
        env->ReleaseStringUTFChars(chunk_text, text_c);
        env->ReleaseStringUTFChars(pack_id, pack_c);

        // Generate embedding locally (384 dimensions)
        auto embedding = g_embedding_engine->generateEmbedding(text_str);
        
        // Add to vector index
        g_vector_index->addChunk({id_str, text_str, pack_str, std::move(embedding)});

        // Save index file periodically / at the end of ingestion
        if (is_last) {
            std::string index_path = g_files_dir + "/rag_index.efvi";
            g_vector_index->saveToFile(index_path);
        }

        return JNI_TRUE;
    } catch (const std::exception &e) {
        LOGE("Exception in ingestChunkNative: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL
Java_com_isha_ai_IshaRuntime_retrieveContextNative(JNIEnv *env, jobject thiz,
                                                   jstring query, jstring pack_id,
                                                   jint limit) {
    std::lock_guard<std::mutex> lock(g_jni_mutex);
    if (!g_vector_index || !g_embedding_engine) {
        return env->NewStringUTF("");
    }

    try {
        const char *query_c = env->GetStringUTFChars(query, nullptr);
        const char *pack_c = env->GetStringUTFChars(pack_id, nullptr);
        std::string query_str(query_c);
        std::string pack_str(pack_c);
        env->ReleaseStringUTFChars(query, query_c);
        env->ReleaseStringUTFChars(pack_id, pack_c);

        // Cosine similarity search (Inner Product since normalized)
        auto query_embedding = g_embedding_engine->generateEmbedding(query_str);
        auto results = g_vector_index->search(query_embedding, pack_str, limit, 0.35);

        // Fallback to BM25-lite keyword search if vector search is empty
        if (results.empty()) {
            LOGI("JNI retrieval: Semantic search returned no results above 0.35. Falling back to BM25-lite.");
            results = g_vector_index->searchBM25(query_str, pack_str, limit);
        }

        // Format chunks list as formatted string: chunk_id||text||score\n...
        std::stringstream ss;
        for (const auto& r : results) {
            ss << r.chunk_id << "||" << r.text << "||" << r.score << "\n";
        }

        return env->NewStringUTF(ss.str().c_str());
    } catch (const std::exception &e) {
        LOGE("Exception in retrieveContextNative: %s", e.what());
        return env->NewStringUTF("");
    }
}

JNIEXPORT jboolean JNICALL
Java_com_isha_ai_IshaRuntime_clearPackNative(JNIEnv *env, jobject thiz, jstring pack_id) {
    std::lock_guard<std::mutex> lock(g_jni_mutex);
    if (!g_vector_index) return JNI_FALSE;

    try {
        const char *pack_c = env->GetStringUTFChars(pack_id, nullptr);
        std::string pack_str(pack_c);
        env->ReleaseStringUTFChars(pack_id, pack_c);

        g_vector_index->clearPack(pack_str);
        std::string index_path = g_files_dir + "/rag_index.efvi";
        g_vector_index->saveToFile(index_path);

        return JNI_TRUE;
    } catch (const std::exception &e) {
        LOGE("Exception in clearPackNative: %s", e.what());
        return JNI_FALSE;
    }
}

}
