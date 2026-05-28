#include "model/model_manager.hpp"
#include "logging/logger.hpp"
#include <iostream>
#include <cassert>

int main() {
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::DEBUG);
    ISHA_LOG_INFO("Benchmark", "=== STARTING LOAD BENCHMARK ===");

    isha::DeviceProfile profile = { isha::DeviceTier::MID, 4096, 4, "Windows" };
    isha::ModelManager manager(profile);

    // Initial state check
    assert(manager.getState() == isha::ModelState::UNLOADED);

    // Try loading T1 model
    bool load_ok = manager.loadModel("Qwen2.5-1.5B-Instruct", 950);
    assert(load_ok);
    assert(manager.getState() == isha::ModelState::ACTIVE);
    assert(manager.getActiveModelName() == "Qwen2.5-1.5B-Instruct");
    assert(manager.getActiveModelSizeMB() == 950);
    std::cout << "Model Load Latency: " << manager.getLoadLatencyMs() << " ms" << std::endl;

    // Set idle
    manager.setIdle();
    assert(manager.getState() == isha::ModelState::IDLE);

    // Clear KV cache
    manager.clearKVCache();

    // Unload model
    manager.unloadModel();
    assert(manager.getState() == isha::ModelState::UNLOADED);
    std::cout << "Model Unload Latency: " << manager.getUnloadLatencyMs() << " ms" << std::endl;

    ISHA_LOG_INFO("Benchmark", "=== LOAD BENCHMARK COMPLETED SUCCESSFULLY ===");
    return 0;
}
