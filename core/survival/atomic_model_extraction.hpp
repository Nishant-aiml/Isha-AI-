#ifndef ISHA_AI_ATOMIC_MODEL_EXTRACTION_HPP
#define ISHA_AI_ATOMIC_MODEL_EXTRACTION_HPP

#include <string>

namespace isha {

class AtomicModelExtractionPipeline {
public:
    struct ExtractionConfig {
        std::string download_temp_path;
        std::string extraction_temp_dir;
        std::string final_model_path;
        std::string expected_checksum;
        size_t required_space_bytes;
    };

    enum class PipelineResult {
        SUCCESS,
        STORAGE_INSUFFICIENT,
        CHECKSUM_MISMATCH,
        EXTRACTION_FAILED,
        TOKENIZER_INVALID,
        MMAP_FAILED,
        HEADER_CORRUPTED,
        ACTIVATION_FAILED
    };

    static std::string resultToString(PipelineResult result);

    static PipelineResult execute(const ExtractionConfig& config);
};

} // namespace isha

#endif // ISHA_AI_ATOMIC_MODEL_EXTRACTION_HPP
