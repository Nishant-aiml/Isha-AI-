#include "local_data_integrity_manager.hpp"
#include <fstream>
#include <cstring>
#include <iostream>

namespace isha {

uint32_t LocalDataIntegrityManager::calculateCRC32(const std::vector<uint8_t>& data) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

bool LocalDataIntegrityManager::transactionallySaveFile(const std::string& target_path, const std::vector<uint8_t>& data) {
    std::string temp_path = target_path + ".tmp";
    std::string backup_path = target_path + ".bak";

    // Open temp file for writing
    std::ofstream out(temp_path, std::ios::binary);
    if (!out) return false;

    // Write header: 4 bytes Magic ("ISHA"), 4 bytes CRC32, 4 bytes payload length
    char magic[4] = {'I', 'S', 'H', 'A'};
    uint32_t crc = calculateCRC32(data);
    uint32_t length = static_cast<uint32_t>(data.size());

    out.write(magic, 4);
    out.write(reinterpret_cast<const char*>(&crc), 4);
    out.write(reinterpret_cast<const char*>(&length), 4);
    
    // Write actual data
    if (length > 0) {
        out.write(reinterpret_cast<const char*>(data.data()), length);
    }
    
    out.flush();
    out.close();

    // Rotate and swap atomically
    // 1. Move target_path to backup_path
    std::remove(backup_path.c_str());
    std::rename(target_path.c_str(), backup_path.c_str());

    // 2. Move temp_path to target_path
    if (std::rename(temp_path.c_str(), target_path.c_str()) != 0) {
        // Rollback backup in case of catastrophic OS failure
        std::rename(backup_path.c_str(), target_path.c_str());
        return false;
    }
    
    return true;
}

bool LocalDataIntegrityManager::transactionallyReadFile(const std::string& target_path, std::vector<uint8_t>& data) {
    auto attemptRead = [&](const std::string& path) -> bool {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;

        char magic[4];
        in.read(magic, 4);
        if (std::memcmp(magic, "ISHA", 4) != 0) return false;

        uint32_t expected_crc = 0;
        uint32_t length = 0;
        in.read(reinterpret_cast<char*>(&expected_crc), 4);
        in.read(reinterpret_cast<char*>(&length), 4);

        data.resize(length);
        if (length > 0) {
            in.read(reinterpret_cast<char*>(data.data()), length);
        }
        in.close();

        // Validate checksum
        uint32_t actual_crc = calculateCRC32(data);
        return (actual_crc == expected_crc);
    };

    // 1. Attempt reading standard target file
    if (attemptRead(target_path)) {
        return true;
    }

    // 2. Fall back to backup_path on corruption/interrupted write
    std::string backup_path = target_path + ".bak";
    if (attemptRead(backup_path)) {
        // Heal the primary file
        transactionallySaveFile(target_path, data);
        return true;
    }

    return false;
}

} // namespace isha
