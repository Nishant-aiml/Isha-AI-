#ifndef ISHA_AI_OUTPUT_SANITIZER_HPP
#define ISHA_AI_OUTPUT_SANITIZER_HPP

#include <atomic>
#include <cstdint>
#include <string>

namespace isha {

struct SanitizeResult {
    bool should_stop = false;
    std::string reason;
};

class OutputSanitizer {
public:
    OutputSanitizer();

    // Check each new token against accumulated output
    SanitizeResult check(const std::string& new_token, const std::string& full_output_so_far);

    // Reset state for new generation
    void reset();

    // Stats
    uint32_t totalStopsTriggered() const;
    uint32_t totalTokensChecked() const;

    // Configuration
    void setMaxConsecutiveRepeats(uint32_t n);
    void setMaxBigramRepeatRatio(float ratio);
    void setStructuredOutputMode(bool enabled);

private:
    // Detect: same token repeated consecutively > threshold
    bool detectConsecutiveRepeat(const std::string& token);

    // Detect: excessive bigram repetition (>ratio of total)
    bool detectBigramRepetition(const std::string& full_output);

    // Detect: invalid UTF-8 sequences
    bool detectInvalidUTF8(const std::string& token);

    // Detect: empty/whitespace degeneration
    bool detectEmptyDegeneration(const std::string& full_output, uint32_t token_count);

    std::string last_token_;
    uint32_t consecutive_count_ = 0;
    uint32_t max_consecutive_ = 5;
    float max_bigram_ratio_ = 0.6f;
    bool structured_mode_ = false;

    std::atomic<uint32_t> stops_triggered_{0};
    std::atomic<uint32_t> tokens_checked_{0};
};

} // namespace isha

#endif // ISHA_AI_OUTPUT_SANITIZER_HPP
