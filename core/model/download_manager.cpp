#include "download_manager.hpp"

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#ifdef ERROR
#undef ERROR
#endif
#else
#include <sys/statvfs.h>
#endif

#include "../logging/logger.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <filesystem>


namespace isha {

DownloadManager::DownloadManager() {}

DownloadResult DownloadManager::download(const DownloadConfig& config, ProgressCallback progress) {
    DownloadResult result;

    total_downloads_.fetch_add(1, std::memory_order_relaxed);
    ISHA_LOG_INFO("DownloadMgr", "Starting download: " + config.url);
    ISHA_LOG_INFO("DownloadMgr", "Output: " + config.output_path);

    // Step 1: Disk space validation
    if (config.min_disk_space_bytes > 0) {
        if (!hasSufficientDiskSpace(config.output_path, config.min_disk_space_bytes)) {
            result.error = "Insufficient disk space";
            failed_downloads_.fetch_add(1, std::memory_order_relaxed);
            ISHA_LOG_ERROR("DownloadMgr", result.error);
            return result;
        }
    }

    // Step 2: Check if file already exists and is valid
    if (std::filesystem::exists(config.output_path)) {
        ISHA_LOG_INFO("DownloadMgr", "File already exists: " + config.output_path);
        if (!config.expected_sha256.empty()) {
            if (verifyFile(config.output_path, config.expected_sha256, config.expected_size_bytes)) {
                ISHA_LOG_INFO("DownloadMgr", "Existing file verified. Skipping download.");
                result.success = true;
                result.filepath = config.output_path;
                result.sha256_verified = true;
                result.bytes_downloaded = getFileSize(config.output_path);
                result.total_bytes = result.bytes_downloaded;
                return result;
            } else {
                ISHA_LOG_WARN("DownloadMgr", "Existing file failed verification. Re-downloading.");
                cleanPartialDownload(config.output_path);
            }
        } else {
            // No SHA to verify, assume valid if size matches
            if (config.expected_size_bytes > 0) {
                uint64_t size = getFileSize(config.output_path);
                if (size == config.expected_size_bytes) {
                    ISHA_LOG_INFO("DownloadMgr", "Existing file size matches. Skipping download.");
                    result.success = true;
                    result.filepath = config.output_path;
                    result.bytes_downloaded = size;
                    result.total_bytes = size;
                    return result;
                }
            }
        }
    }

    // Step 3: Create output directory if needed
    std::filesystem::path output_dir = std::filesystem::path(config.output_path).parent_path();
    if (!output_dir.empty() && !std::filesystem::exists(output_dir)) {
        std::filesystem::create_directories(output_dir);
        ISHA_LOG_INFO("DownloadMgr", "Created directory: " + output_dir.string());
    }

    // Step 4: Download with retries
    bool downloaded = false;
    for (int attempt = 0; attempt <= config.max_retries; attempt++) {
        if (attempt > 0) {
            total_retries_.fetch_add(1, std::memory_order_relaxed);
            ISHA_LOG_WARN("DownloadMgr", "Retry attempt " + std::to_string(attempt) +
                          "/" + std::to_string(config.max_retries));
            std::this_thread::sleep_for(std::chrono::seconds(config.retry_delay_seconds));
        }

        bool resume = config.resume_enabled && attempt > 0 &&
                       std::filesystem::exists(config.output_path);

        if (executeCurl(config.url, config.output_path, resume, progress)) {
            downloaded = true;
            result.retry_count = attempt;
            break;
        }
    }

    if (!downloaded) {
        result.error = "Download failed after " + std::to_string(config.max_retries + 1) + " attempts";
        failed_downloads_.fetch_add(1, std::memory_order_relaxed);
        cleanPartialDownload(config.output_path);
        ISHA_LOG_ERROR("DownloadMgr", result.error);
        return result;
    }

    // Step 5: Size validation
    result.bytes_downloaded = getFileSize(config.output_path);
    result.total_bytes = result.bytes_downloaded;

    if (config.expected_size_bytes > 0 && result.bytes_downloaded != config.expected_size_bytes) {
        result.error = "Size mismatch: got " + std::to_string(result.bytes_downloaded) +
                       ", expected " + std::to_string(config.expected_size_bytes);
        failed_downloads_.fetch_add(1, std::memory_order_relaxed);
        cleanPartialDownload(config.output_path);
        ISHA_LOG_ERROR("DownloadMgr", result.error);
        return result;
    }

    // Step 6: SHA256 verification
    if (!config.expected_sha256.empty()) {
        ISHA_LOG_INFO("DownloadMgr", "Verifying SHA256...");
        std::string actual_sha = computeSHA256(config.output_path);
        if (actual_sha != config.expected_sha256) {
            result.error = "SHA256 mismatch: got " + actual_sha +
                           ", expected " + config.expected_sha256;
            failed_downloads_.fetch_add(1, std::memory_order_relaxed);
            cleanPartialDownload(config.output_path);
            ISHA_LOG_ERROR("DownloadMgr", result.error);
            return result;
        }
        result.sha256_verified = true;
        ISHA_LOG_INFO("DownloadMgr", "SHA256 verified.");
    }

    result.success = true;
    result.filepath = config.output_path;
    ISHA_LOG_INFO("DownloadMgr", "Download complete: " + config.output_path +
                  " (" + std::to_string(result.bytes_downloaded / (1024 * 1024)) + "MB)");
    return result;
}

bool DownloadManager::verifyFile(const std::string& filepath, const std::string& expected_sha256,
                                  uint64_t expected_size) const {
    if (!std::filesystem::exists(filepath)) return false;

    if (expected_size > 0) {
        uint64_t actual_size = getFileSize(filepath);
        if (actual_size != expected_size) {
            ISHA_LOG_WARN("DownloadMgr", "Size mismatch for " + filepath);
            return false;
        }
    }

    if (!expected_sha256.empty()) {
        std::string actual_sha = computeSHA256(filepath);
        if (actual_sha != expected_sha256) {
            ISHA_LOG_WARN("DownloadMgr", "SHA256 mismatch for " + filepath);
            return false;
        }
    }

    return true;
}

bool DownloadManager::hasSufficientDiskSpace(const std::string& path, uint64_t required_bytes) const {
    try {
        auto space = std::filesystem::space(std::filesystem::path(path).parent_path());
        bool sufficient = space.available >= required_bytes;
        if (!sufficient) {
            ISHA_LOG_WARN("DownloadMgr", "Disk space: " + 
                          std::to_string(space.available / (1024 * 1024)) + "MB available, " +
                          std::to_string(required_bytes / (1024 * 1024)) + "MB required");
        }
        return sufficient;
    } catch (...) {
        ISHA_LOG_WARN("DownloadMgr", "Could not check disk space. Proceeding.");
        return true;
    }
}

void DownloadManager::cleanPartialDownload(const std::string& filepath) {
    if (std::filesystem::exists(filepath)) {
        std::filesystem::remove(filepath);
        ISHA_LOG_INFO("DownloadMgr", "Cleaned partial download: " + filepath);
    }
    // Also clean .part files
    std::string part_file = filepath + ".part";
    if (std::filesystem::exists(part_file)) {
        std::filesystem::remove(part_file);
        ISHA_LOG_INFO("DownloadMgr", "Cleaned partial file: " + part_file);
    }
}

StorageValidation DownloadManager::validateStorageHeadroom(const DownloadConfig& config) const {
    StorageValidation result;
    result.required_bytes = config.expected_size_bytes + (config.expected_size_bytes / 2); // 1.5x temp
    result.safety_margin_bytes = config.safety_margin_bytes;
    uint64_t total_needed = result.required_bytes + result.safety_margin_bytes;
    if (config.min_disk_space_bytes > total_needed) {
        total_needed = config.min_disk_space_bytes;
    }
    
    try {
        std::filesystem::path parent = std::filesystem::path(config.output_path).parent_path();
        if (parent.empty()) parent = ".";
        if (!std::filesystem::exists(parent)) {
            std::filesystem::create_directories(parent);
        }
        auto space = std::filesystem::space(parent);
        result.free_before_bytes = space.available;
        result.sufficient_space = (space.available >= total_needed);
    } catch (...) {
        result.free_before_bytes = 0;
        result.sufficient_space = true; // Proceed if check fails
    }
    
    if (!result.sufficient_space) {
        result.rejection_reason = "Need " + std::to_string(total_needed / (1024*1024)) + 
                                  "MB (file+1.5x temp + safety), have " + std::to_string(result.free_before_bytes / (1024*1024)) + "MB";
    }
    return result;
}

void DownloadManager::cleanupCorruptedDownload(const std::string& filepath) {
    cleanPartialDownload(filepath);
}

void DownloadManager::cleanupRollback(const std::string& filepath, const std::string& backup_path) {
    cleanupCorruptedDownload(filepath);
    if (std::filesystem::exists(backup_path)) {
        std::filesystem::rename(backup_path, filepath);
        ISHA_LOG_INFO("DownloadMgr", "Rolled back to backup: " + filepath);
    }
}

std::string DownloadManager::computeSHA256(const std::string& filepath) const {
#ifdef _WIN32
    // Windows: Use BCrypt API
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    NTSTATUS status;

    status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status)) {
        ISHA_LOG_ERROR("DownloadMgr", "Failed to open SHA256 provider");
        return "";
    }

    DWORD hashLen = 0;
    DWORD resultLen = 0;
    BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PUCHAR)&hashLen, sizeof(hashLen), &resultLen, 0);

    std::vector<UCHAR> hashValue(hashLen);
    DWORD objLen = 0;
    BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PUCHAR)&objLen, sizeof(objLen), &resultLen, 0);
    std::vector<UCHAR> hashObj(objLen);

    status = BCryptCreateHash(hAlg, &hHash, hashObj.data(), objLen, nullptr, 0, 0);
    if (!BCRYPT_SUCCESS(status)) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    // Read file in chunks and hash
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    constexpr size_t CHUNK_SIZE = 64 * 1024; // 64KB chunks
    std::vector<char> buffer(CHUNK_SIZE);
    while (file.read(buffer.data(), CHUNK_SIZE) || file.gcount() > 0) {
        BCryptHashData(hHash, (PUCHAR)buffer.data(), (ULONG)file.gcount(), 0);
        if (file.gcount() < static_cast<std::streamsize>(CHUNK_SIZE)) break;
    }
    file.close();

    BCryptFinishHash(hHash, hashValue.data(), hashLen, 0);
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    // Convert to hex string
    std::ostringstream hex;
    hex << std::hex;
    for (DWORD i = 0; i < hashLen; i++) {
        hex << std::setfill('0') << std::setw(2) << static_cast<int>(hashValue[i]);
    }
    return hex.str();
#else
    // Unix: Use openssl command line
    std::string cmd = "sha256sum " + filepath + " 2>/dev/null | cut -d' ' -f1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    char buf[128];
    std::string result;
    while (fgets(buf, sizeof(buf), pipe)) {
        result += buf;
    }
    pclose(pipe);
    
    // Trim whitespace
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    return result;
#endif
}

uint64_t DownloadManager::getFileSize(const std::string& filepath) const {
    try {
        return std::filesystem::file_size(filepath);
    } catch (...) {
        return 0;
    }
}

bool DownloadManager::executeCurl(const std::string& url, const std::string& output_path,
                                   bool resume, ProgressCallback /*progress*/) {
    std::string cmd = "curl -L --fail --connect-timeout 30 --max-time 3600";
    
    if (resume) {
        cmd += " -C -"; // Resume from where we left off
    }
    
    cmd += " -o \"" + output_path + "\"";
    cmd += " \"" + url + "\"";
    cmd += " 2>&1";

    ISHA_LOG_INFO("DownloadMgr", "Executing: " + cmd);
    int ret = std::system(cmd.c_str());
    
    if (ret != 0) {
        ISHA_LOG_ERROR("DownloadMgr", "curl failed with exit code: " + std::to_string(ret));
        return false;
    }

    return std::filesystem::exists(output_path) && getFileSize(output_path) > 0;
}

} // namespace isha
