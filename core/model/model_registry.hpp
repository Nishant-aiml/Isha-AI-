#ifndef ISHA_AI_MODEL_REGISTRY_HPP
#define ISHA_AI_MODEL_REGISTRY_HPP

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <cstdint>
#include "../config/device_profile.hpp"

namespace isha {

enum class ModelSubsystem {
    LLM,
    EMBEDDING,
    STT,
    OCR,
    VISION,
    RERANKER
};

enum class QuantizationType {
    Q4_0,
    Q4_K_M,
    Q4_K_S,
    Q5_K_M,
    Q8_0,
    F16,
    F32,
    UNKNOWN
};

enum class ModelFormat {
    GGUF,
    ONNX,
    COREML,
    WHISPER_BIN,
    CUSTOM,
    UNKNOWN
};

inline std::string subsystemToString(ModelSubsystem s) {
    switch (s) {
        case ModelSubsystem::LLM:       return "LLM";
        case ModelSubsystem::EMBEDDING: return "EMBEDDING";
        case ModelSubsystem::STT:       return "STT";
        case ModelSubsystem::OCR:       return "OCR";
        case ModelSubsystem::VISION:    return "VISION";
        case ModelSubsystem::RERANKER:  return "RERANKER";
        default: return "UNKNOWN";
    }
}

inline std::string quantizationToString(QuantizationType q) {
    switch (q) {
        case QuantizationType::Q4_0:    return "Q4_0";
        case QuantizationType::Q4_K_M:  return "Q4_K_M";
        case QuantizationType::Q4_K_S:  return "Q4_K_S";
        case QuantizationType::Q5_K_M:  return "Q5_K_M";
        case QuantizationType::Q8_0:    return "Q8_0";
        case QuantizationType::F16:     return "F16";
        case QuantizationType::F32:     return "F32";
        default: return "UNKNOWN";
    }
}

inline std::string formatToString(ModelFormat f) {
    switch (f) {
        case ModelFormat::GGUF:        return "GGUF";
        case ModelFormat::ONNX:        return "ONNX";
        case ModelFormat::COREML:      return "COREML";
        case ModelFormat::WHISPER_BIN: return "WHISPER_BIN";
        case ModelFormat::CUSTOM:      return "CUSTOM";
        default: return "UNKNOWN";
    }
}

struct ModelEntry {
    std::string model_id;
    std::string filename;
    std::string filepath;
    ModelSubsystem subsystem = ModelSubsystem::LLM;
    ModelFormat format = ModelFormat::UNKNOWN;
    QuantizationType quantization = QuantizationType::UNKNOWN;

    // Resource constraints
    unsigned int estimated_ram_mb = 0;
    uint64_t file_size_bytes = 0;
    unsigned int context_length = 512;

    // Compatibility
    DeviceTier minimum_tier = DeviceTier::LOW;
    double max_thermal_c = 41.0;     // Disabled above this temperature
    unsigned int max_active_seconds = 0; // 0 = unlimited (T0)

    // Validation
    std::string sha256_checksum;
    bool checksum_verified = false;
    bool is_valid = false;

    // Version
    std::string version;
};

struct CompatibilityResult {
    bool compatible = false;
    std::string reason;
    unsigned int available_ram_mb = 0;
    unsigned int required_ram_mb = 0;
};

class ModelRegistry {
public:
    explicit ModelRegistry(const std::string& models_root_dir);
    ~ModelRegistry() = default;

    // Discovery
    size_t discoverModels();

    // Lookup
    const ModelEntry* findModel(const std::string& model_id) const;
    std::vector<const ModelEntry*> findBySubsystem(ModelSubsystem subsystem) const;
    std::vector<const ModelEntry*> findCompatible(ModelSubsystem subsystem,
                                                   const DeviceProfile& profile,
                                                   double current_temperature_c) const;

    // Validation
    bool validateModel(const std::string& model_id);
    bool verifyChecksum(const std::string& model_id);
    CompatibilityResult checkCompatibility(const std::string& model_id,
                                            const DeviceProfile& profile,
                                            double current_temperature_c) const;

    // Registration
    bool registerModel(ModelEntry entry);
    void unregisterModel(const std::string& model_id);

    // Stats
    size_t modelCount() const;
    size_t validModelCount() const;

    // All entries
    std::vector<ModelEntry> getAllModels() const;

private:
    // Parse GGUF header to extract metadata
    bool parseGGUFMetadata(const std::string& filepath, ModelEntry& entry);

    // Detect subsystem from directory path
    ModelSubsystem detectSubsystem(const std::string& filepath) const;

    // Detect quantization from filename convention
    QuantizationType detectQuantization(const std::string& filename) const;

    std::string models_root_;
    std::map<std::string, ModelEntry> models_;
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_MODEL_REGISTRY_HPP
