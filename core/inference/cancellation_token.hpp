#ifndef ISHA_AI_CANCELLATION_TOKEN_HPP
#define ISHA_AI_CANCELLATION_TOKEN_HPP

#include <atomic>
#include <stdexcept>

namespace isha {

class CancellationToken {
public:
    CancellationToken() : cancelled_(false) {}
    ~CancellationToken() = default;

    void cancel() {
        cancelled_.store(true, std::memory_order_release);
    }

    bool isCancelled() const {
        return cancelled_.load(std::memory_order_acquire);
    }

    void reset() {
        cancelled_.store(false, std::memory_order_release);
    }

    void checkAndThrow() const {
        if (isCancelled()) {
            throw std::runtime_error("Inference task execution cancelled.");
        }
    }

private:
    std::atomic<bool> cancelled_;
};

} // namespace isha

#endif // ISHA_AI_CANCELLATION_TOKEN_HPP
