cmake_minimum_required(VERSION 3.15)
project(isha_ai CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files for the core library (Phase 1 + Phase 2)
set(CORE_SOURCES
    core/logging/logger.cpp
    core/runtime/event_bus.cpp
    core/runtime/runtime.cpp
    core/model/memory_guard.cpp
    core/model/model_manager.cpp
    core/session/session_manager.cpp
    core/retrieval/retrieval_stub.cpp
    
    # Phase 2 Inference
    core/inference/gguf_loader.cpp
    core/inference/gguf_inference_engine.cpp
    core/inference/mmap_manager.cpp
    core/inference/token_streamer.cpp
    core/inference/kv_cache_manager.cpp
    
    # Phase 2 Scheduler
    core/scheduler/inference_scheduler.cpp
    
    # Phase 2 Embeddings & Ingestion
    core/embeddings/embedding_stub.cpp
    core/ingestion/chunker.cpp
    core/ingestion/ingestion_pipeline.cpp

    # Phase 3 Real Cognitive Execution & Observability
    core/embeddings/embedding_engine.cpp
    core/retrieval/local_index.cpp
    core/retrieval/retrieval_engine.cpp
    core/cognition/context_governor.cpp
    core/cognition/grounded_pipeline.cpp
    core/watchdog/runtime_watchdog.cpp
    core/observability/telemetry.cpp

    # Phase 4 Voice Runtime
    core/voice/audio_buffer_manager.cpp
    core/voice/audio_session_manager.cpp
    core/voice/stt_engine.cpp
    core/voice/tts_engine.cpp
    core/voice/voice_orchestrator.cpp

    # Phase 5 Multimodal Core
    core/multimodal/image_context.cpp
    core/multimodal/image_decoder.cpp
    core/multimodal/image_pipeline.cpp
    core/multimodal/multimodal_context_governor.cpp

    # Phase 5 OCR & Vision
    core/ocr/ocr_engine.cpp
    core/vision/vision_engine.cpp
    core/vision/frame_governor.cpp
    core/vision/camera_session_manager.cpp

    # Phase 6 Model Governance
    core/model/model_registry.cpp
    core/model/tokenizer_manager.cpp
    core/model/model_lifecycle_controller.cpp

    # Phase 6 Inference Governance
    core/inference/decode_governor.cpp
    core/inference/output_sanitizer.cpp
    core/inference/latency_tracker.cpp
    core/inference/llama_cpp_engine.cpp

    # Phase 6 Download Manager
    core/model/download_manager.cpp
)

# Platform specific resource monitor selection
if(WIN32)
    list(APPEND CORE_SOURCES core/monitoring/windows_monitor.cpp)
else()
    list(APPEND CORE_SOURCES core/monitoring/android_monitor.cpp)
endif()

# Core static library
add_library(isha_core STATIC ${CORE_SOURCES})

# Include directories
target_include_directories(isha_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp/common
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp/ggml/include
)

# Phase 1 Benchmarks
add_executable(load_benchmark benchmarks/load_benchmark.cpp)
target_link_libraries(load_benchmark PRIVATE isha_core)

add_executable(memory_benchmark benchmarks/memory_benchmark.cpp)
target_link_libraries(memory_benchmark PRIVATE isha_core)

add_executable(session_benchmark benchmarks/session_benchmark.cpp)
target_link_libraries(session_benchmark PRIVATE isha_core)

add_executable(retrieval_stub_benchmark benchmarks/retrieval_stub_benchmark.cpp)
target_link_libraries(retrieval_stub_benchmark PRIVATE isha_core)

# Phase 2 Benchmarks
add_executable(mmap_benchmark benchmarks/mmap_benchmark.cpp)
target_link_libraries(mmap_benchmark PRIVATE isha_core)

add_executable(inference_benchmark benchmarks/inference_benchmark.cpp)
target_link_libraries(inference_benchmark PRIVATE isha_core)

add_executable(kv_cache_benchmark benchmarks/kv_cache_benchmark.cpp)
target_link_libraries(kv_cache_benchmark PRIVATE isha_core)

add_executable(streaming_benchmark benchmarks/streaming_benchmark.cpp)
target_link_libraries(streaming_benchmark PRIVATE isha_core)

add_executable(android_profile_benchmark benchmarks/android_profile_benchmark.cpp)
target_link_libraries(android_profile_benchmark PRIVATE isha_core)

# Phase 3 Benchmarks
add_executable(inference_stress_benchmark benchmarks/inference_stress_benchmark.cpp)
target_link_libraries(inference_stress_benchmark PRIVATE isha_core)

add_executable(context_budget_benchmark benchmarks/context_budget_benchmark.cpp)
target_link_libraries(context_budget_benchmark PRIVATE isha_core)

add_executable(retrieval_latency_benchmark benchmarks/retrieval_latency_benchmark.cpp)
target_link_libraries(retrieval_latency_benchmark PRIVATE isha_core)

add_executable(thermal_stress_benchmark benchmarks/thermal_stress_benchmark.cpp)
target_link_libraries(thermal_stress_benchmark PRIVATE isha_core)

add_executable(watchdog_recovery_benchmark benchmarks/watchdog_recovery_benchmark.cpp)
target_link_libraries(watchdog_recovery_benchmark PRIVATE isha_core)

# Phase 4 Benchmarks
add_executable(voice_latency_benchmark benchmarks/voice_latency_benchmark.cpp)
target_link_libraries(voice_latency_benchmark PRIVATE isha_core)

add_executable(voice_lifecycle_benchmark benchmarks/voice_lifecycle_benchmark.cpp)
target_link_libraries(voice_lifecycle_benchmark PRIVATE isha_core)

add_executable(realtime_hardening_benchmark benchmarks/realtime_hardening_benchmark.cpp)
target_link_libraries(realtime_hardening_benchmark PRIVATE isha_core)

# Phase 5 Benchmarks
add_executable(image_ingestion_benchmark benchmarks/image_ingestion_benchmark.cpp)
target_link_libraries(image_ingestion_benchmark PRIVATE isha_core)

add_executable(ocr_latency_benchmark benchmarks/ocr_latency_benchmark.cpp)
target_link_libraries(ocr_latency_benchmark PRIVATE isha_core)

add_executable(multimodal_scheduler_benchmark benchmarks/multimodal_scheduler_benchmark.cpp)
target_link_libraries(multimodal_scheduler_benchmark PRIVATE isha_core)

add_executable(frame_pressure_benchmark benchmarks/frame_pressure_benchmark.cpp)
target_link_libraries(frame_pressure_benchmark PRIVATE isha_core)

add_executable(thermal_vision_stress_benchmark benchmarks/thermal_vision_stress_benchmark.cpp)
target_link_libraries(thermal_vision_stress_benchmark PRIVATE isha_core)

add_executable(camera_lifecycle_benchmark benchmarks/camera_lifecycle_benchmark.cpp)
target_link_libraries(camera_lifecycle_benchmark PRIVATE isha_core)

# Phase 6 Benchmarks
add_executable(llm_lifecycle_benchmark benchmarks/llm_lifecycle_benchmark.cpp)
target_link_libraries(llm_lifecycle_benchmark PRIVATE isha_core)

add_executable(llm_integration_benchmark benchmarks/llm_integration_benchmark.cpp)
target_link_libraries(llm_integration_benchmark PRIVATE isha_core)

# Enable CTest
enable_testing()
add_test(NAME TestLoad COMMAND load_benchmark)
add_test(NAME TestMemory COMMAND memory_benchmark)
add_test(NAME TestSession COMMAND session_benchmark)
add_test(NAME TestRetrieval COMMAND retrieval_stub_benchmark)
add_test(NAME TestMmap COMMAND mmap_benchmark)
add_test(NAME TestInference COMMAND inference_benchmark)
add_test(NAME TestKvCache COMMAND kv_cache_benchmark)
add_test(NAME TestStreaming COMMAND streaming_benchmark)
add_test(NAME TestAndroidProfile COMMAND android_profile_benchmark)
add_test(NAME TestInferenceStress COMMAND inference_stress_benchmark)
add_test(NAME TestContextBudget COMMAND context_budget_benchmark)
add_test(NAME TestRetrievalLatency COMMAND retrieval_latency_benchmark)
add_test(NAME TestThermalStress COMMAND thermal_stress_benchmark)
add_test(NAME TestWatchdogRecovery COMMAND watchdog_recovery_benchmark)
add_test(NAME TestVoiceLatency COMMAND voice_latency_benchmark)
add_test(NAME TestVoiceLifecycle COMMAND voice_lifecycle_benchmark)
add_test(NAME TestRealtimeHardening COMMAND realtime_hardening_benchmark)
add_test(NAME TestImageIngestion COMMAND image_ingestion_benchmark)
add_test(NAME TestOCRLatency COMMAND ocr_latency_benchmark)
add_test(NAME TestMultimodalScheduler COMMAND multimodal_scheduler_benchmark)
add_test(NAME TestFramePressure COMMAND frame_pressure_benchmark)
add_test(NAME TestThermalVisionStress COMMAND thermal_vision_stress_benchmark)
add_test(NAME TestCameraLifecycle COMMAND camera_lifecycle_benchmark)
add_test(NAME TestLLMLifecycle COMMAND llm_lifecycle_benchmark)

