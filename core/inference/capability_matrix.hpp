#ifndef ISHA_AI_CAPABILITY_MATRIX_HPP
#define ISHA_AI_CAPABILITY_MATRIX_HPP

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <mutex>
#include "../logging/logger.hpp"

namespace isha {

struct CapabilityRecord {
    std::string fingerprint;
    bool is_stable = true;
    std::vector<std::string> successful_modes;
    std::vector<std::string> failed_modes;
    int fallback_count = 0;
    float max_temp_observed = 0.0f;
};

class CapabilityMatrix {
public:
    static CapabilityMatrix& getInstance() {
        static CapabilityMatrix instance;
        return instance;
    }

    void load(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(mutex_);
        filepath_ = filepath;
        std::ifstream file(filepath);
        if (!file.is_open()) return;

        records_.clear();
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string fp;
            int stable_int = 1;
            int fallbacks = 0;
            float max_temp = 0.0f;
            
            if (std::getline(ss, fp, ',')) {
                ss >> stable_int;
                ss.ignore();
                ss >> fallbacks;
                ss.ignore();
                ss >> max_temp;
                
                auto& rec = records_[fp];
                rec.fingerprint = fp;
                rec.is_stable = (stable_int != 0);
                rec.fallback_count = fallbacks;
                rec.max_temp_observed = max_temp;
            }
        }
    }

    void save() {
        if (filepath_.empty()) return;
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream file(filepath_);
        if (!file.is_open()) return;

        for (const auto& [fp, rec] : records_) {
            file << fp << ","
                 << (rec.is_stable ? 1 : 0) << ","
                 << rec.fallback_count << ","
                 << rec.max_temp_observed << "\n";
        }
    }

    void recordFallback(const std::string& fingerprint, float temp) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& rec = records_[fingerprint];
            rec.fingerprint = fingerprint;
            rec.fallback_count++;
            rec.max_temp_observed = std::max(rec.max_temp_observed, temp);
            if (rec.fallback_count >= 3) {
                rec.is_stable = false;
            }
        }
        save();
    }

    void recordSuccess(const std::string& fingerprint, const std::string& mode) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& rec = records_[fingerprint];
            rec.fingerprint = fingerprint;
            rec.is_stable = true;
            rec.successful_modes.push_back(mode);
        }
        save();
    }

    bool isStable(const std::string& fingerprint) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = records_.find(fingerprint);
        if (it == records_.end()) return true;
        return it->second.is_stable;
    }

private:
    CapabilityMatrix() = default;
    std::map<std::string, CapabilityRecord> records_;
    std::string filepath_;
    std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_CAPABILITY_MATRIX_HPP
