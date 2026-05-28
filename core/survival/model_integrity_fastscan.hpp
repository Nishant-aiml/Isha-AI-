#ifndef ISHA_AI_MODEL_INTEGRITY_FASTSCAN_HPP
#define ISHA_AI_MODEL_INTEGRITY_FASTSCAN_HPP

#include <string>

namespace isha {

class ModelIntegrityFastscan {
public:
    enum class ScanResult {
        SUCCESS,
        HEADER_MALFORMED,
        METADATA_MISMATCH,
        MMAP_UNMAPPED,
        SPOT_CHECK_CORRUPT
    };

    static std::string resultToString(ScanResult result);

    // Runs lightweight checks (GGUF magic, headers, spot sample hashes) in <10ms to prevent cold start ANRs
    static ScanResult performFastScan(const std::string& model_path, bool is_emmc);

    // Invokes full resource-intensive SHA256 checksums only under recovery triggers
    static bool performFullChecksum(const std::string& model_path, const std::string& expected_checksum);
};

} // namespace isha

#endif // ISHA_AI_MODEL_INTEGRITY_FASTSCAN_HPP
