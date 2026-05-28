#include <iostream>
#include <vector>
#include <cassert>
#include "survival/runtime_state_versioning.hpp"
#include "survival/cache_quarantine_manager.hpp"
#include "survival/atomic_model_extraction.hpp"
#include "survival/install_recovery_manager.hpp"
#include "survival/storage_speed_classifier.hpp"
#include "survival/startup_strategy_manager.hpp"
#include "survival/background_survival_manager.hpp"
#include "survival/survival_priority_mode.hpp"
#include "survival/local_data_integrity_manager.hpp"

using namespace isha;

void testRuntimeStateVersioning() {
    std::cout << "Testing RuntimeStateVersioning..." << std::endl;
    RuntimeState state = RuntimeStateVersioning::getDefaults();
    assert(RuntimeStateVersioning::verifyVersion(state));
    
    std::vector<uint8_t> buffer;
    assert(RuntimeStateVersioning::serialize(state, buffer));
    assert(!buffer.empty());
    
    RuntimeState deserialized;
    assert(RuntimeStateVersioning::deserialize(buffer, deserialized));
    assert(deserialized.version == state.version);
    assert(deserialized.build_id == state.build_id);
    
    // Test version mismatch
    deserialized.version = 999;
    assert(!RuntimeStateVersioning::verifyVersion(deserialized));
    
    // Test migration
    RuntimeState migrated;
    assert(RuntimeStateVersioning::migrateState(deserialized, migrated));
    assert(RuntimeStateVersioning::verifyVersion(migrated));
    std::cout << "RuntimeStateVersioning PASSED!" << std::endl;
}

void testCacheQuarantineManager() {
    std::cout << "Testing CacheQuarantineManager..." << std::endl;
    CacheQuarantineManager& manager = CacheQuarantineManager::getInstance();
    manager.clearQuarantine();
    
    std::string bad_path = "/sdcard/isha/corrupt_tokenizer.bin";
    assert(!manager.isQuarantined(bad_path));
    
    assert(manager.quarantineFile(bad_path, "CRC check failed"));
    assert(manager.isQuarantined(bad_path));
    assert(manager.getReason(bad_path) == "CRC check failed");
    
    auto list = manager.getQuarantinedFiles();
    assert(list.size() == 1);
    assert(list[0] == bad_path);
    
    manager.clearQuarantine();
    assert(!manager.isQuarantined(bad_path));
    std::cout << "CacheQuarantineManager PASSED!" << std::endl;
}

void testAtomicModelExtraction() {
    std::cout << "Testing AtomicModelExtraction..." << std::endl;
    AtomicModelExtractionPipeline::ExtractionConfig config;
    config.download_temp_path = "tmp_model.gguf";
    config.extraction_temp_dir = "extract_tmp";
    config.final_model_path = "model.gguf";
    config.expected_checksum = "VALID_MD5_HASH";
    config.required_space_bytes = 100 * 1024 * 1024; // 100MB
    
    auto result = AtomicModelExtractionPipeline::execute(config);
    assert(result == AtomicModelExtractionPipeline::PipelineResult::SUCCESS);
    
    // Test storage insufficient
    config.required_space_bytes = 2000ULL * 1024ULL * 1024ULL; // 2GB
    result = AtomicModelExtractionPipeline::execute(config);
    assert(result == AtomicModelExtractionPipeline::PipelineResult::STORAGE_INSUFFICIENT);
    std::cout << "AtomicModelExtraction PASSED!" << std::endl;
}

void testInstallRecoveryManager() {
    std::cout << "Testing InstallRecoveryManager..." << std::endl;
    auto status = InstallRecoveryManager::diagnosticSelfCheck();
    assert(status == InstallRecoveryManager::SystemStatus::STABLE);
    
    auto report = InstallRecoveryManager::performEmergencyRollback(InstallRecoveryManager::SystemStatus::CORRUPTED_MODEL);
    assert(report.success);
    assert(report.restored_model_version == "KNOWN_GOOD_CPU_0.5B");
    std::cout << "InstallRecoveryManager PASSED!" << std::endl;
}

void testStorageSpeedClassifier() {
    std::cout << "Testing StorageSpeedClassifier..." << std::endl;
    auto tier = StorageSpeedClassifier::classifyStorage(".");
    std::cout << "Detected storage tier: " << StorageSpeedClassifier::tierToString(tier) << std::endl;
    
    auto policy = StorageSpeedClassifier::getPolicy(tier);
    if (tier == StorageSpeedTier::EMMC_SLOW) {
        assert(!policy.mmap_prefetch_aggressive);
    } else {
        assert(policy.mmap_prefetch_aggressive);
    }
    std::cout << "StorageSpeedClassifier PASSED!" << std::endl;
}

void testStartupStrategyManager() {
    std::cout << "Testing StartupStrategyManager..." << std::endl;
    auto type = StartupStrategyManager::determineStartupType(true, false, false);
    assert(type == StartupType::COLD);
    
    auto budgets = StartupStrategyManager::getBudget(type);
    assert(budgets.max_startup_latency_ms == 4000);
    
    StartupMetrics metrics;
    metrics.startup_latency_ms = 1500;
    metrics.tokenizer_init_ms = 200;
    metrics.mmap_init_ms = 400;
    metrics.first_token_latency_ms = 1200;
    metrics.anr_risk_detected = false;
    
    assert(StartupStrategyManager::validateStartup(type, metrics));
    std::cout << "StartupStrategyManager PASSED!" << std::endl;
}

void testBackgroundSurvivalManager() {
    std::cout << "Testing BackgroundSurvivalManager..." << std::endl;
    auto background_params = BackgroundSurvivalManager::onEnterBackground();
    assert(background_params.acceleration_unloaded);
    assert(background_params.context_size_cells == 512);
    
    auto foreground_params = BackgroundSurvivalManager::onEnterForeground();
    assert(!foreground_params.acceleration_unloaded);
    assert(foreground_params.context_size_cells == 2048);
    std::cout << "BackgroundSurvivalManager PASSED!" << std::endl;
}

void testSurvivalPriorityMode() {
    std::cout << "Testing SurvivalPriorityMode..." << std::endl;
    DeviceMetrics metrics;
    metrics.thermal_temp_c = 45.0; // High thermal spike
    metrics.fragmentation_ratio = 0.2;
    metrics.anr_risk_threat = false;
    metrics.scheduler_stalled = false;
    metrics.storage_stalled = false;
    metrics.battery_drain_rate = 1.0;
    metrics.oem_restrictions_tight = false;
    
    auto policy = SurvivalPriorityMode::evaluateSystemMetrics(metrics);
    assert(policy.survival_mode_active);
    assert(!policy.use_acceleration); // Falls back to CPU
    assert(policy.optimal_batch_size == 4);
    std::cout << "SurvivalPriorityMode PASSED!" << std::endl;
}

void testLocalDataIntegrityManager() {
    std::cout << "Testing LocalDataIntegrityManager..." << std::endl;
    std::string test_path = "./integrity_test.dat";
    std::vector<uint8_t> original_data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    
    assert(LocalDataIntegrityManager::transactionallySaveFile(test_path, original_data));
    
    std::vector<uint8_t> read_data;
    assert(LocalDataIntegrityManager::transactionallyReadFile(test_path, read_data));
    assert(read_data == original_data);
    
    // Clean up
    std::remove(test_path.c_str());
    std::remove((test_path + ".bak").c_str());
    std::cout << "LocalDataIntegrityManager PASSED!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "RUNNING PHASE 10 SURVIVABILITY BENCHMARK" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testRuntimeStateVersioning();
    testCacheQuarantineManager();
    testAtomicModelExtraction();
    testInstallRecoveryManager();
    testStorageSpeedClassifier();
    testStartupStrategyManager();
    testBackgroundSurvivalManager();
    testSurvivalPriorityMode();
    testLocalDataIntegrityManager();
    
    std::cout << "========================================" << std::endl;
    std::cout << "ALL PHASE 10 SURVIVABILITY TESTS PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
