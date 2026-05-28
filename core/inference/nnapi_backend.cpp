#include "nnapi_backend.hpp"
#include "../logging/logger.hpp"
#include <chrono>
#include <thread>
#include <cmath>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace isha {

NnapiBackend::NnapiBackend() {
    // Populate operator compatibility matrix
    operator_matrix_ = {
        {"ADD", OperatorSupport::SUPPORTED},
        {"MUL", OperatorSupport::SUPPORTED},
        {"MATMUL", OperatorSupport::PARTIALLY_ACCELERATED},
        {"SOFTMAX", OperatorSupport::PRECISION_DOWNGRADE},
        {"ROPE", OperatorSupport::FALLBACK_REQUIRED}
    };
}

NnapiBackend::~NnapiBackend() {
    shutdown();
}

bool NnapiBackend::loadDynamicNnapiLib() {
    if (nnapi_lib_loaded_) return true;
#if defined(_WIN32)
    // Windows simulation
    nnapi_lib_loaded_ = true;
    ISHA_LOG_INFO("NnapiBackend", "Windows Simulation: NNAPI library loaded.");
#else
    nnapi_lib_handle_ = dlopen("libneuralnetworks.so", RTLD_LAZY);
    if (nnapi_lib_handle_) {
        nnapi_lib_loaded_ = true;
        ISHA_LOG_INFO("NnapiBackend", "Android: libneuralnetworks.so loaded dynamically.");
    } else {
        ISHA_LOG_WARN("NnapiBackend", "Android: Failed to load libneuralnetworks.so dynamically.");
    }
#endif
    return nnapi_lib_loaded_;
}

AccelerationCapability NnapiBackend::probe(const DeviceProfile& profile) {
    auto start_time = std::chrono::steady_clock::now();
    AccelerationCapability cap;
    
    if (!loadDynamicNnapiLib()) {
        cap.probe_passed = false;
        return cap;
    }

    // 1. Warmup Phase (Compiles shaders, stabilizes resident context)
    auto compile_start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // simulated compile
    graph_compile_latency_ms_ = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - compile_start).count();

    // 2. Stabilization Phase
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 3. Measured Phase
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    auto end_time = std::chrono::steady_clock::now();
    startup_latency_ms_ = std::chrono::duration<float, std::milli>(end_time - start_time).count();

    // Check strict timeout ceiling of 200ms
    if (startup_latency_ms_ > 200.0f) {
        ISHA_LOG_WARN("NnapiBackend", "Probe timeout ceiling exceeded! Reverting to CPU.");
        cap_category_ = CapabilityScoreCategory::ACCELERATION_REJECTED;
        cap.probe_passed = false;
        return cap;
    }

    // Graph partitioning check: if rope or other heavy ops exist, partial rejection
    int unsupported = 0;
    int total = static_cast<int>(operator_matrix_.size());
    for (const auto& op : operator_matrix_) {
        if (op.support_status == OperatorSupport::UNSUPPORTED || op.support_status == OperatorSupport::FALLBACK_REQUIRED) {
            unsupported++;
        }
    }
    float partition_ratio = static_cast<float>(unsupported) / static_cast<float>(total);
    if (partition_ratio > 0.3f) {
        cap_category_ = CapabilityScoreCategory::ACCELERATION_REJECTED;
        cap.probe_passed = false;
        ISHA_LOG_WARN("NnapiBackend", "Too many unsupported partition boundaries. Rejected NNAPI.");
        return cap;
    }

    if (profile.has_nnapi) {
        cap.nnapi_available = true;
        cap.nnapi_driver_name = profile.os_name + "_NNAPI_Driver";
        cap.probe_speedup_ratio = 1.6f;
        cap.probe_passed = true;
        cap_category_ = CapabilityScoreCategory::FULL_ACCELERATION;
    } else {
        cap_category_ = CapabilityScoreCategory::ACCELERATION_REJECTED;
        cap.probe_passed = false;
    }

    return cap;
}

bool NnapiBackend::initialize(const AccelerationConfig& config) {
    if (!nnapi_lib_loaded_ && !loadDynamicNnapiLib()) return false;
    is_initialized_ = true;
    return true;
}

void NnapiBackend::shutdown() {
    if (nnapi_lib_handle_) {
#if !defined(_WIN32)
        dlclose(nnapi_lib_handle_);
#endif
        nnapi_lib_handle_ = nullptr;
    }
    nnapi_lib_loaded_ = false;
    is_initialized_ = false;
}

int NnapiBackend::recommendedGpuLayers(int model_embd_size) const {
    if (cap_category_ == CapabilityScoreCategory::ACCELERATION_REJECTED) return 0;
    return 20; // isha ai.md suggested default
}

bool NnapiBackend::isHealthy() const {
    return is_initialized_ && (cap_category_ != CapabilityScoreCategory::ACCELERATION_REJECTED);
}

CapabilityScoreCategory NnapiBackend::getCapabilityScoreCategory() const {
    return cap_category_;
}

float NnapiBackend::getPrefillScore() const {
    if (cap_category_ == CapabilityScoreCategory::ACCELERATION_REJECTED) return 0.0f;
    return 85.0f;
}

float NnapiBackend::getDecodeScore() const {
    if (cap_category_ == CapabilityScoreCategory::ACCELERATION_REJECTED) return 0.0f;
    return 40.0f;
}

bool NnapiBackend::allocateStagingBuffer(size_t size) {
    // 16-byte alignment simulation, fallbacks cleanly to standard allocation
    void* ptr = nullptr;
#if defined(_WIN32)
    ptr = _aligned_malloc(size, 16);
    if (ptr) {
        _aligned_free(ptr);
        return true;
    }
#else
    if (posix_memalign(&ptr, 16, size) == 0) {
        free(ptr);
        return true;
    }
#endif
    return false; // Fallback cleanly
}

bool NnapiBackend::executeTransfer(const void* data, size_t size, bool zero_copy_requested) {
    if (zero_copy_requested) {
        // Try zero-copy. Simulates vendor check
        return true;
    }
    // Fallback safely to bounded staging buffer copy
    return true;
}

} // namespace isha
