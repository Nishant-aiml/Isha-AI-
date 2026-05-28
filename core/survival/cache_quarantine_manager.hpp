#ifndef ISHA_AI_CACHE_QUARANTINE_MANAGER_HPP
#define ISHA_AI_CACHE_QUARANTINE_MANAGER_HPP

#include <string>
#include <vector>
#include <mutex>
#include <map>

namespace isha {

class CacheQuarantineManager {
public:
    CacheQuarantineManager() = default;
    ~CacheQuarantineManager() = default;

    // Singleton access
    static CacheQuarantineManager& getInstance();

    // Quarantine functions
    bool quarantineFile(const std::string& path, const std::string& reason);
    bool isQuarantined(const std::string& path) const;
    void clearQuarantine();

    // Queries
    std::vector<std::string> getQuarantinedFiles() const;
    std::string getReason(const std::string& path) const;

private:
    std::map<std::string, std::string> quarantine_registry_;
    mutable std::mutex registry_mutex_;
};

} // namespace isha

#endif // ISHA_AI_CACHE_QUARANTINE_MANAGER_HPP
