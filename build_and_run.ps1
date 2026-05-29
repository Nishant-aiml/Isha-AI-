# Setup build variables using MSVC cl.exe
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

# Core sources - NO llama.cpp dependencies
$sources = "core/logging/logger.cpp core/runtime/event_bus.cpp core/runtime/runtime.cpp core/model/memory_guard.cpp core/model/model_manager.cpp core/session/session_manager.cpp core/retrieval/retrieval_stub.cpp core/monitoring/windows_monitor.cpp core/inference/gguf_loader.cpp core/inference/gguf_inference_engine.cpp core/inference/mmap_manager.cpp core/inference/token_streamer.cpp core/inference/kv_cache_manager.cpp core/scheduler/inference_scheduler.cpp core/embeddings/embedding_stub.cpp core/ingestion/chunker.cpp core/ingestion/ingestion_pipeline.cpp core/embeddings/embedding_engine.cpp core/retrieval/local_index.cpp core/retrieval/retrieval_engine.cpp core/cognition/context_governor.cpp core/cognition/grounded_pipeline.cpp core/watchdog/runtime_watchdog.cpp core/observability/telemetry.cpp core/voice/audio_buffer_manager.cpp core/voice/audio_session_manager.cpp core/voice/stt_engine.cpp core/voice/tts_engine.cpp core/voice/voice_orchestrator.cpp core/multimodal/image_context.cpp core/multimodal/image_decoder.cpp core/multimodal/image_pipeline.cpp core/multimodal/multimodal_context_governor.cpp core/ocr/ocr_engine.cpp core/vision/vision_engine.cpp core/vision/frame_governor.cpp core/vision/camera_session_manager.cpp core/model/model_registry.cpp core/model/tokenizer_manager.cpp core/model/model_lifecycle_controller.cpp core/inference/decode_governor.cpp core/inference/output_sanitizer.cpp core/inference/latency_tracker.cpp core/model/download_manager.cpp core/inference/acceleration_probe.cpp core/inference/nnapi_backend.cpp core/inference/ios_coreml_manager.cpp core/survival/runtime_state_versioning.cpp core/survival/cache_quarantine_manager.cpp core/survival/atomic_model_extraction.cpp core/survival/install_recovery_manager.cpp core/survival/storage_speed_classifier.cpp core/survival/startup_strategy_manager.cpp core/survival/background_survival_manager.cpp core/survival/survival_priority_mode.cpp core/survival/local_data_integrity_manager.cpp core/survival/battery_floor_policy.cpp core/survival/storage_floor_policy.cpp core/survival/write_budget_manager.cpp core/survival/field_failure_escalation.cpp core/survival/device_certification_matrix.cpp core/survival/update_safety_window.cpp core/survival/foreground_service_policy.cpp core/survival/dynamic_degradation_ladder.cpp core/survival/runtime_health_snapshot.cpp core/survival/model_integrity_fastscan.cpp core/survival/recovery_ux_manager.cpp core/survival/thermal_recovery_window.cpp core/survival/background_checkpoint_priority.cpp core/survival/hard_recovery_mode.cpp core/survival/storage_recovery_assistant.cpp core/survival/device_thermal_ceiling_policy.cpp core/survival/deployment_health_summary.cpp core/survival/first_token_consistency_policy.cpp core/survival/ui_starvation_guard.cpp core/survival/model_recommendation_policy.cpp core/survival/recovery_notification_policy.cpp core/survival/memory_drift_trend_guard.cpp core/survival/install_time_device_profiler.cpp core/survival/background_resume_latency_policy.cpp core/survival/interruption_resilience_policy.cpp core/survival/total_storage_governance.cpp core/survival/telemetry_export_manager.cpp core/survival/apk_update_migration_validation.cpp core/survival/battery_lifetime_observer.cpp core/survival/install_failure_recovery.cpp core/survival/session_durability_validation.cpp core/survival/first_week_retention_validation.cpp core/survival/storage_fragmentation_guard.cpp core/survival/oem_update_regression_policy.cpp core/survival/reinstall_recovery_policy.cpp"
$include = "/Icore"

# Llama-dependent sources (compiled separately)
$llama_sources = "core/inference/llama_cpp_engine.cpp"
$llama_include = "/Icore /Ithird_party/llama.cpp/include /Ithird_party/llama.cpp/common /Ithird_party/llama.cpp/ggml/include"
$llama_defines = "/DGGML_USE_CPU /D_CRT_SECURE_NO_WARNINGS /DNOMINMAX"
$llama_lib = "lib/llama.lib"

# Ensure output and temp directories exist
if (-not (Test-Path bin)) { New-Item -ItemType Directory -Path bin }
if (-not (Test-Path obj)) { New-Item -ItemType Directory -Path obj }
if (-not (Test-Path obj/llama_objs)) { New-Item -ItemType Directory -Path obj/llama_objs -Force }

# Clean old root-level and obj-level benchmark object files to prevent linker collisions
Get-ChildItem -Filter *.obj | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path obj -Filter *.obj | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem -Path obj/llama_objs -Filter *.obj -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue

# Step 1: Compile core library (no llama.cpp deps)
Write-Host "Compiling core library components into obj/..." -ForegroundColor Magenta
cmd.exe /c "call `"$vcvars`" && cl.exe /EHsc /std:c++17 $include /c $sources /Foobj/"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Core compilation FAILED" -ForegroundColor Red
    exit 1
}

# List of core object files
$objs = Get-ChildItem -Path obj -Filter *.obj | ForEach-Object { 'obj\' + $_.Name }
$obj_string = $objs -join " "

# Step 2: Compile llama-dependent sources (needs llama headers)
Write-Host "Compiling llama-dependent sources..." -ForegroundColor Magenta
cmd.exe /c "call `"$vcvars`" && cl.exe /EHsc /std:c++17 $llama_defines $llama_include /c $llama_sources /Foobj/llama_objs/"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Llama source compilation FAILED" -ForegroundColor Red
    exit 1
}

$llama_objs = Get-ChildItem -Path obj/llama_objs -Filter *.obj | ForEach-Object { 'obj\llama_objs\' + $_.Name }
$llama_obj_string = $llama_objs -join " "

$core_benchmarks = @(
    "download_1_5b_model",
    "load_benchmark", "memory_benchmark", "session_benchmark", "retrieval_stub_benchmark",
    "mmap_benchmark", "inference_benchmark", "kv_cache_benchmark", "streaming_benchmark",
    "android_profile_benchmark", "inference_stress_benchmark", "context_budget_benchmark",
    "retrieval_latency_benchmark", "thermal_stress_benchmark", "watchdog_recovery_benchmark",
    "voice_latency_benchmark", "voice_lifecycle_benchmark", "realtime_hardening_benchmark",
    "image_ingestion_benchmark", "ocr_latency_benchmark", "multimodal_scheduler_benchmark",
    "frame_pressure_benchmark", "thermal_vision_stress_benchmark", "camera_lifecycle_benchmark",
    "llm_lifecycle_benchmark", "storage_chaos_benchmark", "quarantine_adaptive_benchmark",
    "watchdog_cooperative_benchmark", "telemetry_normalization_benchmark", "thermal_cycling_soak_benchmark",
    "nnapi_partitioning_benchmark", "nnapi_capability_score_benchmark", "nnapi_rejection_benchmark",
    "nnapi_transfer_overhead_benchmark", "nnapi_prefill_decode_split_benchmark", "nnapi_residency_benchmark",
    "nnapi_probe_timeout_benchmark", "nnapi_startup_latency_benchmark",
    "thermal_hysteresis_benchmark", "lmk_integrity_benchmark", "recovery_quality_drift_benchmark",
    "page_fault_burst_benchmark", "throughput_half_life_benchmark", "startup_variance_benchmark",
    "randomized_fallback_storm_benchmark", "device_memory_collapse_benchmark", "efficiency_decay_curve_benchmark",
    "apk_size_governance_benchmark", "download_interruption_chaos_benchmark", "rapid_model_switch_benchmark",
    "install_update_recovery_benchmark", "session_recovery_benchmark", "permission_chaos_benchmark",
    "survival_benchmark", "survival_a_benchmark",
    "thermal_recovery_window_benchmark", "background_checkpoint_priority_benchmark", "hard_recovery_mode_benchmark",
    "storage_recovery_assistant_benchmark", "device_thermal_ceiling_benchmark", "deployment_health_summary_benchmark",
    "uninstall_risk_policy_benchmark",
    "first_token_consistency_benchmark", "ui_starvation_guard_benchmark", "model_recommendation_policy_benchmark",
    "recovery_notification_policy_benchmark", "memory_drift_trend_guard_benchmark", "apk_size_budget_policy_benchmark",
    "install_time_device_profiler_benchmark", "first_run_experience_policy_benchmark", "background_resume_latency_benchmark",
    "telemetry_export_benchmark", "ios_coreml_manager_benchmark",
    "apk_update_migration_validation_benchmark", "battery_lifetime_observer_benchmark", "install_failure_recovery_benchmark",
    "consumer_expectation_policy_benchmark", "session_durability_validation_benchmark", "ship_blocker_matrix_benchmark",
    "first_week_retention_validation_benchmark", "storage_fragmentation_guard_benchmark", "oem_update_regression_policy_benchmark",
    "reinstall_recovery_policy_benchmark",
    "survivability_residency_class_benchmark"
)

# Llama.cpp benchmarks (require llama.lib + llama_cpp_engine.obj)
$llama_benchmarks = @(
    "llm_integration_benchmark", "advanced_llm_stress_benchmark",
    "compute_buffer_profiling_benchmark", "peak_memory_benchmark",
    "llm_1_5b_integration_benchmark", "overlap_spike_benchmark",
    "android_lmk_chaos_benchmark", "dual_model_tokenizer_benchmark",
    "prompt_scale_benchmark"
)

foreach ($b in $core_benchmarks) {
    Write-Host "Compiling and linking $b..." -ForegroundColor Cyan
    cmd.exe /c "call `"$vcvars`" && cl.exe /EHsc /std:c++17 $include benchmarks/$b.cpp $obj_string /Febin/$b.exe /Foobj/"
}

foreach ($b in $llama_benchmarks) {
    Write-Host "Compiling and linking $b (with llama.lib)..." -ForegroundColor Magenta
    cmd.exe /c "call `"$vcvars`" && cl.exe /EHsc /std:c++17 $llama_defines $llama_include benchmarks/$b.cpp $obj_string $llama_obj_string $llama_lib advapi32.lib /Febin/$b.exe /Foobj/"
}

# Run them
Write-Host "`n=== RUNNING BENCHMARKS ===`n" -ForegroundColor Green

$all_benchmarks = $core_benchmarks + $llama_benchmarks
foreach ($b in $all_benchmarks) {
    if (Test-Path "bin/$b.exe") {
        Write-Host "Running $b..." -ForegroundColor Yellow
        & "bin/$b.exe"
    } else {
        Write-Host "Failed to build $b" -ForegroundColor Red
    }
}
