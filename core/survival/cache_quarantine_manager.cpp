#include "cache_quarantine_manager.hpp"

namespace isha {

CacheQuarantineManager& CacheQuarantineManager::getInstance() {
    static CacheQuarantineManager instance;
    return instance;
}

bool CacheQuarantineManager::quarantineFile(const std::string& path, const std::string& reason) {
    if (path.empty()) return false;
    std::lock_guard<std::mutex> lock(registry_mutex_);
    quarantine_registry_[path] = reason;
    return true;
}

bool CacheQuarantineManager::isQuarantined(const std::string& path) const {
    if (path.empty()) return false;
    std::lock_guard<std::mutex> lock(registry_mutex_);
    return quarantine_registry_.find(path) != quarantine_registry_.end();
}

void CacheQuarantineManager::clearQuarantine() {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    quarantine_registry_.clear();
}

std::vector<std::string> CacheQuarantineManager::getQuarantinedFiles() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    std::vector<std::string> files;
    for (const auto& pair : quarantine_registry_) {
        files.push_back(pair.first);
    }
    return files;
}

std::string CacheQuarantineManager::getReason(const std::string& path) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = quarantine_registry_.find(path);
    if (it != quarantine_registry_.end()) {
        return it->second;
    }
    return "";
}

} // namespace isha
