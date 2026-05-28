#include <iostream>
#include <cassert>
#include <chrono>
#include "../core/vision/camera_session_manager.hpp"

using namespace isha;

static int tests_passed = 0;
static int tests_failed = 0;

static void CHECK(bool cond, const char* msg) {
    if (cond) { ++tests_passed; }
    else { ++tests_failed; std::cerr << "[FAIL] " << msg << "\n"; }
}

// Test 1: Normal lifecycle: IDLE -> OPENING -> ACTIVE -> PAUSED -> ACTIVE -> IDLE
static void test_normal_lifecycle() {
    CameraSessionManager mgr;
    CHECK(mgr.getState() == CameraState::IDLE, "Initial state must be IDLE");
    bool opened = mgr.openSession();
    CHECK(opened, "openSession must succeed from IDLE");
    CHECK(mgr.getState() == CameraState::ACTIVE, "State must be ACTIVE after open");
    mgr.pauseSession();
    CHECK(mgr.getState() == CameraState::PAUSED, "State must be PAUSED");
    bool resumed = mgr.resumeSession();
    CHECK(resumed, "resumeSession must succeed from PAUSED");
    CHECK(mgr.getState() == CameraState::ACTIVE, "State must be ACTIVE after resume");
    mgr.closeSession();
    CHECK(mgr.getState() == CameraState::IDLE, "State must be IDLE after close");
}

// Test 2: Permission revoke from ACTIVE
static void test_permission_revoke_active() {
    CameraSessionManager mgr;
    mgr.openSession();
    CHECK(mgr.getState() == CameraState::ACTIVE, "Must be ACTIVE");
    mgr.handlePermissionRevoke();
    CHECK(mgr.getState() == CameraState::PERMISSION_REVOKED,
          "State must be PERMISSION_REVOKED after revoke");
    CHECK(mgr.revokeCount() == 1, "Revoke counter must increment");
}

// Test 3: Permission revoke from PAUSED
static void test_permission_revoke_paused() {
    CameraSessionManager mgr;
    mgr.openSession();
    mgr.pauseSession();
    mgr.handlePermissionRevoke();
    CHECK(mgr.getState() == CameraState::PERMISSION_REVOKED,
          "Revoke from PAUSED must transition to PERMISSION_REVOKED");
}

// Test 4: Resource conflict handling
static void test_resource_conflict() {
    CameraSessionManager mgr;
    mgr.openSession();
    mgr.handleResourceConflict();
    CHECK(mgr.getState() == CameraState::RESOURCE_CONFLICT,
          "State must be RESOURCE_CONFLICT");
}

// Test 5: Recovery from ERROR
static void test_recovery_from_error() {
    CameraSessionManager mgr;
    mgr.openSession();
    mgr.handleResourceConflict();
    CHECK(mgr.getState() == CameraState::RESOURCE_CONFLICT, "Must be in conflict");
    bool recovered = mgr.tryRecoverSession();
    CHECK(recovered, "Recovery from RESOURCE_CONFLICT must succeed");
    CHECK(mgr.recoveryCount() == 1, "Recovery counter must increment");
}

// Test 6: Invalid transition rejection
static void test_invalid_transitions() {
    CameraSessionManager mgr;
    // Cannot pause from IDLE
    mgr.pauseSession();
    CHECK(mgr.getState() == CameraState::IDLE, "Pause from IDLE must be rejected");
    // Cannot resume from IDLE
    bool resumed = mgr.resumeSession();
    CHECK(!resumed, "Resume from IDLE must fail");
}

// Test 7: Double open
static void test_double_open() {
    CameraSessionManager mgr;
    mgr.openSession();
    bool second = mgr.openSession();
    CHECK(!second, "Double open must be rejected");
    CHECK(mgr.getState() == CameraState::ACTIVE, "State must remain ACTIVE");
}

// Test 8: Session counter
static void test_session_counter() {
    CameraSessionManager mgr;
    mgr.openSession();
    mgr.closeSession();
    mgr.openSession();
    mgr.closeSession();
    mgr.openSession();
    mgr.closeSession();
    CHECK(mgr.sessionCount() == 3, "3 sessions must be counted");
}

// Test 9: State string conversion
static void test_state_strings() {
    CameraSessionManager mgr;
    CHECK(mgr.stateToString(CameraState::IDLE) == "IDLE", "IDLE string");
    CHECK(mgr.stateToString(CameraState::ACTIVE) == "ACTIVE", "ACTIVE string");
    CHECK(mgr.stateToString(CameraState::PAUSED) == "PAUSED", "PAUSED string");
    CHECK(mgr.stateToString(CameraState::ERROR) == "ERROR", "ERROR string");
    CHECK(mgr.stateToString(CameraState::PERMISSION_REVOKED) == "PERMISSION_REVOKED",
          "PERMISSION_REVOKED string");
    CHECK(mgr.stateToString(CameraState::RESOURCE_CONFLICT) == "RESOURCE_CONFLICT",
          "RESOURCE_CONFLICT string");
}

// Test 10: Rapid open/close cycles (lifecycle stability)
static void test_rapid_lifecycle_cycles() {
    CameraSessionManager mgr;
    for (int i = 0; i < 50; ++i) {
        mgr.openSession();
        mgr.pauseSession();
        mgr.resumeSession();
        mgr.closeSession();
    }
    CHECK(mgr.sessionCount() == 50, "50 rapid lifecycle cycles must complete");
    CHECK(mgr.getState() == CameraState::IDLE, "Final state must be IDLE");
}

int main() {
    std::cout << "=== CAMERA LIFECYCLE BENCHMARK ===\n";
    test_normal_lifecycle();
    test_permission_revoke_active();
    test_permission_revoke_paused();
    test_resource_conflict();
    test_recovery_from_error();
    test_invalid_transitions();
    test_double_open();
    test_session_counter();
    test_state_strings();
    test_rapid_lifecycle_cycles();
    std::cout << "Passed: " << tests_passed << " / " << (tests_passed + tests_failed) << "\n";
    if (tests_failed > 0) {
        std::cerr << "FAILED: " << tests_failed << " tests\n";
        return 1;
    }
    std::cout << "[OK] Camera lifecycle benchmark passed.\n";
    return 0;
}
