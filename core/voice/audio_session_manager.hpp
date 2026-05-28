#ifndef ISHA_AI_AUDIO_SESSION_MANAGER_HPP
#define ISHA_AI_AUDIO_SESSION_MANAGER_HPP

#include <string>
#include <mutex>
#include <atomic>

namespace isha {

class AudioSessionManager {
public:
    static AudioSessionManager& getInstance() {
        static AudioSessionManager instance;
        return instance;
    }

    bool requestMicrophoneFocus();
    void abandonMicrophoneFocus();
    
    void setPauseOnFocusLoss(bool pause);
    bool getPauseOnFocusLoss() const;

    void setRoute(const std::string& route);
    std::string getRoute() const;

    bool isMicrophoneActive() const;
    bool hasAudioFocus() const;

    void simulateFocusLoss();
    void simulateFocusGain();
    void simulateRouteChanged(const std::string& new_route);

private:
    AudioSessionManager() = default;
    ~AudioSessionManager() = default;
    AudioSessionManager(const AudioSessionManager&) = delete;
    AudioSessionManager& operator=(const AudioSessionManager&) = delete;

    std::atomic<bool> has_focus_{false};
    std::atomic<bool> mic_active_{false};
    std::atomic<bool> pause_on_loss_{true};
    std::string current_route_ = "speaker";
    
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_AUDIO_SESSION_MANAGER_HPP
