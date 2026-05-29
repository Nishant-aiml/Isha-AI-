#ifndef ISHA_AI_DOWNLOAD_MANAGER_HPP
#define ISHA_AI_DOWNLOAD_MANAGER_HPP

#include <string>
#include <atomic>
#include <functional>
#include <cstdint>
#include <mutex>

namespace isha {

struct DownloadResult {
    bool success = false;
    std::string filepath;
    std::string error;
    uint64_t bytes_downloaded = 0;
    uint64_t total_bytes = 0;
    bool sha256_verified = false;
    int retry_count = 0;
};

struct DownloadConfig {
    std::string url;
    std::string output_path;
    std::string expected_sha256;     // Empty = skip verification
    uint64_t expected_size_bytes = 0; // 0 = skip size check
    int max_retries = 3;
    int retry_delay_seconds = 5;
    uint64_t min_disk_space_bytes = 0; // Minimum free space required
    bool resume_enabled = true;       // Attempt resume on retry
    uint64_t safety_margin_bytes = 512 * 1024 * 1024; // 512MB headroom after download
    bool validate_post_download = true;                // Verify file integrity
};

struct StorageValidation {
    bool sufficient_space = false;
    uint64_t free_before_bytes = 0;
    uint64_t required_bytes = 0;       // file + temp overhead
    uint64_t safety_margin_bytes = 0;
    std::string rejection_reason;
};

using ProgressCallback = std::function<void(uint64_t downloaded, uint64_t total, float percent)>;

class DownloadManager {
public:
    DownloadManager();

    // Download with full governance
    DownloadResult download(const DownloadConfig& config, ProgressCallback progress = nullptr);

    // Verify existing file
    bool verifyFile(const std::string& filepath, const std::string& expected_sha256, 
                    uint64_t expected_size = 0) const;

    // Check disk space
    bool hasSufficientDiskSpace(const std::string& path, uint64_t required_bytes) const;

    // Storage check helper with 1.5x factor
    StorageValidation validateStorageHeadroom(const DownloadConfig& config) const;

    // Clean partial downloads
    void cleanPartialDownload(const std::string& filepath);

    // Rollback and cleanup helpers
    void cleanupCorruptedDownload(const std::string& filepath);
    void cleanupRollback(const std::string& filepath, const std::string& backup_path);

    // Storage governance policy structures and methods
    struct ModelStorageStats {
        std::string model_id;
        int days_inactive = 0;
        uint64_t size_bytes = 0;
        std::string filepath;
    };

    // Evicts models based on inactivity time and model tiers (T0/T1 keep resident, T2 after 7 days, T3 after 14 days)
    void enforceStorageGovernance(const std::vector<ModelStorageStats>& stats);

    // Evicts models under storage pressure, deleting T3 first, then T2, never T0
    void handleStoragePressureEviction(const std::vector<ModelStorageStats>& stats);

    // Stats
    uint32_t totalDownloads() const { return total_downloads_.load(std::memory_order_relaxed); }
    uint32_t failedDownloads() const { return failed_downloads_.load(std::memory_order_relaxed); }
    uint32_t totalRetries() const { return total_retries_.load(std::memory_order_relaxed); }

private:
    // SHA256 computation (platform-independent, uses Win32 CryptoAPI on Windows)
    std::string computeSHA256(const std::string& filepath) const;
    
    // File size check
    uint64_t getFileSize(const std::string& filepath) const;

    // Execute curl download
    bool executeCurl(const std::string& url, const std::string& output_path, 
                     bool resume, ProgressCallback progress);

    std::atomic<uint32_t> total_downloads_{0};
    std::atomic<uint32_t> failed_downloads_{0};
    std::atomic<uint32_t> total_retries_{0};
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_DOWNLOAD_MANAGER_HPP
