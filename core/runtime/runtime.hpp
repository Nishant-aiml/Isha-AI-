#ifndef ISHA_AI_RUNTIME_HPP
#define ISHA_AI_RUNTIME_HPP

#include "../config/runtime_config.hpp"
#include "../config/device_profile.hpp"

namespace isha {

class Runtime {
public:
    Runtime(const RuntimeConfig& config, const DeviceProfile& profile);
    ~Runtime();

    void start();
    void hibernate();
    void wake();
    void shutdown();

    std::string processRequest(const std::string& query);
    
    bool isRunning() const { return is_running_; }
    bool isHibernating() const { return is_hibernating_; }

private:
    RuntimeConfig config_;
    DeviceProfile profile_;
    bool is_running_;
    bool is_hibernating_;
};

} // namespace isha

#endif // ISHA_AI_RUNTIME_HPP
