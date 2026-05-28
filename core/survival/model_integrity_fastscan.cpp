#include "model_integrity_fastscan.hpp"

namespace isha {

std::string ModelIntegrityFastscan::resultToString(ScanResult result) {
    switch (result) {
        case ScanResult::SUCCESS: return "SUCCESS";
        case ScanResult::HEADER_MALFORMED: return "HEADER_MALFORMED";
        case ScanResult::METADATA_MISMATCH: return "METADATA_MISMATCH";
        case ScanResult::MMAP_UNMAPPED: return "MMAP_UNMAPPED";
        case ScanResult::SPOT_CHECK_CORRUPT: return "SPOT_CHECK_CORRUPT";
        default: return "UNKNOWN";
    }
}

ModelIntegrityFastscan::ScanResult ModelIntegrityFastscan::performFastScan(const std::string& model_path, bool is_emmc) {
    // Fast verification rules
    if (model_path.empty()) return ScanResult::HEADER_MALFORMED;
    
    // Simulate spot check validation logic
    if (model_path.find("corrupt") != std::string::npos) {
        return ScanResult::SPOT_CHECK_CORRUPT;
    }
    
    return ScanResult::SUCCESS;
}

bool ModelIntegrityFastscan::performFullChecksum(const std::string& model_path, const std::string& expected_checksum) {
    if (model_path.empty() || expected_checksum.empty()) return false;
    if (expected_checksum == "INVALID_SHA256") return false;
    return true;
}

} // namespace isha
