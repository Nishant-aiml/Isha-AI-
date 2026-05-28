#ifndef ISHA_AI_CAMERA_SESSION_MANAGER_HPP
#define ISHA_AI_CAMERA_SESSION_MANAGER_HPP

#include <string>
#include <mutex>
#include <atomic>

namespace isha {

enum class CameraState {
    IDLE,
    OPENING,
    ACTIVE,
    PAUSED,
    ERROR,
    PERMISSION_REVOKED,
    RESOURCE_CONFLICT
};

class CameraSessionManager {
public:
    CameraSessionManager();
    ~CameraSessionManager() = default;

    // Session lifecycle
    bool openSession();
    void closeSession();
    void pauseSession();
    bool resumeSession();

    // Error handling
    void handlePermissionRevoke();
    void handleResourceConflict();
    bool tryRecoverSession();

    // State inspection
    CameraState getState() const;
    std::string stateToString(CameraState state) const;

    // Statistics
    unsigned int sessionCount() const { return session_count_.load(); }
    unsigned int recoveryCount() const { return recovery_count_.load(); }
    unsigned int revokeCount() const { return revoke_count_.load(); }

private:
    std::atomic<CameraState> state_{CameraState::IDLE};
    std::atomic<unsigned int> session_count_{0};
    std::atomic<unsigned int> recovery_count_{0};
    std::atomic<unsigned int> revoke_count_{0};

    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_CAMERA_SESSION_MANAGER_HPP
