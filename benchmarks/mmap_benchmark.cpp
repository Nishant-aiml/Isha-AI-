#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "inference/mmap_manager.hpp"
#include "logging/logger.hpp"
#include <iostream>
#include <fstream>
#include <cassert>
#include <vector>
#include <algorithm>

static void createDummyGguf(const std::string& path, size_t size_bytes, bool valid_magic = true) {
    std::ofstream file(path, std::ios::binary);
    assert(file.is_open());

    // Write GGUF Header
    char magic[4] = {'G', 'G', 'U', 'F'};
    if (!valid_magic) {
        magic[0] = 'X';
    }
    uint32_t version = 3;
    uint64_t tensor_count = 10;
    uint64_t metadata_kv_count = 5;

    file.write(magic, sizeof(magic));
    file.write(reinterpret_cast<char*>(&version), sizeof(version));
    file.write(reinterpret_cast<char*>(&tensor_count), sizeof(tensor_count));
    file.write(reinterpret_cast<char*>(&metadata_kv_count), sizeof(metadata_kv_count));

    // Pad file to target size
    size_t written = 24;
    std::vector<char> padding(1024, 0);
    while (written < size_bytes) {
        size_t to_write = std::min(padding.size(), size_bytes - written);
        file.write(padding.data(), to_write);
        written += to_write;
    }
}

int main() {
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::DEBUG);
    ISHA_LOG_INFO("Benchmark", "=== STARTING MMAP BENCHMARK ===");

    std::string test_model_path = "test_model.gguf";
    size_t target_size = 5 * 1024 * 1024; // 5MB
    createDummyGguf(test_model_path, target_size);

    isha::MmapManager manager;
    assert(manager.getState() == isha::MappingLifecycleState::UNMAPPED);

    // Map file
    bool map_ok = manager.mapFile(test_model_path);
    assert(map_ok);
    assert(manager.getState() == isha::MappingLifecycleState::MAPPED);
    assert(manager.getRegion().length == target_size);
    assert(manager.getRegion().address != nullptr);

    // Unmap file
    manager.unmapFile();
    assert(manager.getState() == isha::MappingLifecycleState::UNMAPPED);
    assert(manager.getRegion().address == nullptr);

    // Adversarial test: Mapping a non-existent file
    bool fail_ok = manager.mapFile("missing_file.gguf");
    assert(!fail_ok);
    assert(manager.getState() == isha::MappingLifecycleState::FAILED);

    // Cleanup local test file
    std::remove(test_model_path.c_str());

    ISHA_LOG_INFO("Benchmark", "=== MMAP BENCHMARK COMPLETED SUCCESSFULLY ===");
    return 0;
}
