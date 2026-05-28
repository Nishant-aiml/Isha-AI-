#include "model_registry.hpp"
#include "../logging/logger.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace isha {

ModelRegistry::ModelRegistry(const std::string& models_root_dir)
    : models_root_(models_root_dir) {
    ISHA_LOG_INFO("ModelRegistry", "Initialized with root: " + models_root_);
}

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

ModelSubsystem ModelRegistry::detectSubsystem(const std::string& filepath) const {
    std::string lower = toLower(filepath);
    // Normalize path separators
    std::replace(lower.begin(), lower.end(), '\\', '/');

    if (lower.find("/llm/") != std::string::npos)        return ModelSubsystem::LLM;
    if (lower.find("/embeddings/") != std::string::npos)  return ModelSubsystem::EMBEDDING;
    if (lower.find("/stt/") != std::string::npos)         return ModelSubsystem::STT;
    if (lower.find("/ocr/") != std::string::npos)         return ModelSubsystem::OCR;
    if (lower.find("/vision/") != std::string::npos)      return ModelSubsystem::VISION;
    if (lower.find("/reranker/") != std::string::npos)    return ModelSubsystem::RERANKER;
    return ModelSubsystem::LLM; // default
}

QuantizationType ModelRegistry::detectQuantization(const std::string& filename) const {
    std::string lower = toLower(filename);
    if (lower.find("q4_k_m") != std::string::npos)  return QuantizationType::Q4_K_M;
    if (lower.find("q4_k_s") != std::string::npos)  return QuantizationType::Q4_K_S;
    if (lower.find("q4_0") != std::string::npos)     return QuantizationType::Q4_0;
    if (lower.find("q5_k_m") != std::string::npos)  return QuantizationType::Q5_K_M;
    if (lower.find("q8_0") != std::string::npos)     return QuantizationType::Q8_0;
    if (lower.find("f16") != std::string::npos)      return QuantizationType::F16;
    if (lower.find("f32") != std::string::npos)      return QuantizationType::F32;
    return QuantizationType::UNKNOWN;
}

bool ModelRegistry::parseGGUFMetadata(const std::string& filepath, ModelEntry& entry) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    char magic[4] = {};
    file.read(magic, 4);
    if (file.gcount() != 4) return false;

    if (magic[0] != 'G' || magic[1] != 'G' || magic[2] != 'U' || magic[3] != 'F') {
        ISHA_LOG_WARN("ModelRegistry", "Invalid GGUF magic: " + filepath);
        return false;
    }

    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));

    uint64_t tensor_count = 0;
    file.read(reinterpret_cast<char*>(&tensor_count), sizeof(tensor_count));

    uint64_t metadata_kv_count = 0;
    file.read(reinterpret_cast<char*>(&metadata_kv_count), sizeof(metadata_kv_count));

    entry.is_valid = true;
    entry.format = ModelFormat::GGUF;

    ISHA_LOG_DEBUG("ModelRegistry", "GGUF v" + std::to_string(version) +
                   " tensors=" + std::to_string(tensor_count) +
                   " metadata=" + std::to_string(metadata_kv_count) +
                   " path=" + filepath);
    return true;
}

size_t ModelRegistry::discoverModels() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!fs::exists(models_root_)) {
        ISHA_LOG_WARN("ModelRegistry", "Models root directory does not exist: " + models_root_);
        return 0;
    }

    size_t discovered = 0;
    std::error_code ec;

    for (auto& dir_entry : fs::recursive_directory_iterator(models_root_, ec)) {
        if (ec) {
            ISHA_LOG_WARN("ModelRegistry", "Directory iteration error: " + ec.message());
            break;
        }
        if (!dir_entry.is_regular_file()) continue;

        std::string ext = toLower(dir_entry.path().extension().string());
        if (ext != ".gguf" && ext != ".onnx" && ext != ".bin") continue;

        std::string filepath = dir_entry.path().string();
        std::string filename = dir_entry.path().filename().string();
        uint64_t file_size = dir_entry.file_size();

        // Skip tiny files (< 1KB, probably not real models)
        if (file_size < 1024) continue;

        ModelEntry entry;
        entry.filename = filename;
        entry.filepath = filepath;
        entry.file_size_bytes = file_size;
        entry.subsystem = detectSubsystem(filepath);
        entry.quantization = detectQuantization(filename);

        // Detect format
        if (ext == ".gguf")      entry.format = ModelFormat::GGUF;
        else if (ext == ".onnx") entry.format = ModelFormat::ONNX;
        else if (ext == ".bin")  entry.format = ModelFormat::WHISPER_BIN;

        // Generate model_id from filename without extension
        entry.model_id = dir_entry.path().stem().string();

        // Estimate RAM based on subsystem
        switch (entry.subsystem) {
            case ModelSubsystem::LLM:
                entry.estimated_ram_mb = static_cast<unsigned int>(
                    static_cast<double>(file_size) * 1.1 / (1024.0 * 1024.0));
                entry.context_length = 512;
                break;
            case ModelSubsystem::EMBEDDING:
                entry.estimated_ram_mb = 28;
                break;
            case ModelSubsystem::STT:
                entry.estimated_ram_mb = static_cast<unsigned int>(
                    file_size / (1024.0 * 1024.0));
                break;
            case ModelSubsystem::OCR:
                entry.estimated_ram_mb = 50;
                break;
            case ModelSubsystem::VISION:
                entry.estimated_ram_mb = static_cast<unsigned int>(
                    static_cast<double>(file_size) * 1.2 / (1024.0 * 1024.0));
                break;
            case ModelSubsystem::RERANKER:
                entry.estimated_ram_mb = 52;
                break;
        }

        // Set tier requirements based on RAM estimate
        if (entry.estimated_ram_mb <= 250)       entry.minimum_tier = DeviceTier::LOW;
        else if (entry.estimated_ram_mb <= 750)  entry.minimum_tier = DeviceTier::MID;
        else if (entry.estimated_ram_mb <= 1500) entry.minimum_tier = DeviceTier::HIGH;
        else                                     entry.minimum_tier = DeviceTier::FLAGSHIP;

        // Set thermal limits per doc thresholds
        if (entry.estimated_ram_mb > 1000) {
            entry.max_thermal_c = 38.0;  // Large models disabled at HOT
            entry.max_active_seconds = 90;
        } else if (entry.estimated_ram_mb > 500) {
            entry.max_thermal_c = 41.0;  // Medium models disabled at CRITICAL
            entry.max_active_seconds = 300;
        } else {
            entry.max_thermal_c = 45.0;  // Small models tolerant
            entry.max_active_seconds = 0; // unlimited
        }

        // Validate GGUF format
        if (entry.format == ModelFormat::GGUF) {
            parseGGUFMetadata(filepath, entry);
        } else {
            entry.is_valid = true; // Non-GGUF assumed valid if file exists
        }

        models_[entry.model_id] = entry;
        discovered++;

        ISHA_LOG_INFO("ModelRegistry", "Discovered: " + entry.model_id +
                      " subsystem=" + subsystemToString(entry.subsystem) +
                      " format=" + formatToString(entry.format) +
                      " quant=" + quantizationToString(entry.quantization) +
                      " ram=" + std::to_string(entry.estimated_ram_mb) + "MB" +
                      " size=" + std::to_string(file_size / (1024 * 1024)) + "MB");
    }

    ISHA_LOG_INFO("ModelRegistry", "Discovery complete: " + std::to_string(discovered) + " models found.");
    return discovered;
}

const ModelEntry* ModelRegistry::findModel(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = models_.find(model_id);
    if (it == models_.end()) return nullptr;
    return &it->second;
}

std::vector<const ModelEntry*> ModelRegistry::findBySubsystem(ModelSubsystem subsystem) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<const ModelEntry*> results;
    for (auto& [id, entry] : models_) {
        if (entry.subsystem == subsystem) {
            results.push_back(&entry);
        }
    }
    return results;
}

std::vector<const ModelEntry*> ModelRegistry::findCompatible(
    ModelSubsystem subsystem,
    const DeviceProfile& profile,
    double current_temperature_c) const {

    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<const ModelEntry*> results;

    for (auto& [id, entry] : models_) {
        if (entry.subsystem != subsystem) continue;
        if (!entry.is_valid) continue;

        // Tier check: model min tier must be <= device tier
        if (static_cast<int>(entry.minimum_tier) > static_cast<int>(profile.tier)) continue;

        // RAM safety margin: device must have 2x the model's RAM requirement
        if (profile.total_ram_mb < entry.estimated_ram_mb * 2) continue;

        // Thermal check
        if (current_temperature_c >= entry.max_thermal_c) continue;

        results.push_back(&entry);
    }
    return results;
}

bool ModelRegistry::validateModel(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        ISHA_LOG_WARN("ModelRegistry", "Validation failed: model not found: " + model_id);
        return false;
    }

    auto& entry = it->second;

    // Check file exists
    if (!fs::exists(entry.filepath)) {
        ISHA_LOG_ERROR("ModelRegistry", "Validation failed: file missing: " + entry.filepath);
        entry.is_valid = false;
        return false;
    }

    // Check file size matches
    uint64_t current_size = fs::file_size(entry.filepath);
    if (current_size != entry.file_size_bytes) {
        ISHA_LOG_ERROR("ModelRegistry", "Validation failed: size mismatch for " + model_id);
        entry.is_valid = false;
        return false;
    }

    // GGUF magic validation
    if (entry.format == ModelFormat::GGUF) {
        ModelEntry temp;
        if (!parseGGUFMetadata(entry.filepath, temp)) {
            entry.is_valid = false;
            return false;
        }
    }

    entry.is_valid = true;
    return true;
}

bool ModelRegistry::verifyChecksum(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = models_.find(model_id);
    if (it == models_.end()) return false;

    auto& entry = it->second;

    // No checksum set — pass (not yet computed)
    if (entry.sha256_checksum.empty()) {
        entry.checksum_verified = true;
        return true;
    }

    // Simple block-read hash verification placeholder
    // Real implementation would compute SHA-256
    std::ifstream file(entry.filepath, std::ios::binary);
    if (!file.is_open()) {
        entry.checksum_verified = false;
        return false;
    }

    // Verify file is readable and non-empty
    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    if (size <= 0) {
        entry.checksum_verified = false;
        return false;
    }

    // Checksum comparison deferred to real SHA-256 impl
    ISHA_LOG_INFO("ModelRegistry", "Checksum verification deferred for: " + model_id +
                  " (SHA-256 impl pending)");
    entry.checksum_verified = false;
    return false;
}

CompatibilityResult ModelRegistry::checkCompatibility(
    const std::string& model_id,
    const DeviceProfile& profile,
    double current_temperature_c) const {

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = models_.find(model_id);
    if (it == models_.end()) {
        return {false, "Model not found in registry", profile.total_ram_mb, 0};
    }

    const auto& entry = it->second;

    if (!entry.is_valid) {
        return {false, "Model validation failed", profile.total_ram_mb, entry.estimated_ram_mb};
    }

    if (static_cast<int>(entry.minimum_tier) > static_cast<int>(profile.tier)) {
        return {false, "Device tier " + deviceTierToString(profile.tier) +
                " below minimum " + deviceTierToString(entry.minimum_tier),
                profile.total_ram_mb, entry.estimated_ram_mb};
    }

    if (profile.total_ram_mb < entry.estimated_ram_mb * 2) {
        return {false, "Insufficient RAM: need " + std::to_string(entry.estimated_ram_mb * 2) +
                "MB, have " + std::to_string(profile.total_ram_mb) + "MB",
                profile.total_ram_mb, entry.estimated_ram_mb};
    }

    if (current_temperature_c >= entry.max_thermal_c) {
        return {false, "Thermal limit exceeded: " + std::to_string(current_temperature_c) +
                "C >= " + std::to_string(entry.max_thermal_c) + "C",
                profile.total_ram_mb, entry.estimated_ram_mb};
    }

    return {true, "Compatible", profile.total_ram_mb, entry.estimated_ram_mb};
}

bool ModelRegistry::registerModel(ModelEntry entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (entry.model_id.empty()) return false;
    models_[entry.model_id] = std::move(entry);
    ISHA_LOG_INFO("ModelRegistry", "Registered model: " + models_.rbegin()->second.model_id);
    return true;
}

void ModelRegistry::unregisterModel(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto erased = models_.erase(model_id);
    if (erased > 0) {
        ISHA_LOG_INFO("ModelRegistry", "Unregistered model: " + model_id);
    }
}

size_t ModelRegistry::modelCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return models_.size();
}

size_t ModelRegistry::validModelCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (auto& [id, entry] : models_) {
        if (entry.is_valid) count++;
    }
    return count;
}

std::vector<ModelEntry> ModelRegistry::getAllModels() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ModelEntry> result;
    result.reserve(models_.size());
    for (auto& [id, entry] : models_) {
        result.push_back(entry);
    }
    return result;
}

} // namespace isha
