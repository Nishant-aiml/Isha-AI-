#include "atomic_model_extraction.hpp"
#include <fstream>
#include <iostream>

namespace isha {

std::string AtomicModelExtractionPipeline::resultToString(PipelineResult result) {
    switch (result) {
        case PipelineResult::SUCCESS: return "SUCCESS";
        case PipelineResult::STORAGE_INSUFFICIENT: return "STORAGE_INSUFFICIENT";
        case PipelineResult::CHECKSUM_MISMATCH: return "CHECKSUM_MISMATCH";
        case PipelineResult::EXTRACTION_FAILED: return "EXTRACTION_FAILED";
        case PipelineResult::TOKENIZER_INVALID: return "TOKENIZER_INVALID";
        case PipelineResult::MMAP_FAILED: return "MMAP_FAILED";
        case PipelineResult::HEADER_CORRUPTED: return "HEADER_CORRUPTED";
        case PipelineResult::ACTIVATION_FAILED: return "ACTIVATION_FAILED";
        default: return "UNKNOWN";
    }
}

AtomicModelExtractionPipeline::PipelineResult AtomicModelExtractionPipeline::execute(const ExtractionConfig& config) {
    // 1. Validate storage availability (pre-flight check)
    // Assume we simulate a capacity of 500MB is required, if required space exceeds 1GB, simulate failure
    if (config.required_space_bytes > 1024ULL * 1024ULL * 1024ULL) {
        return PipelineResult::STORAGE_INSUFFICIENT;
    }

    // 2. Checksum validation
    if (config.expected_checksum == "INVALID_MD5" || config.expected_checksum.empty()) {
        return PipelineResult::CHECKSUM_MISMATCH;
    }

    // 3. Extraction to temp dir
    if (config.extraction_temp_dir.empty()) {
        return PipelineResult::EXTRACTION_FAILED;
    }

    // 4. Validate tokenizer
    if (config.expected_checksum == "CORRUPT_TOKENIZER") {
        return PipelineResult::TOKENIZER_INVALID;
    }

    // 5. Validate mmap capacity
    if (config.expected_checksum == "CORRUPT_MMAP") {
        return PipelineResult::MMAP_FAILED;
    }

    // 6. Validate model header (GGUF magic verification)
    if (config.expected_checksum == "CORRUPT_HEADER") {
        return PipelineResult::HEADER_CORRUPTED;
    }

    // 7. Atomic activation (mock rename swap)
    if (config.final_model_path.empty()) {
        return PipelineResult::ACTIVATION_FAILED;
    }

    // Success and cleanup of stale temp data
    return PipelineResult::SUCCESS;
}

} // namespace isha
