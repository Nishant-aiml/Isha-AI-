#include "gguf_loader.hpp"
#include "../logging/logger.hpp"
#include <fstream>
#include <cstring>

namespace isha {

GGUFLoader::GGUFLoader() {
    std::memset(&header_, 0, sizeof(header_));
}

bool GGUFLoader::parseFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        ISHA_LOG_ERROR("GGUFLoader", "Failed to open file: " + path);
        return false;
    }

    // Read GGUFHeader
    file.read(reinterpret_cast<char*>(&header_), sizeof(header_));
    if (file.gcount() != sizeof(header_)) {
        ISHA_LOG_ERROR("GGUFLoader", "Truncated GGUF file header: " + path);
        return false;
    }

    // Validate GGUF Magic (must be "GGUF" in ASCII, i.e., 'G', 'G', 'U', 'F')
    if (header_.magic[0] != 'G' || header_.magic[1] != 'G' || 
        header_.magic[2] != 'U' || header_.magic[3] != 'F') {
        ISHA_LOG_ERROR("GGUFLoader", "Invalid GGUF magic bytes: " + path);
        return false;
    }

    ISHA_LOG_INFO("GGUFLoader", "Parsed GGUF file v" + std::to_string(header_.version) 
                  + ", Tensors: " + std::to_string(header_.tensor_count)
                  + ", Metadata KVs: " + std::to_string(header_.metadata_kv_count));

    // Simulate reading mock metadata and tensors for Phase 2 validation
    metadata_["general.architecture"] = "llama";
    metadata_["general.name"] = "Mock-Qwen-Edge";
    
    for (uint64_t i = 0; i < header_.tensor_count; ++i) {
        tensors_.push_back({
            "token_embeddings.weight", 2, {4096, 32000}, 0, i * 4096 * 32000
        });
    }

    return true;
}

} // namespace isha
