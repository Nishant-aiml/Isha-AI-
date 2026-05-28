#include <iostream>
#include <vector>
#include <cassert>
#include "survival/battery_floor_policy.hpp"
#include "survival/storage_floor_policy.hpp"
#include "survival/write_budget_manager.hpp"
#include "survival/field_failure_escalation.hpp"
#include "survival/device_certification_matrix.hpp"
#include "survival/update_safety_window.hpp"
#include "survival/foreground_service_policy.hpp"
#include "survival/dynamic_degradation_ladder.hpp"
#include "survival/runtime_health_snapshot.hpp"
#include "survival/model_integrity_fastscan.hpp"
#include "survival/recovery_ux_manager.hpp"

using namespace isha;

void testBatteryFloorPolicy() {
    std::cout << "Testing BatteryFloorPolicy..." << std::endl;
    assert(BatteryFloorPolicy::determineState(50.0) == BatteryState::NORMAL);
    assert(BatteryFloorPolicy::determineState(30.0) == BatteryState::LIGHT_THROTTLE);
    assert(BatteryFloorPolicy::determineState(15.0) == BatteryState::SURVIVAL_MODE);
    assert(BatteryFloorPolicy::determineState(5.0) == BatteryState::SAFE_MODE_RECOMMENDED);
    
    BatteryPolicy normal_policy = BatteryFloorPolicy::getPolicy(BatteryState::NORMAL);
    assert(normal_policy.optimal_thread_count == 4);
    assert(normal_policy.allow_acceleration);
    
    BatteryPolicy low_policy = BatteryFloorPolicy::getPolicy(BatteryState::SURVIVAL_MODE);
    assert(low_policy.optimal_thread_count == 1);
    assert(!low_policy.allow_acceleration);
    std::cout << "BatteryFloorPolicy PASSED!" << std::endl;
}

void testStorageFloorPolicy() {
    std::cout << "Testing StorageFloorPolicy..." << std::endl;
    assert(StorageFloorPolicy::determineStorageState(3.0) == StorageState::NORMAL);
    assert(StorageFloorPolicy::determineStorageState(1.5) == StorageState::LOW_STORAGE_WARNING);
    assert(StorageFloorPolicy::determineStorageState(0.7) == StorageState::SURVIVAL_STORAGE_MODE);
    assert(StorageFloorPolicy::determineStorageState(0.2) == StorageState::SAFE_MODE_STORAGE_RECOVERY);
    
    assert(StorageFloorPolicy::allowModelDownload(3.0, 1.0));
    assert(!StorageFloorPolicy::allowModelDownload(1.5, 1.0)); // Remaining space below limit
    
    auto cleanup = StorageFloorPolicy::getRequiredCleanup(StorageState::SURVIVAL_STORAGE_MODE);
    assert(cleanup.purge_stale_temp);
    assert(cleanup.purge_telemetry_history);
    assert(!cleanup.purge_prompt_caches);
    std::cout << "StorageFloorPolicy PASSED!" << std::endl;
}

void testWriteBudgetManager() {
    std::cout << "Testing WriteBudgetManager..." << std::endl;
    WriteBudgetManager& manager = WriteBudgetManager::getInstance();
    manager.resetAccumulator();
    
    manager.setHourlyLimit(1000);
    assert(manager.requestWritePermission(400));
    manager.recordActualWrite(400);
    assert(manager.getAccumulatedWrites() == 400);
    
    assert(manager.requestWritePermission(400));
    manager.recordActualWrite(400);
    
    // Pushed past limit
    assert(!manager.requestWritePermission(300));
    
    manager.adaptBudget(true, false, false, false); // eMMC slow wear mitigation
    assert(manager.getHourlyLimit() < 10000000);
    std::cout << "WriteBudgetManager PASSED!" << std::endl;
}

void testFieldFailureEscalation() {
    std::cout << "Testing FieldFailureEscalation..." << std::endl;
    EscalationCounters counters = {0, 0, 0, 0, 0, 0, 0};
    auto policy = FieldFailureEscalation::evaluateFailureRates(counters);
    assert(!policy.is_field_critical);
    
    counters.anr_count = 4; // Repeated ANRs registered
    policy = FieldFailureEscalation::evaluateFailureRates(counters);
    assert(policy.is_field_critical);
    assert(policy.disable_acceleration);
    assert(policy.limit_context_size == 256);
    std::cout << "FieldFailureEscalation PASSED!" << std::endl;
}

void testDeviceCertificationMatrix() {
    std::cout << "Testing DeviceCertificationMatrix..." << std::endl;
    DeviceSpecs specs;
    specs.chipset_family = "Snapdragon";
    specs.ram_gb = 8;
    specs.is_emmc = false;
    specs.android_sdk_version = 33;
    specs.high_oem_restrictions = false;
    
    assert(DeviceCertificationMatrix::evaluateSpecs(specs) == CertificationTier::CERTIFIED);
    
    specs.ram_gb = 2; // Below 3GB RAM limit
    assert(DeviceCertificationMatrix::evaluateSpecs(specs) == CertificationTier::UNSAFE);
    assert(!DeviceCertificationMatrix::isDeploymentAllowed(CertificationTier::UNSAFE));
    std::cout << "DeviceCertificationMatrix PASSED!" << std::endl;
}

void testUpdateSafetyWindow() {
    std::cout << "Testing UpdateSafetyWindow..." << std::endl;
    UpdateSafetyFactors factors;
    factors.battery_level_percentage = 50.0;
    factors.thermal_temp_c = 30.0;
    factors.free_storage_gb = 2.0;
    factors.active_inference_running = false;
    factors.watchdog_escalation_active = false;
    factors.safe_mode_active = false;
    factors.fragmentation_critical = false;
    
    assert(UpdateSafetyWindow::isSafetyWindowOpen(factors));
    
    factors.battery_level_percentage = 10.0; // Critically low battery
    assert(!UpdateSafetyWindow::isSafetyWindowOpen(factors));
    std::cout << "UpdateSafetyWindow PASSED!" << std::endl;
}

void testForegroundServicePolicy() {
    std::cout << "Testing ForegroundServicePolicy..." << std::endl;
    BackgroundConstraints constraints;
    constraints.is_doze_mode_active = false;
    constraints.is_oem_background_kill_aggressive = false;
    constraints.standby_bucket = AppStandbyBucket::ACTIVE;
    
    auto params = ForegroundServicePolicy::evaluateConstraints(constraints);
    assert(params.run_as_foreground_service);
    assert(params.allow_heavy_inference);
    
    constraints.is_doze_mode_active = true; // Doze Mode engaged
    params = ForegroundServicePolicy::evaluateConstraints(constraints);
    assert(!params.allow_heavy_inference);
    assert(params.context_size_cells == 256);
    std::cout << "ForegroundServicePolicy PASSED!" << std::endl;
}

void testDynamicDegradationLadder() {
    std::cout << "Testing DynamicDegradationLadder..." << std::endl;
    EnvironmentalStress stress;
    stress.temp_c = 25.0;
    stress.fragmentation_ratio = 0.1;
    stress.battery_percentage = 90.0;
    stress.free_storage_gb = 5.0;
    stress.anr_risk_threat = false;
    stress.watchdog_escalation = false;
    stress.page_fault_storm = false;
    
    assert(DynamicDegradationLadder::determineDegradationStep(stress) == DegradationStep::NORMAL);
    
    stress.temp_c = 43.0; // Moderate thermal surge
    assert(DynamicDegradationLadder::determineDegradationStep(stress) == DegradationStep::SURVIVAL_MODE);
    
    stress.anr_risk_threat = true; // Extreme block risk
    assert(DynamicDegradationLadder::determineDegradationStep(stress) == DegradationStep::SAFE_MODE);
    
    auto limits = DynamicDegradationLadder::getLimits(DegradationStep::SURVIVAL_MODE);
    assert(!limits.use_acceleration);
    assert(limits.batch_size == 4);
    std::cout << "DynamicDegradationLadder PASSED!" << std::endl;
}

void testRuntimeHealthSnapshot() {
    std::cout << "Testing RuntimeHealthSnapshot..." << std::endl;
    HealthRecord record = RuntimeHealthSnapshot::getDefaults();
    assert(record.last_crash_reason == "NONE");
    
    std::vector<uint8_t> buffer;
    assert(RuntimeHealthSnapshot::serialize(record, buffer));
    assert(!buffer.empty());
    
    HealthRecord deserialized;
    assert(RuntimeHealthSnapshot::deserialize(buffer, deserialized));
    assert(deserialized.last_crash_reason == "NONE");
    std::cout << "RuntimeHealthSnapshot PASSED!" << std::endl;
}

void testModelIntegrityFastscan() {
    std::cout << "Testing ModelIntegrityFastscan..." << std::endl;
    assert(ModelIntegrityFastscan::performFastScan("model.gguf", false) == ModelIntegrityFastscan::ScanResult::SUCCESS);
    assert(ModelIntegrityFastscan::performFastScan("corrupt_model.gguf", false) == ModelIntegrityFastscan::ScanResult::SPOT_CHECK_CORRUPT);
    
    assert(ModelIntegrityFastscan::performFullChecksum("model.gguf", "VALID_SHA256"));
    assert(!ModelIntegrityFastscan::performFullChecksum("model.gguf", "INVALID_SHA256"));
    std::cout << "ModelIntegrityFastscan PASSED!" << std::endl;
}

void testRecoveryUXManager() {
    std::cout << "Testing RecoveryUXManager..." << std::endl;
    std::string heading = RecoveryUXManager::getDisplayHeading(UserNotificationEvent::SAFE_MODE_ACTIVATED);
    std::string desc = RecoveryUXManager::getDisplayDescription(UserNotificationEvent::SAFE_MODE_ACTIVATED);
    
    assert(heading == "Safe Mode Enabled");
    assert(!desc.empty());
    std::cout << "RecoveryUXManager PASSED!" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "RUNNING PHASE 10A SURVIVABILITY BENCHMARK" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testBatteryFloorPolicy();
    testStorageFloorPolicy();
    testWriteBudgetManager();
    testFieldFailureEscalation();
    testDeviceCertificationMatrix();
    testUpdateSafetyWindow();
    testForegroundServicePolicy();
    testDynamicDegradationLadder();
    testRuntimeHealthSnapshot();
    testModelIntegrityFastscan();
    testRecoveryUXManager();
    
    std::cout << "========================================" << std::endl;
    std::cout << "ALL PHASE 10A SURVIVABILITY TESTS PASSED!" << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
