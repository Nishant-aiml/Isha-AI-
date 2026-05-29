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
    core/inference/acceleration_probe.cpp
    core/inference/nnapi_backend.cpp
    core/inference/ios_coreml_manager.cpp

    # Phase 6 Download Manager
    core/model/download_manager.cpp

    # Phase 10 Survival Subsystem
    core/survival/runtime_state_versioning.cpp
    core/survival/cache_quarantine_manager.cpp
    core/survival/atomic_model_extraction.cpp
    core/survival/install_recovery_manager.cpp
    core/survival/storage_speed_classifier.cpp
    core/survival/startup_strategy_manager.cpp
    core/survival/background_survival_manager.cpp
    core/survival/survival_priority_mode.cpp
    core/survival/local_data_integrity_manager.cpp

    # Phase 10A Survival Subsystem
    core/survival/battery_floor_policy.cpp
    core/survival/storage_floor_policy.cpp
    core/survival/write_budget_manager.cpp
    core/survival/field_failure_escalation.cpp
    core/survival/device_certification_matrix.cpp
    core/survival/update_safety_window.cpp
    core/survival/foreground_service_policy.cpp
    core/survival/dynamic_degradation_ladder.cpp
    core/survival/runtime_health_snapshot.cpp
    core/survival/model_integrity_fastscan.cpp
    core/survival/recovery_ux_manager.cpp

    # Phase 10B Survival Subsystem
    core/survival/thermal_recovery_window.cpp
    core/survival/background_checkpoint_priority.cpp
    core/survival/hard_recovery_mode.cpp
    core/survival/storage_recovery_assistant.cpp
    core/survival/device_thermal_ceiling_policy.cpp
    core/survival/deployment_health_summary.cpp

    # Phase 10C Survival Subsystem
    core/survival/first_token_consistency_policy.cpp
    core/survival/ui_starvation_guard.cpp
    core/survival/model_recommendation_policy.cpp
    core/survival/recovery_notification_policy.cpp
    core/survival/memory_drift_trend_guard.cpp

    # Phase 10C Patch Survival Subsystem
    core/survival/install_time_device_profiler.cpp
    core/survival/background_resume_latency_policy.cpp
    core/survival/interruption_resilience_policy.cpp
    core/survival/total_storage_governance.cpp
    core/survival/telemetry_export_manager.cpp

    # Phase 11A Survival Subsystem
    core/survival/apk_update_migration_validation.cpp
    core/survival/battery_lifetime_observer.cpp
    core/survival/install_failure_recovery.cpp
    core/survival/session_durability_validation.cpp
    core/survival/first_week_retention_validation.cpp
    core/survival/storage_fragmentation_guard.cpp
    core/survival/oem_update_regression_policy.cpp
    core/survival/reinstall_recovery_policy.cpp
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

add_executable(ios_coreml_manager_benchmark benchmarks/ios_coreml_manager_benchmark.cpp)
target_link_libraries(ios_coreml_manager_benchmark PRIVATE isha_core)

add_executable(llm_integration_benchmark benchmarks/llm_integration_benchmark.cpp)
target_link_libraries(llm_integration_benchmark PRIVATE isha_core)

# Phase 8 Benchmarks
add_executable(quarantine_adaptive_benchmark benchmarks/quarantine_adaptive_benchmark.cpp)
target_link_libraries(quarantine_adaptive_benchmark PRIVATE isha_core)

add_executable(watchdog_cooperative_benchmark benchmarks/watchdog_cooperative_benchmark.cpp)
target_link_libraries(watchdog_cooperative_benchmark PRIVATE isha_core)

add_executable(telemetry_normalization_benchmark benchmarks/telemetry_normalization_benchmark.cpp)
target_link_libraries(telemetry_normalization_benchmark PRIVATE isha_core)

add_executable(thermal_cycling_soak_benchmark benchmarks/thermal_cycling_soak_benchmark.cpp)
target_link_libraries(thermal_cycling_soak_benchmark PRIVATE isha_core)

# Phase 8A NNAPI Benchmarks
add_executable(nnapi_partitioning_benchmark benchmarks/nnapi_partitioning_benchmark.cpp)
target_link_libraries(nnapi_partitioning_benchmark PRIVATE isha_core)

add_executable(nnapi_capability_score_benchmark benchmarks/nnapi_capability_score_benchmark.cpp)
target_link_libraries(nnapi_capability_score_benchmark PRIVATE isha_core)

add_executable(nnapi_rejection_benchmark benchmarks/nnapi_rejection_benchmark.cpp)
target_link_libraries(nnapi_rejection_benchmark PRIVATE isha_core)

add_executable(nnapi_transfer_overhead_benchmark benchmarks/nnapi_transfer_overhead_benchmark.cpp)
target_link_libraries(nnapi_transfer_overhead_benchmark PRIVATE isha_core)

add_executable(nnapi_prefill_decode_split_benchmark benchmarks/nnapi_prefill_decode_split_benchmark.cpp)
target_link_libraries(nnapi_prefill_decode_split_benchmark PRIVATE isha_core)

add_executable(nnapi_residency_benchmark benchmarks/nnapi_residency_benchmark.cpp)
target_link_libraries(nnapi_residency_benchmark PRIVATE isha_core)

add_executable(nnapi_probe_timeout_benchmark benchmarks/nnapi_probe_timeout_benchmark.cpp)
target_link_libraries(nnapi_probe_timeout_benchmark PRIVATE isha_core)

add_executable(nnapi_startup_latency_benchmark benchmarks/nnapi_startup_latency_benchmark.cpp)
target_link_libraries(nnapi_startup_latency_benchmark PRIVATE isha_core)

# Phase 8B Benchmarks
add_executable(thermal_hysteresis_benchmark benchmarks/thermal_hysteresis_benchmark.cpp)
target_link_libraries(thermal_hysteresis_benchmark PRIVATE isha_core)

add_executable(lmk_integrity_benchmark benchmarks/lmk_integrity_benchmark.cpp)
target_link_libraries(lmk_integrity_benchmark PRIVATE isha_core)

add_executable(recovery_quality_drift_benchmark benchmarks/recovery_quality_drift_benchmark.cpp)
target_link_libraries(recovery_quality_drift_benchmark PRIVATE isha_core)

add_executable(page_fault_burst_benchmark benchmarks/page_fault_burst_benchmark.cpp)
target_link_libraries(page_fault_burst_benchmark PRIVATE isha_core)

add_executable(throughput_half_life_benchmark benchmarks/throughput_half_life_benchmark.cpp)
target_link_libraries(throughput_half_life_benchmark PRIVATE isha_core)

add_executable(startup_variance_benchmark benchmarks/startup_variance_benchmark.cpp)
target_link_libraries(startup_variance_benchmark PRIVATE isha_core)

add_executable(randomized_fallback_storm_benchmark benchmarks/randomized_fallback_storm_benchmark.cpp)
target_link_libraries(randomized_fallback_storm_benchmark PRIVATE isha_core)

add_executable(device_memory_collapse_benchmark benchmarks/device_memory_collapse_benchmark.cpp)
target_link_libraries(device_memory_collapse_benchmark PRIVATE isha_core)

add_executable(efficiency_decay_curve_benchmark benchmarks/efficiency_decay_curve_benchmark.cpp)
target_link_libraries(efficiency_decay_curve_benchmark PRIVATE isha_core)

# Phase 9 Benchmarks
add_executable(apk_size_governance_benchmark benchmarks/apk_size_governance_benchmark.cpp)
target_link_libraries(apk_size_governance_benchmark PRIVATE isha_core)

add_executable(download_interruption_chaos_benchmark benchmarks/download_interruption_chaos_benchmark.cpp)
target_link_libraries(download_interruption_chaos_benchmark PRIVATE isha_core)

add_executable(rapid_model_switch_benchmark benchmarks/rapid_model_switch_benchmark.cpp)
target_link_libraries(rapid_model_switch_benchmark PRIVATE isha_core)

add_executable(install_update_recovery_benchmark benchmarks/install_update_recovery_benchmark.cpp)
target_link_libraries(install_update_recovery_benchmark PRIVATE isha_core)

add_executable(session_recovery_benchmark benchmarks/session_recovery_benchmark.cpp)
target_link_libraries(session_recovery_benchmark PRIVATE isha_core)

add_executable(permission_chaos_benchmark benchmarks/permission_chaos_benchmark.cpp)
target_link_libraries(permission_chaos_benchmark PRIVATE isha_core)

# Phase 10 Benchmarks
add_executable(survival_benchmark benchmarks/survival_benchmark.cpp)
target_link_libraries(survival_benchmark PRIVATE isha_core)

add_executable(survival_a_benchmark benchmarks/survival_a_benchmark.cpp)
target_link_libraries(survival_a_benchmark PRIVATE isha_core)

# Phase 10B Benchmarks
add_executable(thermal_recovery_window_benchmark benchmarks/thermal_recovery_window_benchmark.cpp)
target_link_libraries(thermal_recovery_window_benchmark PRIVATE isha_core)

add_executable(background_checkpoint_priority_benchmark benchmarks/background_checkpoint_priority_benchmark.cpp)
target_link_libraries(background_checkpoint_priority_benchmark PRIVATE isha_core)

add_executable(hard_recovery_mode_benchmark benchmarks/hard_recovery_mode_benchmark.cpp)
target_link_libraries(hard_recovery_mode_benchmark PRIVATE isha_core)

add_executable(storage_recovery_assistant_benchmark benchmarks/storage_recovery_assistant_benchmark.cpp)
target_link_libraries(storage_recovery_assistant_benchmark PRIVATE isha_core)

add_executable(device_thermal_ceiling_benchmark benchmarks/device_thermal_ceiling_benchmark.cpp)
target_link_libraries(device_thermal_ceiling_benchmark PRIVATE isha_core)

add_executable(deployment_health_summary_benchmark benchmarks/deployment_health_summary_benchmark.cpp)
target_link_libraries(deployment_health_summary_benchmark PRIVATE isha_core)

add_executable(uninstall_risk_policy_benchmark benchmarks/uninstall_risk_policy_benchmark.cpp)
target_link_libraries(uninstall_risk_policy_benchmark PRIVATE isha_core)

# Phase 10C Benchmarks
add_executable(first_token_consistency_benchmark benchmarks/first_token_consistency_benchmark.cpp)
target_link_libraries(first_token_consistency_benchmark PRIVATE isha_core)

add_executable(ui_starvation_guard_benchmark benchmarks/ui_starvation_guard_benchmark.cpp)
target_link_libraries(ui_starvation_guard_benchmark PRIVATE isha_core)

add_executable(model_recommendation_policy_benchmark benchmarks/model_recommendation_policy_benchmark.cpp)
target_link_libraries(model_recommendation_policy_benchmark PRIVATE isha_core)

add_executable(recovery_notification_policy_benchmark benchmarks/recovery_notification_policy_benchmark.cpp)
target_link_libraries(recovery_notification_policy_benchmark PRIVATE isha_core)

add_executable(memory_drift_trend_guard_benchmark benchmarks/memory_drift_trend_guard_benchmark.cpp)
target_link_libraries(memory_drift_trend_guard_benchmark PRIVATE isha_core)

add_executable(apk_size_budget_policy_benchmark benchmarks/apk_size_budget_policy_benchmark.cpp)
target_link_libraries(apk_size_budget_policy_benchmark PRIVATE isha_core)

# Phase 10C Patch Benchmarks
add_executable(install_time_device_profiler_benchmark benchmarks/install_time_device_profiler_benchmark.cpp)
target_link_libraries(install_time_device_profiler_benchmark PRIVATE isha_core)

add_executable(first_run_experience_policy_benchmark benchmarks/first_run_experience_policy_benchmark.cpp)
target_link_libraries(first_run_experience_policy_benchmark PRIVATE isha_core)

add_executable(background_resume_latency_benchmark benchmarks/background_resume_latency_benchmark.cpp)
target_link_libraries(background_resume_latency_benchmark PRIVATE isha_core)

add_executable(interruption_resilience_policy_benchmark benchmarks/interruption_resilience_policy_benchmark.cpp)
target_link_libraries(interruption_resilience_policy_benchmark PRIVATE isha_core)

add_executable(total_storage_governance_benchmark benchmarks/total_storage_governance_benchmark.cpp)
target_link_libraries(total_storage_governance_benchmark PRIVATE isha_core)

add_executable(playstore_readiness_policy_benchmark benchmarks/playstore_readiness_policy_benchmark.cpp)
target_link_libraries(playstore_readiness_policy_benchmark PRIVATE isha_core)

add_executable(telemetry_export_benchmark benchmarks/telemetry_export_benchmark.cpp)
target_link_libraries(telemetry_export_benchmark PRIVATE isha_core)

add_executable(apk_update_migration_validation_benchmark benchmarks/apk_update_migration_validation_benchmark.cpp)
target_link_libraries(apk_update_migration_validation_benchmark PRIVATE isha_core)

add_executable(battery_lifetime_observer_benchmark benchmarks/battery_lifetime_observer_benchmark.cpp)
target_link_libraries(battery_lifetime_observer_benchmark PRIVATE isha_core)

add_executable(install_failure_recovery_benchmark benchmarks/install_failure_recovery_benchmark.cpp)
target_link_libraries(install_failure_recovery_benchmark PRIVATE isha_core)

add_executable(consumer_expectation_policy_benchmark benchmarks/consumer_expectation_policy_benchmark.cpp)
target_link_libraries(consumer_expectation_policy_benchmark PRIVATE isha_core)

add_executable(session_durability_validation_benchmark benchmarks/session_durability_validation_benchmark.cpp)
target_link_libraries(session_durability_validation_benchmark PRIVATE isha_core)

add_executable(ship_blocker_matrix_benchmark benchmarks/ship_blocker_matrix_benchmark.cpp)
target_link_libraries(ship_blocker_matrix_benchmark PRIVATE isha_core)

add_executable(first_week_retention_validation_benchmark benchmarks/first_week_retention_validation_benchmark.cpp)
target_link_libraries(first_week_retention_validation_benchmark PRIVATE isha_core)

add_executable(storage_fragmentation_guard_benchmark benchmarks/storage_fragmentation_guard_benchmark.cpp)
target_link_libraries(storage_fragmentation_guard_benchmark PRIVATE isha_core)

add_executable(oem_update_regression_policy_benchmark benchmarks/oem_update_regression_policy_benchmark.cpp)
target_link_libraries(oem_update_regression_policy_benchmark PRIVATE isha_core)

add_executable(reinstall_recovery_policy_benchmark benchmarks/reinstall_recovery_policy_benchmark.cpp)
target_link_libraries(reinstall_recovery_policy_benchmark PRIVATE isha_core)



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
add_test(NAME TestIosCoremlManager COMMAND ios_coreml_manager_benchmark)
add_test(NAME TestQuarantineAdaptive COMMAND quarantine_adaptive_benchmark)
add_test(NAME TestWatchdogCooperative COMMAND watchdog_cooperative_benchmark)
add_test(NAME TestTelemetryNormalization COMMAND telemetry_normalization_benchmark)
add_test(NAME TestThermalCyclingSoak COMMAND thermal_cycling_soak_benchmark)
add_test(NAME TestNnapiPartitioning COMMAND nnapi_partitioning_benchmark)
add_test(NAME TestNnapiCapabilityScore COMMAND nnapi_capability_score_benchmark)
add_test(NAME TestNnapiRejection COMMAND nnapi_rejection_benchmark)
add_test(NAME TestNnapiTransferOverhead COMMAND nnapi_transfer_overhead_benchmark)
add_test(NAME TestNnapiPrefillDecodeSplit COMMAND nnapi_prefill_decode_split_benchmark)
add_test(NAME TestNnapiResidency COMMAND nnapi_residency_benchmark)
add_test(NAME TestNnapiProbeTimeout COMMAND nnapi_probe_timeout_benchmark)
add_test(NAME TestNnapiStartupLatency COMMAND nnapi_startup_latency_benchmark)
add_test(NAME TestThermalHysteresis COMMAND thermal_hysteresis_benchmark)
add_test(NAME TestLmkIntegrity COMMAND lmk_integrity_benchmark)
add_test(NAME TestRecoveryQualityDrift COMMAND recovery_quality_drift_benchmark)
add_test(NAME TestPageFaultBurst COMMAND page_fault_burst_benchmark)
add_test(NAME TestThroughputHalfLife COMMAND throughput_half_life_benchmark)
add_test(NAME TestStartupVariance COMMAND startup_variance_benchmark)
add_test(NAME TestRandomizedFallbackStorm COMMAND randomized_fallback_storm_benchmark)
add_test(NAME TestDeviceMemoryCollapse COMMAND device_memory_collapse_benchmark)
add_test(NAME TestEfficiencyDecayCurve COMMAND efficiency_decay_curve_benchmark)

# Phase 9 Tests
add_test(NAME TestApkSize COMMAND apk_size_governance_benchmark)
add_test(NAME TestDownloadInterruption COMMAND download_interruption_chaos_benchmark)
add_test(NAME TestRapidModelSwitch COMMAND rapid_model_switch_benchmark)
add_test(NAME TestInstallUpdateRecovery COMMAND install_update_recovery_benchmark)
add_test(NAME TestSessionRecovery COMMAND session_recovery_benchmark)
add_test(NAME TestPermissionChaos COMMAND permission_chaos_benchmark)

# Phase 10 Tests
add_test(NAME TestSurvival COMMAND survival_benchmark)
add_test(NAME TestSurvivalA COMMAND survival_a_benchmark)

# Phase 10B Tests
add_test(NAME TestThermalRecoveryWindow COMMAND thermal_recovery_window_benchmark)
add_test(NAME TestBackgroundCheckpointPriority COMMAND background_checkpoint_priority_benchmark)
add_test(NAME TestHardRecoveryMode COMMAND hard_recovery_mode_benchmark)
add_test(NAME TestStorageRecoveryAssistant COMMAND storage_recovery_assistant_benchmark)
add_test(NAME TestDeviceThermalCeiling COMMAND device_thermal_ceiling_benchmark)
add_test(NAME TestDeploymentHealthSummary COMMAND deployment_health_summary_benchmark)
add_test(NAME TestUninstallRiskPolicy COMMAND uninstall_risk_policy_benchmark)

# Phase 10C Tests
add_test(NAME TestFirstTokenConsistency COMMAND first_token_consistency_benchmark)
add_test(NAME TestUIStarvationGuard COMMAND ui_starvation_guard_benchmark)
add_test(NAME TestModelRecommendationPolicy COMMAND model_recommendation_policy_benchmark)
add_test(NAME TestRecoveryNotificationPolicy COMMAND recovery_notification_policy_benchmark)
add_test(NAME TestMemoryDriftTrendGuard COMMAND memory_drift_trend_guard_benchmark)
add_test(NAME TestApkSizeBudgetPolicy COMMAND apk_size_budget_policy_benchmark)

# Phase 10C Patch Tests
add_test(NAME TestInstallTimeDeviceProfiler COMMAND install_time_device_profiler_benchmark)
add_test(NAME TestFirstRunExperiencePolicy COMMAND first_run_experience_policy_benchmark)
add_test(NAME TestBackgroundResumeLatency COMMAND background_resume_latency_benchmark)
add_test(NAME TestInterruptionResiliencePolicy COMMAND interruption_resilience_policy_benchmark)
add_test(NAME TestTotalStorageGovernance COMMAND total_storage_governance_benchmark)
add_test(NAME TestPlayStoreReadinessPolicy COMMAND playstore_readiness_policy_benchmark)
add_test(NAME TestTelemetryExport COMMAND telemetry_export_benchmark)

# Phase 11A Tests
add_test(NAME TestApkUpdateMigration COMMAND apk_update_migration_validation_benchmark)
add_test(NAME TestBatteryLifetimeObserver COMMAND battery_lifetime_observer_benchmark)
add_test(NAME TestInstallFailureRecovery COMMAND install_failure_recovery_benchmark)
add_test(NAME TestConsumerExpectationPolicy COMMAND consumer_expectation_policy_benchmark)
add_test(NAME TestSessionDurabilityValidation COMMAND session_durability_validation_benchmark)
add_test(NAME TestShipBlockerMatrix COMMAND ship_blocker_matrix_benchmark)
add_test(NAME TestFirstWeekRetention COMMAND first_week_retention_validation_benchmark)
add_test(NAME TestStorageFragmentationGuard COMMAND storage_fragmentation_guard_benchmark)
add_test(NAME TestOemUpdateRegression COMMAND oem_update_regression_policy_benchmark)
add_test(NAME TestReinstallRecovery COMMAND reinstall_recovery_policy_benchmark)


