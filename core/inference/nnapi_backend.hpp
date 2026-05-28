#ifndef ISHA_AI_NNAPI_BACKEND_HPP
#define ISHA_AI_NNAPI_BACKEND_HPP

#include "acceleration_backend.hpp"
#include <string>
#include <vector>

namespace isha {

enum class OperatorSupport {
    SUPPORTED,
    UNSUPPORTED,
    PARTIALLY_ACCELERATED,
    FALLBACK_REQUIRED,
    PRECISION_DOWNGRADE,
    LAYOUT_CONVERSION_REQUIRED,
    STAGING_COPY_REQUIRED
};

enum class CapabilityScoreCategory {
    FULL_ACCELERATION,
    PARTIAL_ACCELERATION,
    FALLBACK_HEAVY,
    THERMALLY_UNSAFE,
    JITTER_UNSTABLE,
    UNSTABLE,
    ACCELERATION_REJECTED
};

struct PrefillCapabilityMetrics {
    float acceleration_gain = 0.0f;
    float graph_compile_latency_ms = 0.0f;
    float transfer_overhead_ms = 0.0f;
    float thermal_spike_c = 0.0f;
    float memory_spike_bytes = 0.0f;
    float queue_latency_ms = 0.0f;
};

struct DecodeCapabilityMetrics {
    float sustained_tok_sec = 0.0f;
    float decode_jitter_ms = 0.0f;
    float cancellation_responsiveness_score = 0.0f;
    float thermal_drift_c = 0.0f;
    float scheduler_responsiveness_score = 0.0f;
    int fallback_frequency = 0;
};

struct OperatorCapabilityRecord {
    std::string op_name;
    OperatorSupport support_status = OperatorSupport::SUPPORTED;
};

class NnapiBackend : public IAccelerationBackend {
public:
    NnapiBackend();
    ~NnapiBackend() override;

    // IAccelerationBackend interface
    AccelerationBackend type() const override { return AccelerationBackend::NNAPI; }
    AccelerationCapability probe(const DeviceProfile& profile) override;
    bool initialize(const AccelerationConfig& config) override;
    void shutdown() override;
    int recommendedGpuLayers(int model_embd_size) const override;
    bool isHealthy() const override;

    // Phase 8A Capability Checks
    CapabilityScoreCategory getCapabilityScoreCategory() const;
    float getPrefillScore() const;
    float getDecodeScore() const;
    
    // Staging and zero-copy transfers
    bool allocateStagingBuffer(size_t size);
    bool executeTransfer(const void* data, size_t size, bool zero_copy_requested);

private:
    bool loadDynamicNnapiLib();
    
    bool is_initialized_ = false;
    bool nnapi_lib_loaded_ = false;
    void* nnapi_lib_handle_ = nullptr;
    
    std::vector<OperatorCapabilityRecord> operator_matrix_;
    CapabilityScoreCategory cap_category_ = CapabilityScoreCategory::FULL_ACCELERATION;
    
    // Telemetry fields
    float startup_latency_ms_ = 0.0f;
    float graph_compile_latency_ms_ = 0.0f;
    float warmup_latency_ms_ = 0.0f;
    float first_token_latency_ms_ = 0.0f;
};

} // namespace isha

#endif // ISHA_AI_NNAPI_BACKEND_HPP
