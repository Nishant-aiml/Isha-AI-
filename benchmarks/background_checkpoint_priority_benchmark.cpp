#include <iostream>
#include <cassert>
#include "survival/background_checkpoint_priority.hpp"

using namespace isha;

int main() {
    std::cout << "Starting background_checkpoint_priority_benchmark..." << std::endl;

    ExecutionContextPressure pressure = {false, false, false, false, false};
    
    // Default healthy operational conditions: all writes are allowed
    assert(BackgroundCheckpointPriority::shouldWriteData(PriorityTier::CRITICAL, pressure));
    assert(BackgroundCheckpointPriority::shouldWriteData(PriorityTier::DISCARDABLE, pressure));

    // Under background LMK pressure, only critical writes are allowed
    pressure.lmk_pressure = true;
    assert(BackgroundCheckpointPriority::shouldWriteData(PriorityTier::CRITICAL, pressure));
    assert(!BackgroundCheckpointPriority::shouldWriteData(PriorityTier::MEDIUM, pressure));
    assert(!BackgroundCheckpointPriority::shouldWriteData(PriorityTier::DISCARDABLE, pressure));

    std::cout << "background_checkpoint_priority_benchmark PASSED!" << std::endl;
    return 0;
}
