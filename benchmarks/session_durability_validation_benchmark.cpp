#include "survival/session_durability_validation.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Starting session_durability_validation_benchmark..." << std::endl;

    std::string test_checkpoint = "session_state.dat";
    isha::SessionState original;
    original.last_token_pos = 42;
    original.history_ids = { 101, 203, 506 };
    original.in_transaction = true;

    // Save state atomically
    bool save_ok = isha::SessionDurabilityValidation::saveSessionStateAtomic(test_checkpoint, original);
    assert(save_ok);

    // Restore state and verify
    isha::SessionState reloaded;
    bool load_ok = isha::SessionDurabilityValidation::loadAndVerifyState(test_checkpoint, reloaded);
    assert(load_ok);
    assert(reloaded.last_token_pos == 42);
    assert(reloaded.history_ids.size() == 3);
    assert(reloaded.history_ids[1] == 203);
    assert(reloaded.in_transaction == true);

    // Clean up
    std::remove(test_checkpoint.c_str());

    std::cout << "session_durability_validation_benchmark PASSED!" << std::endl;
    return 0;
}
