cmake_minimum_required(VERSION 3.22.1)
project(isha_jni C CXX)

# Force compiler optimization flags for mobile execution performance (prevent Debug slowdown)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -fno-finite-math-only -DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fno-finite-math-only -DNDEBUG")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../..")

# All core sources relative to the root workspace
set(CORE_SOURCES
    "${ROOT_DIR}/core/logging/logger.cpp"
    "${ROOT_DIR}/core/runtime/event_bus.cpp"
    "${ROOT_DIR}/core/runtime/runtime.cpp"
    "${ROOT_DIR}/core/model/memory_guard.cpp"
    "${ROOT_DIR}/core/model/model_manager.cpp"
    "${ROOT_DIR}/core/session/session_manager.cpp"
    "${ROOT_DIR}/core/retrieval/retrieval_stub.cpp"
    "${ROOT_DIR}/core/inference/gguf_loader.cpp"
    "${ROOT_DIR}/core/inference/gguf_inference_engine.cpp"
    "${ROOT_DIR}/core/inference/mmap_manager.cpp"
    "${ROOT_DIR}/core/inference/token_streamer.cpp"
    "${ROOT_DIR}/core/inference/kv_cache_manager.cpp"
    "${ROOT_DIR}/core/scheduler/inference_scheduler.cpp"
    "${ROOT_DIR}/core/embeddings/embedding_stub.cpp"
    "${ROOT_DIR}/core/ingestion/chunker.cpp"
    "${ROOT_DIR}/core/ingestion/ingestion_pipeline.cpp"
    "${ROOT_DIR}/core/embeddings/embedding_engine.cpp"
    "${ROOT_DIR}/core/retrieval/local_index.cpp"
    "${ROOT_DIR}/core/retrieval/retrieval_engine.cpp"
    "${ROOT_DIR}/core/cognition/context_governor.cpp"
    "${ROOT_DIR}/core/cognition/grounded_pipeline.cpp"
    "${ROOT_DIR}/core/watchdog/runtime_watchdog.cpp"
    "${ROOT_DIR}/core/observability/telemetry.cpp"
    "${ROOT_DIR}/core/voice/audio_buffer_manager.cpp"
    "${ROOT_DIR}/core/voice/audio_session_manager.cpp"
    "${ROOT_DIR}/core/voice/stt_engine.cpp"
    "${ROOT_DIR}/core/voice/tts_engine.cpp"
    "${ROOT_DIR}/core/voice/voice_orchestrator.cpp"
    "${ROOT_DIR}/core/multimodal/image_context.cpp"
    "${ROOT_DIR}/core/multimodal/image_decoder.cpp"
    "${ROOT_DIR}/core/multimodal/image_pipeline.cpp"
    "${ROOT_DIR}/core/multimodal/multimodal_context_governor.cpp"
    "${ROOT_DIR}/core/ocr/ocr_engine.cpp"
    "${ROOT_DIR}/core/vision/vision_engine.cpp"
    "${ROOT_DIR}/core/vision/frame_governor.cpp"
    "${ROOT_DIR}/core/vision/camera_session_manager.cpp"
    "${ROOT_DIR}/core/model/model_registry.cpp"
    "${ROOT_DIR}/core/model/tokenizer_manager.cpp"
    "${ROOT_DIR}/core/model/model_lifecycle_controller.cpp"
    "${ROOT_DIR}/core/inference/decode_governor.cpp"
    "${ROOT_DIR}/core/inference/output_sanitizer.cpp"
    "${ROOT_DIR}/core/inference/latency_tracker.cpp"
    "${ROOT_DIR}/core/inference/llama_cpp_engine.cpp"
    "${ROOT_DIR}/core/inference/acceleration_probe.cpp"
    "${ROOT_DIR}/core/inference/nnapi_backend.cpp"
    "${ROOT_DIR}/core/inference/ios_coreml_manager.cpp"
    "${ROOT_DIR}/core/model/download_manager.cpp"
    "${ROOT_DIR}/core/survival/runtime_state_versioning.cpp"
    "${ROOT_DIR}/core/survival/cache_quarantine_manager.cpp"
    "${ROOT_DIR}/core/survival/atomic_model_extraction.cpp"
    "${ROOT_DIR}/core/survival/install_recovery_manager.cpp"
    "${ROOT_DIR}/core/survival/storage_speed_classifier.cpp"
    "${ROOT_DIR}/core/survival/startup_strategy_manager.cpp"
    "${ROOT_DIR}/core/survival/background_survival_manager.cpp"
    "${ROOT_DIR}/core/survival/survival_priority_mode.cpp"
    "${ROOT_DIR}/core/survival/local_data_integrity_manager.cpp"
    "${ROOT_DIR}/core/survival/battery_floor_policy.cpp"
    "${ROOT_DIR}/core/survival/storage_floor_policy.cpp"
    "${ROOT_DIR}/core/survival/write_budget_manager.cpp"
    "${ROOT_DIR}/core/survival/field_failure_escalation.cpp"
    "${ROOT_DIR}/core/survival/device_certification_matrix.cpp"
    "${ROOT_DIR}/core/survival/update_safety_window.cpp"
    "${ROOT_DIR}/core/survival/foreground_service_policy.cpp"
    "${ROOT_DIR}/core/survival/dynamic_degradation_ladder.cpp"
    "${ROOT_DIR}/core/survival/runtime_health_snapshot.cpp"
    "${ROOT_DIR}/core/survival/model_integrity_fastscan.cpp"
    "${ROOT_DIR}/core/survival/recovery_ux_manager.cpp"
    "${ROOT_DIR}/core/survival/thermal_recovery_window.cpp"
    "${ROOT_DIR}/core/survival/background_checkpoint_priority.cpp"
    "${ROOT_DIR}/core/survival/hard_recovery_mode.cpp"
    "${ROOT_DIR}/core/survival/storage_recovery_assistant.cpp"
    "${ROOT_DIR}/core/survival/device_thermal_ceiling_policy.cpp"
    "${ROOT_DIR}/core/survival/deployment_health_summary.cpp"
    "${ROOT_DIR}/core/survival/first_token_consistency_policy.cpp"
    "${ROOT_DIR}/core/survival/ui_starvation_guard.cpp"
    "${ROOT_DIR}/core/survival/model_recommendation_policy.cpp"
    "${ROOT_DIR}/core/survival/recovery_notification_policy.cpp"
    "${ROOT_DIR}/core/survival/memory_drift_trend_guard.cpp"
    "${ROOT_DIR}/core/survival/install_time_device_profiler.cpp"
    "${ROOT_DIR}/core/survival/background_resume_latency_policy.cpp"
    "${ROOT_DIR}/core/survival/interruption_resilience_policy.cpp"
    "${ROOT_DIR}/core/survival/total_storage_governance.cpp"
    "${ROOT_DIR}/core/survival/telemetry_export_manager.cpp"
    "${ROOT_DIR}/core/survival/apk_update_migration_validation.cpp"
    "${ROOT_DIR}/core/survival/battery_lifetime_observer.cpp"
    "${ROOT_DIR}/core/survival/install_failure_recovery.cpp"
    "${ROOT_DIR}/core/survival/session_durability_validation.cpp"
    "${ROOT_DIR}/core/survival/first_week_retention_validation.cpp"
    "${ROOT_DIR}/core/survival/storage_fragmentation_guard.cpp"
    "${ROOT_DIR}/core/survival/oem_update_regression_policy.cpp"
    "${ROOT_DIR}/core/survival/reinstall_recovery_policy.cpp"
    "${ROOT_DIR}/core/monitoring/android_monitor.cpp"
)

# Compile llama.cpp library
set(LLAMA_DIR "${ROOT_DIR}/third_party/llama.cpp")

set(LLAMA_SOURCES
    "${LLAMA_DIR}/src/llama.cpp"
    "${LLAMA_DIR}/src/llama-adapter.cpp"
    "${LLAMA_DIR}/src/llama-arch.cpp"
    "${LLAMA_DIR}/src/llama-batch.cpp"
    "${LLAMA_DIR}/src/llama-chat.cpp"
    "${LLAMA_DIR}/src/llama-context.cpp"
    "${LLAMA_DIR}/src/llama-cparams.cpp"
    "${LLAMA_DIR}/src/llama-grammar.cpp"
    "${LLAMA_DIR}/src/llama-graph.cpp"
    "${LLAMA_DIR}/src/llama-hparams.cpp"
    "${LLAMA_DIR}/src/llama-impl.cpp"
    "${LLAMA_DIR}/src/llama-io.cpp"
    "${LLAMA_DIR}/src/llama-kv-cache.cpp"
    "${LLAMA_DIR}/src/llama-memory.cpp"
    "${LLAMA_DIR}/src/llama-mmap.cpp"
    "${LLAMA_DIR}/src/llama-model-loader.cpp"
    "${LLAMA_DIR}/src/llama-model.cpp"
    "${LLAMA_DIR}/src/llama-quant.cpp"
    "${LLAMA_DIR}/src/llama-sampling.cpp"
    "${LLAMA_DIR}/src/llama-vocab.cpp"
    "${LLAMA_DIR}/src/unicode.cpp"
    "${LLAMA_DIR}/src/unicode-data.cpp"

    "${LLAMA_DIR}/ggml/src/ggml-backend.cpp"
    "${LLAMA_DIR}/ggml/src/ggml-backend-reg.cpp"
    "${LLAMA_DIR}/ggml/src/ggml-opt.cpp"
    "${LLAMA_DIR}/ggml/src/ggml-threading.cpp"
    "${LLAMA_DIR}/ggml/src/gguf.cpp"

    "${LLAMA_DIR}/ggml/src/ggml-cpu/ggml-cpu-traits.cpp"
    "${LLAMA_DIR}/ggml/src/ggml-cpu/ggml-cpu-aarch64.cpp"
    "${LLAMA_DIR}/ggml/src/ggml-cpu/cpu-feats-x86.cpp"
    "${LLAMA_DIR}/ggml/src/ggml-cpu/ggml-cpu.cpp"

    "${LLAMA_DIR}/common/common.cpp"
    "${LLAMA_DIR}/common/sampling.cpp"
    "${LLAMA_DIR}/common/log.cpp"

    "${LLAMA_DIR}/ggml/src/ggml.c"
    "${LLAMA_DIR}/ggml/src/ggml-alloc.c"
    "${LLAMA_DIR}/ggml/src/ggml-quants.c"
    "${LLAMA_DIR}/ggml/src/ggml-cpu/ggml-cpu.c"
    "${LLAMA_DIR}/ggml/src/ggml-cpu/ggml-cpu-quants.c"
)

add_library(llama_static STATIC
    ${LLAMA_SOURCES}
)

target_compile_definitions(llama_static PUBLIC
    GGML_USE_CPU
)

target_include_directories(llama_static PRIVATE
    "${LLAMA_DIR}/include"
    "${LLAMA_DIR}/src"
    "${LLAMA_DIR}/ggml/include"
    "${LLAMA_DIR}/ggml/src"
    "${LLAMA_DIR}/ggml/src/ggml-cpu"
    "${LLAMA_DIR}/common"
)

# Compile JNI bridge and links everything
add_library(isha_jni SHARED
    isha_jni.cpp
    ${CORE_SOURCES}
)

# Include directories
target_include_directories(isha_jni PRIVATE
    "${ROOT_DIR}/core"
    "${ROOT_DIR}/third_party/llama.cpp/include"
    "${ROOT_DIR}/third_party/llama.cpp/common"
    "${ROOT_DIR}/third_party/llama.cpp/ggml/include"
)

# Link standard NDK libraries
target_link_libraries(isha_jni PRIVATE
    log
    android
    llama_static
)
