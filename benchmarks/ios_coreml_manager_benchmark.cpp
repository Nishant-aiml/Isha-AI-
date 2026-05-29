#include <iostream>
#include <cassert>
#include <thread>
#include "model/model_manager.hpp"
#include "inference/ios_coreml_manager.hpp"

using namespace isha;

int main() {
    std::cout << "Starting ios_coreml_manager_benchmark..." << std::endl;

    DeviceProfile profile{DeviceTier::FLAGSHIP, 12288, 8, "iOS"};
    ModelManager manager(profile);
    IosCoremlManager ios_mgr(manager);

    assert(ios_mgr.isAneAccelerated());

    // Load large T3 model
    bool loaded = manager.loadModel("mistral-7b", 4200);
    assert(loaded);
    assert(manager.getState() == ModelState::ACTIVE);

    // Test iOS didReceiveMemoryWarning event
    ios_mgr.handleMemoryWarning();
    assert(manager.getState() == ModelState::UNLOADED);

    // Test iOS transition to background
    std::this_thread::sleep_for(std::chrono::milliseconds(2050)); // Cooldown wait
    manager.loadModel("gemma-3-2b", 1200);
    assert(manager.getState() == ModelState::ACTIVE);
    ios_mgr.handleBackgroundEntry();
    assert(manager.getState() == ModelState::UNLOADED);

    // Test iOS background task expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(2050)); // Cooldown wait
    manager.loadModel("qwen-1.5b", 700);
    assert(manager.getState() == ModelState::ACTIVE);
    ios_mgr.handleBackgroundExpiration();
    assert(manager.getState() == ModelState::UNLOADED);

    std::cout << "ios_coreml_manager_benchmark PASSED!" << std::endl;
    return 0;
}
