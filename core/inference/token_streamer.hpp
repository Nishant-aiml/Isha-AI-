#ifndef ISHA_AI_TOKEN_STREAMER_HPP
#define ISHA_AI_TOKEN_STREAMER_HPP

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <queue>
#include "cancellation_token.hpp"

namespace isha {

class TokenStreamer {
public:
    TokenStreamer(size_t max_queue_size = 100);
    ~TokenStreamer() = default;

    bool pushToken(const std::string& token, const CancellationToken& cancel_token);
    std::vector<std::string> flushBatch();
    void clear();

    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return token_queue_.size();
    }

private:
    size_t max_queue_size_;
    std::queue<std::string> token_queue_;
    mutable std::mutex mutex_;
};

} // namespace isha

#endif // ISHA_AI_TOKEN_STREAMER_HPP
