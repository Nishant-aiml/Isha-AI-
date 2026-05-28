#include "output_sanitizer.hpp"
#include "../logging/logger.hpp"
#include <unordered_map>

namespace isha {

OutputSanitizer::OutputSanitizer() = default;

SanitizeResult OutputSanitizer::check(const std::string& new_token, 
                                       const std::string& full_output_so_far) {
    tokens_checked_.fetch_add(1, std::memory_order_relaxed);
    uint32_t token_count = tokens_checked_.load(std::memory_order_relaxed);

    // Check consecutive repeat
    if (detectConsecutiveRepeat(new_token)) {
        stops_triggered_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_WARN("OutputSanitizer", "Consecutive repeat detected: token '" 
                      + new_token + "' repeated " + std::to_string(consecutive_count_) + " times");
        return {true, "Consecutive token repeat detected ('" + new_token + "' x" 
                + std::to_string(consecutive_count_) + ")"};
    }

    // Check bigram repetition only if output is substantial (>50 tokens worth ~200 chars)
    if (full_output_so_far.size() > 200 && detectBigramRepetition(full_output_so_far)) {
        stops_triggered_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_WARN("OutputSanitizer", "Excessive bigram repetition detected in output.");
        return {true, "Excessive bigram repetition detected"};
    }

    // Check invalid UTF-8
    if (detectInvalidUTF8(new_token)) {
        stops_triggered_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_WARN("OutputSanitizer", "Invalid UTF-8 sequence detected in token.");
        return {true, "Invalid UTF-8 sequence detected"};
    }

    // Check empty degeneration
    if (detectEmptyDegeneration(full_output_so_far, token_count)) {
        stops_triggered_.fetch_add(1, std::memory_order_relaxed);
        ISHA_LOG_WARN("OutputSanitizer", "Empty/whitespace degeneration detected.");
        return {true, "Empty/whitespace degeneration detected"};
    }

    return {false, ""};
}

void OutputSanitizer::reset() {
    last_token_.clear();
    consecutive_count_ = 0;
    stops_triggered_.store(0, std::memory_order_relaxed);
    tokens_checked_.store(0, std::memory_order_relaxed);
    ISHA_LOG_DEBUG("OutputSanitizer", "Sanitizer state reset for new generation.");
}

uint32_t OutputSanitizer::totalStopsTriggered() const {
    return stops_triggered_.load(std::memory_order_relaxed);
}

uint32_t OutputSanitizer::totalTokensChecked() const {
    return tokens_checked_.load(std::memory_order_relaxed);
}

void OutputSanitizer::setMaxConsecutiveRepeats(uint32_t n) {
    max_consecutive_ = n;
}

void OutputSanitizer::setMaxBigramRepeatRatio(float ratio) {
    max_bigram_ratio_ = ratio;
}

void OutputSanitizer::setStructuredOutputMode(bool enabled) {
    structured_mode_ = enabled;
    ISHA_LOG_INFO("OutputSanitizer", "Structured output mode " 
                  + std::string(enabled ? "enabled" : "disabled"));
}

bool OutputSanitizer::detectConsecutiveRepeat(const std::string& token) {
    if (token == last_token_) {
        consecutive_count_++;
    } else {
        last_token_ = token;
        consecutive_count_ = 1;
    }

    if (consecutive_count_ >= max_consecutive_ && !structured_mode_) {
        return true;
    }
    return false;
}

bool OutputSanitizer::detectBigramRepetition(const std::string& full_output) {
    if (structured_mode_) {
        return false;
    }

    // Split output into words for bigram analysis
    std::vector<std::string> words;
    std::string current_word;
    for (char c : full_output) {
        if (c == ' ' || c == '\n' || c == '\t') {
            if (!current_word.empty()) {
                words.push_back(current_word);
                current_word.clear();
            }
        } else {
            current_word += c;
        }
    }
    if (!current_word.empty()) {
        words.push_back(current_word);
    }

    if (words.size() < 2) {
        return false;
    }

    // Count bigrams
    std::unordered_map<std::string, uint32_t> bigram_counts;
    uint32_t total_bigrams = 0;
    for (size_t i = 0; i + 1 < words.size(); ++i) {
        std::string bigram = words[i] + " " + words[i + 1];
        bigram_counts[bigram]++;
        total_bigrams++;
    }

    if (total_bigrams == 0) {
        return false;
    }

    // Check if any single bigram exceeds the ratio threshold
    for (const auto& pair : bigram_counts) {
        float ratio = static_cast<float>(pair.second) / static_cast<float>(total_bigrams);
        if (ratio > max_bigram_ratio_) {
            return true;
        }
    }

    return false;
}

bool OutputSanitizer::detectInvalidUTF8(const std::string& token) {
    const auto* bytes = reinterpret_cast<const unsigned char*>(token.data());
    size_t len = token.size();
    size_t i = 0;

    while (i < len) {
        unsigned char b = bytes[i];

        int expected_continuation = 0;
        if (b <= 0x7F) {
            // Single byte (ASCII)
            expected_continuation = 0;
        } else if ((b & 0xE0) == 0xC0) {
            // 2-byte sequence
            expected_continuation = 1;
        } else if ((b & 0xF0) == 0xE0) {
            // 3-byte sequence
            expected_continuation = 2;
        } else if ((b & 0xF8) == 0xF0) {
            // 4-byte sequence
            expected_continuation = 3;
        } else {
            // Invalid leading byte or unexpected continuation byte
            return true;
        }

        // Verify continuation bytes
        for (int j = 0; j < expected_continuation; ++j) {
            i++;
            if (i >= len) {
                return true;  // Truncated sequence
            }
            if ((bytes[i] & 0xC0) != 0x80) {
                return true;  // Invalid continuation byte
            }
        }

        i++;
    }

    return false;
}

bool OutputSanitizer::detectEmptyDegeneration(const std::string& full_output, 
                                               uint32_t /*token_count*/) {
    if (full_output.size() <= 100) {
        return false;
    }

    size_t whitespace_count = 0;
    for (char c : full_output) {
        if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
            whitespace_count++;
        }
    }

    float whitespace_ratio = static_cast<float>(whitespace_count) 
                             / static_cast<float>(full_output.size());
    return whitespace_ratio > 0.8f;
}

} // namespace isha
