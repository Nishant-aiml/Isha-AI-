#ifndef ISHA_AI_LOCAL_DATA_INTEGRITY_MANAGER_HPP
#define ISHA_AI_LOCAL_DATA_INTEGRITY_MANAGER_HPP

#include <string>
#include <vector>

namespace isha {

class LocalDataIntegrityManager {
public:
    // Computes basic checksum to guard file content integrity
    static uint32_t calculateCRC32(const std::vector<uint8_t>& data);

    // Save payload transactionally using double buffering (write to temp -> fsync -> rename swap)
    static bool transactionallySaveFile(const std::string& target_path, const std::vector<uint8_t>& data);

    // Load payload verifying CRC32 checksum, attempting automatic rollback if base file is damaged
    static bool transactionallyReadFile(const std::string& target_path, std::vector<uint8_t>& data);
};

} // namespace isha

#endif // ISHA_AI_LOCAL_DATA_INTEGRITY_MANAGER_HPP
